/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2022 John Sanpe <sanpeqf@gmail.com>
 */

#ifndef _LIBMO_H_
#define _LIBMO_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

/**
 * The Format of GNU MO Files.
 *
 *         byte
 *              +------------------------------------------+
 *           0  | magic number = 0x950412de                |
 *              |                                          |
 *           4  | file format revision = 0                 |
 *              |                                          |
 *           8  | number of strings                        |  == N
 *              |                                          |
 *          12  | offset of table with original strings    |  == O
 *              |                                          |
 *          16  | offset of table with translation strings |  == T
 *              |                                          |
 *          20  | size of hashing table                    |  == S
 *              |                                          |
 *          24  | offset of hashing table                  |  == H
 *              |                                          |
 *              .                                          .
 *              .    (possibly more entries later)         .
 *              .                                          .
 *              |                                          |
 *           O  | length & offset 0th string  ----------------.
 *       O + 8  | length & offset 1st string  ------------------.
 *               ...                                    ...   | |
 * O + ((N-1)*8)| length & offset (N-1)th string           |  | |
 *              |                                          |  | |
 *           T  | length & offset 0th translation  ---------------.
 *       T + 8  | length & offset 1st translation  -----------------.
 *               ...                                    ...   | | | |
 * T + ((N-1)*8)| length & offset (N-1)th translation      |  | | | |
 *              |                                          |  | | | |
 *           H  | start hash table                         |  | | | |
 *               ...                                    ...   | | | |
 *   H + S * 4  | end hash table                           |  | | | |
 *              |                                          |  | | | |
 *              | NUL terminated 0th string  <----------------' | | |
 *              |                                          |    | | |
 *              | NUL terminated 1st string  <------------------' | |
 *              |                                          |      | |
 *               ...                                    ...       | |
 *              |                                          |      | |
 *              | NUL terminated 0th translation  <---------------' |
 *              |                                          |        |
 *              | NUL terminated 1st translation  <-----------------'
 *              |                                          |
 *               ...                                    ...
 *              |                                          |
 *              +------------------------------------------+
 */

struct libmo_header {
    uint32_t magic;
    uint32_t revision;
    uint32_t index_num;
    uint32_t orig_offset;
    uint32_t tran_offset;
    uint32_t hash_size;
    uint32_t hash_offset;
};

struct libmo_sdesc {
    uint32_t length;
    uint32_t offset;
};

struct libmo_context {
    const void *data;
    size_t size;
    bool swab;

    uint16_t major;
    uint16_t minor;

    uint32_t index_num;
    uint32_t hash_size;
    uint32_t orig_offset;
    uint32_t tran_offset;
    uint32_t hash_offset;
};

#define LIBMO_MAGIC 0x950412de
#define LIBMO_MAGIC_SWAB 0xde120495
#define PJWHASH_BITS 32

#define LIBMO_MAJOR(revision) ((revision) >> 8)
#define LIBMO_MINOR(revision) ((revision) & 0xff)

#define LIBMO_ORIG_OFFSET(ctx, index) ((ctx)->orig_offset + (sizeof(struct libmo_sdesc) * (index)))
#define LIBMO_TRAN_OFFSET(ctx, index) ((ctx)->tran_offset + (sizeof(struct libmo_sdesc) * (index)))
#define LIBMO_HASH_OFFSET(ctx, index) ((ctx)->hash_offset + (sizeof(uint32_t) * (index)))

extern int libmo_load(struct libmo_context *ctx, const void *data, size_t size);
extern int libmo_verify(const struct libmo_context *ctx);
extern const char *libmo_lookup(const struct libmo_context *ctx, const char *orig, size_t origlen, size_t *tranlenp);

static inline const char *libmo_gettext(const struct libmo_context *ctx, const char *orig)
{
    const char *tran = libmo_lookup(ctx, orig, 0, NULL);
    return tran ?: orig;
}

#endif /* _LIBMO_H_ */
