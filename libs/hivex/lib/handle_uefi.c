/* hivex - Windows Registry "hive" extraction library.
 * Copyright (C) 2009-2011 Red Hat Inc.
 * Derived from code by Petter Nordahl-Hagen under a compatible license:
 *   Copyright (c) 1997-2007 Petter Nordahl-Hagen.
 * Derived from code by Markus Stephany under a compatible license:
 *   Copyright (c) 2000-2004, Markus Stephany.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * See file LICENSE for the full license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <iconv.h>
#include "lock.h"
#include "ctype.h"
#include "system.h"
#include "hivex.h"
#include "hivex-internal.h"
#include "uefi.h"
#include "locate.h"
#include "compatible.h"
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/SimpleFileSystem.h>
#include <Guid/FileInfo.h>

static uint32_t
header_checksum (const hive_h *h)
{
  uint32_t *daddr = (uint32_t *) h->addr;
  size_t i;
  uint32_t sum = 0;

  for (i = 0; i < 0x1fc / 4; ++i) {
    sum ^= le32toh (*daddr);
    daddr++;
  }

  return sum;
}

#define HIVEX_OPEN_MSGLVL_MASK (HIVEX_OPEN_VERBOSE|HIVEX_OPEN_DEBUG)

iconv_t *
_hivex_get_iconv (hive_h *h, recode_type t)
{
  MUTEX_LOCK (h->iconv_cache[t].mutex);
  if (h->iconv_cache[t].handle == NULL) {
    if (t == utf8_to_latin1)
      h->iconv_cache[t].handle = iconv_open ("LATIN1", "UTF-8");
    else if (t == latin1_to_utf8)
      h->iconv_cache[t].handle = iconv_open ("UTF-8", "LATIN1");
    else if (t == utf8_to_utf16le)
      h->iconv_cache[t].handle = iconv_open ("UTF-16LE", "UTF-8");
    else if (t == utf16le_to_utf8)
      h->iconv_cache[t].handle = iconv_open ("UTF-8", "UTF-16LE");
  } else {
    /* reinitialize iconv context */
    iconv (h->iconv_cache[t].handle, NULL, 0, NULL, 0);
  }
  return h->iconv_cache[t].handle;
}

void
_hivex_release_iconv (hive_h *h, recode_type t)
{
  MUTEX_UNLOCK (h->iconv_cache[t].mutex);
}

static hive_h *
_hivex_open (EFI_FILE_PROTOCOL *file, const char *filename, int flags)
{
  hive_h *h = NULL;
  EFI_STATUS st = EFI_SUCCESS;
  EFI_FILE_INFO *info = NULL;

  h = calloc (1, sizeof *h);
  if (h == NULL)
    goto error;

  h->fd = file;
  h->msglvl = flags & HIVEX_OPEN_MSGLVL_MASK;

  const char *debug = getenv ("HIVEX_DEBUG");
  if (debug && STREQ (debug, "1"))
    h->msglvl = 2;

  DEBUG (2, "created handle %p", h);

  h->writable = !!(flags & HIVEX_OPEN_WRITE);
  h->unsafe = !!(flags & HIVEX_OPEN_UNSAFE);
  h->filename = strdup (filename);
  if (h->filename == NULL)
    goto error;

  st = efi_file_get_file_info(h->fd, NULL, &info);
  if (EFI_ERROR(st) || info == NULL)
    goto error;

  h->size = info->FileSize;
  FreePool(info);

  if (h->size < 0x2000) {
    SET_ERRNO (EINVAL,
               "%s: file is too small to be a Windows NT Registry hive file",
               filename);
    goto error;
  }

  h->addr = AllocatePool (h->size);
  if (h->addr == NULL)
    goto error;

  UINTN size = h->size;
  st = h->fd->Read (h->fd, &size, h->addr);
  if (EFI_ERROR(st) || size != h->size)
    goto error;

  /* Check header. */
  if (h->hdr->magic[0] != 'r' ||
      h->hdr->magic[1] != 'e' ||
      h->hdr->magic[2] != 'g' ||
      h->hdr->magic[3] != 'f') {
    SET_ERRNO (ENOTSUP,
               "%s: not a Windows NT Registry hive file", filename);
    goto error;
  }

  /* Check major version. */
  uint32_t major_ver = le32toh (h->hdr->major_ver);
  if (major_ver != 1) {
    SET_ERRNO (ENOTSUP,
               "%s: hive file major version %" PRIu32 " (expected 1)",
               filename, major_ver);
    goto error;
  }

  h->bitmap = calloc (1 + h->size / 32, 1);
  if (h->bitmap == NULL)
    goto error;

  /* Header checksum. */
  uint32_t sum = header_checksum (h);
  if (sum != le32toh (h->hdr->csum)) {
    SET_ERRNO (EINVAL, "%s: bad checksum in hive header", filename);
    goto error;
  }

  for (int t=0; t<nr_recode_types; t++) {
    MUTEX_INIT (h->iconv_cache[t].mutex);
    h->iconv_cache[t].handle = NULL;
  }

  /* Last modified time. */
  h->last_modified = le64toh ((int64_t) h->hdr->last_modified);

  if (h->msglvl >= 2) {
    char *name = _hivex_recode (h, utf16le_to_utf8,
                                h->hdr->name, 64, NULL);

    fprintf (stderr,
             "hivex_open: header fields:\n"
             "  file version             %" PRIu32 ".%" PRIu32 "\n"
             "  sequence nos             %" PRIu32 " %" PRIu32 "\n"
             "    (sequences nos should match if hive was synched at shutdown)\n"
             "  last modified            %" PRIi64 "\n"
             "    (Windows filetime, x 100 ns since 1601-01-01)\n"
             "  original file name       %s\n"
             "    (only 32 chars are stored, name is probably truncated)\n"
             "  root offset              0x%x + 0x1000\n"
             "  end of last page         0x%x + 0x1000 (total file size 0x%zx)\n"
             "  checksum                 0x%x (calculated 0x%x)\n",
             major_ver, le32toh (h->hdr->minor_ver),
             le32toh (h->hdr->sequence1), le32toh (h->hdr->sequence2),
             h->last_modified,
             name ? name : "(conversion failed)",
             le32toh (h->hdr->offset),
             le32toh (h->hdr->blocks), h->size,
             le32toh (h->hdr->csum), sum);
    free (name);
  }

  h->rootoffs = le32toh (h->hdr->offset) + 0x1000;
  h->endpages = le32toh (h->hdr->blocks) + 0x1000;

  DEBUG (2, "root offset = 0x%zx", h->rootoffs);

  /* We'll set this flag when we see a block with the root offset (ie.
   * the root block).
   */
  int seen_root_block = 0, bad_root_block = 0;

  /* Collect some stats. */
  size_t pages = 0;           /* Number of hbin pages read. */
  size_t smallest_page = SIZE_MAX, largest_page = 0;
  size_t blocks = 0;          /* Total number of blocks found. */
  size_t smallest_block = SIZE_MAX, largest_block = 0, blocks_bytes = 0;
  size_t used_blocks = 0;     /* Total number of used blocks found. */
  size_t used_size = 0;       /* Total size (bytes) of used blocks. */

  /* Read the pages and blocks.  The aim here is to be robust against
   * corrupt or malicious registries.  So we make sure the loops
   * always make forward progress.  We add the address of each block
   * we read to a hash table so pointers will only reference the start
   * of valid blocks.
   */
  size_t off;
  struct ntreg_hbin_page *page;
  for (off = 0x1000; off < h->size; off += le32toh (page->page_size)) {
    if (off >= h->endpages)
      break;

    page = (struct ntreg_hbin_page *) ((char *) h->addr + off);
    if (page->magic[0] != 'h' ||
        page->magic[1] != 'b' ||
        page->magic[2] != 'i' ||
        page->magic[3] != 'n') {

      if (!h->unsafe) {
        SET_ERRNO (ENOTSUP,
                   "%s: trailing garbage at end of file "
                   "(at 0x%zx, after %zu pages)",
                   filename, off, pages);
        goto error;
      }

      DEBUG (2,
             "page not found at expected offset 0x%zx, "
             "seeking until one is found or EOF is reached",
             off);

      int found = 0;
      while (off < h->size) {
        off += 0x1000;

        if (off >= h->endpages)
          break;

        page = (struct ntreg_hbin_page *) ((char *) h->addr + off);
        if (page->magic[0] == 'h' &&
            page->magic[1] == 'b' &&
            page->magic[2] == 'i' &&
            page->magic[3] == 'n') {
          DEBUG (2, "found next page by seeking at 0x%zx", off);
          found = 1;
          break;
        }
      }

      if (!found) {
        DEBUG (2, "page not found and end of pages section reached");
        break;
      }
    }

    size_t page_size = le32toh (page->page_size);
    DEBUG (2, "page at 0x%zx, size %zu", off, page_size);
    pages++;
    if (page_size < smallest_page) smallest_page = page_size;
    if (page_size > largest_page) largest_page = page_size;

    if (page_size <= sizeof (struct ntreg_hbin_page) ||
        (page_size & 0x0fff) != 0) {
      SET_ERRNO (ENOTSUP,
                 "%s: page size %zu at 0x%zx, bad registry",
                 filename, page_size, off);
      goto error;
    }

    if (off + page_size > h->size) {
      SET_ERRNO (ENOTSUP,
                 "%s: page size %zu at 0x%zx extends beyond end of file, bad registry",
                 filename, page_size, off);
      goto error;
    }

    size_t page_offset = le32toh(page->offset_first) + 0x1000;

    if (page_offset != off) {
      SET_ERRNO (ENOTSUP,
                 "%s: declared page offset (0x%zx) does not match computed "
                 "offset (0x%zx), bad registry",
                 filename, page_offset, off);
      goto error;
    }

    /* Read the blocks in this page. */
    size_t blkoff;
    struct ntreg_hbin_block *block;
    size_t seg_len;
    for (blkoff = off + 0x20;
         blkoff < off + page_size;
         blkoff += seg_len) {
      blocks++;

      int is_root = blkoff == h->rootoffs;
      if (is_root)
        seen_root_block = 1;

      block = (struct ntreg_hbin_block *) ((char *) h->addr + blkoff);
      int used;
      seg_len = block_len (h, blkoff, &used);
/* https://gcc.gnu.org/bugzilla/show_bug.cgi?id=78665 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-overflow"
      if (seg_len <= 4 || (seg_len & 3) != 0) {
#pragma GCC diagnostic pop
        if (is_root || !h->unsafe) {
          SET_ERRNO (ENOTSUP,
                     "%s, the block at 0x%zx size %" PRIu32
                     " <= 4 or not a multiple of 4, bad registry",
                     filename, blkoff, le32toh (block->seg_len));
          goto error;
        } else {
          DEBUG (2,
                 "%s: block at 0x%zx has invalid size %" PRIu32 ", skipping",
                 filename, blkoff, le32toh (block->seg_len));
          break;
        }
      }

      if (blkoff + seg_len > off + page_size) {
        SET_ERRNO (ENOTSUP,
                   "%s, the block at 0x%zx size %" PRIu32
                   " extends beyond the current page, bad registry",
                   filename, blkoff, le32toh (block->seg_len));
        goto error;
      }

      if (h->msglvl >= 2) {
        unsigned char *id = (unsigned char *) block->id;
        int id0 = id[0], id1 = id[1];

        fprintf (stderr, "%s: %s: "
                 "%s block id %d,%d (%c%c) at 0x%zx size %zu%s\n",
                 "hivex", __func__,
                 used ? "used" : "free",
                 id0, id1,
                 isprint (id0) ? id0 : '.',
                 isprint (id1) ? id1 : '.',
                 blkoff,
                 seg_len, is_root ? " (root)" : "");
      }

      blocks_bytes += seg_len;
      if (seg_len < smallest_block) smallest_block = seg_len;
      if (seg_len > largest_block) largest_block = seg_len;

      if (is_root && !used)
        bad_root_block = 1;

      if (used) {
        used_blocks++;
        used_size += seg_len;

        /* Root block must be an nk-block. */
        if (is_root && (block->id[0] != 'n' || block->id[1] != 'k'))
          bad_root_block = 1;

        /* Note this blkoff is a valid address. */
        BITMAP_SET (h->bitmap, blkoff);
      }
    }
  }

  if (!seen_root_block) {
    SET_ERRNO (ENOTSUP, "%s: no root block found", filename);
    goto error;
  }

  if (bad_root_block) {
    SET_ERRNO (ENOTSUP, "%s: bad root block (free or not nk)", filename);
    goto error;
  }

  DEBUG (1, "successfully read Windows Registry hive file:\n"
         "  pages:          %zu [sml: %zu, lge: %zu]\n"
         "  blocks:         %zu [sml: %zu, avg: %zu, lge: %zu]\n"
         "  blocks used:    %zu\n"
         "  bytes used:     %zu",
         pages, smallest_page, largest_page,
         blocks, smallest_block, blocks_bytes / blocks, largest_block,
         used_blocks, used_size);

  return h;

 error:;
  int err = errno;
  if (err == 0 && EFI_ERROR(st))
    err = efi_status_to_errno(st);
  if (h) {
    free (h->bitmap);
    if (h->addr && h->size) {
      free (h->addr);
    }
    free (h->filename);
    free (h);
  }
  errno = err;
  return NULL;
}

hive_h *
hivex_open (const char *filename, int flags)
{
  locate_ret *loc = NULL;

  loc = AllocateZeroPool(sizeof (locate_ret));
  if (loc == NULL){
    errno = ENOMEM;
    goto error;
  }

  if (!boot_locate(loc, filename)){
    errno = ENOENT;
    goto error;
  }

  if (loc->type != LOCATE_FILE) {
    errno = EINVAL;
    goto error;
  }

  hive_h *h = _hivex_open(loc->file, filename, flags);
  if (h != NULL) {
    if (h->fd != NULL)
      h->fd = NULL;
    h->dir = loc->root;
    h->use_locate = true;
  }
  loc->file->Close(loc->file);
  FreePool(loc);
  return h;
  error:;
  int err = errno;
  if (loc != NULL) {
    if(loc->type == LOCATE_FILE && loc->file != NULL)
      loc->file->Close(loc->file);
    FreePool(loc);
  }
  errno = err;
  return NULL;
}

hive_h *
hivex_open_uefi (EFI_FILE_PROTOCOL *dir, const char *filename, int flags)
{
  EFI_STATUS st;
  CHAR16 *buffer = NULL;
  EFI_FILE_PROTOCOL *fp = NULL;
  UINTN size;
  if (dir == NULL || filename == NULL) {
    errno = EINVAL;
    goto error;
  }
  size = AsciiStrSize(filename) * sizeof(CHAR16);
  buffer = AllocateZeroPool(size);
  if (!buffer) {
    errno = ENOMEM;
    goto error;
  }
  AsciiStrToUnicodeStrS(filename, buffer, size);
  st = dir->Open(dir, &fp, buffer, EFI_FILE_MODE_READ, 0);
  if (EFI_ERROR(st) || !fp)
    goto error;
  FreePool(buffer);
  hive_h *h = _hivex_open(fp, filename, flags);
  if (h != NULL) {
    if (h->fd !=NULL)
      h->fd=NULL;
    h->dir = dir;
  }
  fp->Close(fp);
  return h;
  error:;
  int err = errno;
  if (fp != NULL)
    fp->Close(fp);
  if (buffer != NULL)
    FreePool(buffer);
  errno = err;
  return NULL;
}

int
hivex_close (hive_h *h)
{
  int r = 0;

  DEBUG (1, "hivex_close");

  free (h->bitmap);
  free (h->addr);
  if (h->fd != NULL)
    if (EFI_ERROR(h->fd->Close(h->fd))) 
      r = -1;

  free (h->filename);
  for (int t=0; t<nr_recode_types; t++) {
    if (h->iconv_cache[t].handle != NULL) {
      iconv_close (h->iconv_cache[t].handle);
      h->iconv_cache[t].handle = NULL;
    }
  }
  free (h);

  return r;
}

static int
_hivex_commit (hive_h *h, EFI_FILE_PROTOCOL *fp, int flags)
{
  UINTN size = 0;
  EFI_STATUS st = EFI_SUCCESS;
  EFI_FILE_INFO *info = NULL;

  if (flags != 0) {
    SET_ERRNO (EINVAL, "flags != 0");
    return -1;
  }
  if (fp == NULL && h->fd != NULL)
    fp = h->fd;
  if (fp == NULL) {
    errno = EINVAL;
    return -1;
  }

  CHECK_WRITABLE (-1);

  /* Update the header fields. */
  uint32_t sequence = le32toh (h->hdr->sequence1);
  sequence++;
  h->hdr->sequence1 = htole32 (sequence);
  h->hdr->sequence2 = htole32 (sequence);
  /* XXX Ought to update h->hdr->last_modified. */
  h->hdr->blocks = htole32 (h->endpages - 0x1000);

  /* Recompute header checksum. */
  uint32_t sum = header_checksum (h);
  h->hdr->csum = htole32 (sum);

  DEBUG (2, "hivex_commit: new header checksum: 0x%x", sum);

  st = efi_file_get_file_info(fp, &size, &info);
  if (info != NULL && EFI_ERROR(st)){
    info->FileSize = h->size;
    fp->SetInfo(fp, &gEfiFileInfoGuid, size, info);
    FreePool(info);
  }

  fp->SetPosition(fp, 0);
  size = h->size;
  st = fp->Write(fp, &size,h->addr);
  if (EFI_ERROR(st)) {
    errno = efi_status_to_errno(st);
    return -1;
  }

  if (size != h->size){
    errno = ENOSPC;
    return -1;
  }

  return 0;
}

int
hivex_commit_uefi (hive_h *h, EFI_FILE_PROTOCOL *dir, const char *filename, int flags)
{
  EFI_STATUS st;
  CHAR16 *buffer = NULL;
  EFI_FILE_PROTOCOL *fp = NULL;
  UINTN size;
  if (dir == NULL && h->dir != NULL)
    dir = h->dir;
  if (filename == NULL && h->filename != NULL)
    filename = h->filename;
  bool inv = filename == NULL || dir == NULL;
  if (inv && h->fd == NULL){
    errno = EINVAL;
    goto error;
  }
  fp = h->fd;
  if (!inv) {
    size = AsciiStrSize(filename) * sizeof(CHAR16);
    buffer = AllocateZeroPool(size);
    if (!buffer) {
      errno = ENOMEM;
      goto error;
    }
    AsciiStrToUnicodeStrS(filename, buffer, size);
    UINT64 fm =
      EFI_FILE_MODE_READ | 
      EFI_FILE_MODE_WRITE | 
      EFI_FILE_MODE_CREATE;
    st = dir->Open(dir, &fp, buffer, fm, 0);
    if (EFI_ERROR(st) || !fp){
      errno = efi_status_to_errno(st);
      goto error;
    }
    FreePool(buffer);
  }
  int r = _hivex_commit(h, fp, flags);
  if (h->dir == NULL)
    h->dir = dir;
  fp->Close(fp);
  return r;
  error:;
  int err = errno;
  if (fp != NULL)
    fp->Close(fp);
  if (buffer != NULL)
    FreePool(buffer);
  errno = err;
  return -1;
}

int
hivex_commit (hive_h *h, const char *filename, int flags)
{
  locate_ret *loc = NULL;

  if (filename == NULL && h->filename != NULL)
    filename = h->filename;
  if (h->dir != NULL) {
    if (filename == NULL)
      return hivex_commit_uefi(h, h->dir, NULL, flags);
    if (!h->use_locate)
      return hivex_commit_uefi(h, h->dir, filename, flags);
  }

  loc = AllocateZeroPool(sizeof (locate_ret));
  if (loc == NULL){
    errno = ENOMEM;
    goto error;
  }

  if (!boot_locate_create_file(loc, filename)){
    errno = ENOENT;
    goto error;
  }

  if (loc->type != LOCATE_FILE) {
    errno = EINVAL;
    goto error;
  }

  int r = _hivex_commit(h, loc->file, flags);
  if (h->dir == NULL)
    h->dir = loc->root;
  loc->file->Close(loc->file);
  FreePool(loc);
  return r;
  error:;
  int err = errno;
  if (loc != NULL) {
    if(loc->type == LOCATE_FILE && loc->file != NULL)
      loc->file->Close(loc->file);
    FreePool(loc);
  }
  errno = err;
  return -1;
}
