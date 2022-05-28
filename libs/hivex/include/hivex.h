/* hivex generated file
 * WARNING: THIS FILE IS GENERATED FROM:
 *   generator/generator.ml
 * ANY CHANGES YOU MAKE TO THIS FILE WILL BE LOST.
 *
 * Copyright (C) 2009-2022 Red Hat Inc.
 * Derived from code by Petter Nordahl-Hagen under a compatible license:
 *   Copyright (c) 1997-2007 Petter Nordahl-Hagen.
 * Derived from code by Markus Stephany under a compatible license:
 *   Copyright (c)2000-2004, Markus Stephany.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef HIVEX_H_
#define HIVEX_H_

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* NOTE: This API is documented in the man page hivex(3). */

/* Hive handle. */
typedef struct hive_h hive_h;

/* Nodes and values. */
typedef size_t hive_node_h;
typedef size_t hive_value_h;
#ifdef ENABLE_UEFI
#include <Protocol/SimpleFileSystem.h>
#endif
#include <errno.h>
#ifdef ENOKEY
# define HIVEX_NO_KEY ENOKEY
#else
# define HIVEX_NO_KEY ENOENT
#endif

/* Pre-defined types. */
enum hive_type {
  /* Just a key without a value */
  hive_t_REG_NONE,
#define hive_t_none hive_t_REG_NONE

  /* A Windows string (encoding is unknown, but often UTF16-LE) */
  hive_t_REG_SZ,
#define hive_t_string hive_t_REG_SZ

  /* A Windows string that contains %env% (environment variable expansion) */
  hive_t_REG_EXPAND_SZ,
#define hive_t_expand_string hive_t_REG_EXPAND_SZ

  /* A blob of binary */
  hive_t_REG_BINARY,
#define hive_t_binary hive_t_REG_BINARY

  /* DWORD (32 bit integer), little endian */
  hive_t_REG_DWORD,
#define hive_t_dword hive_t_REG_DWORD

  /* DWORD (32 bit integer), big endian */
  hive_t_REG_DWORD_BIG_ENDIAN,
#define hive_t_dword_be hive_t_REG_DWORD_BIG_ENDIAN

  /* Symbolic link to another part of the registry tree */
  hive_t_REG_LINK,
#define hive_t_link hive_t_REG_LINK

  /* Multiple Windows strings.  See http://blogs.msdn.com/oldnewthing/archive/2009/10/08/9904646.aspx */
  hive_t_REG_MULTI_SZ,
#define hive_t_multiple_strings hive_t_REG_MULTI_SZ

  /* Resource list */
  hive_t_REG_RESOURCE_LIST,
#define hive_t_resource_list hive_t_REG_RESOURCE_LIST

  /* Resource descriptor */
  hive_t_REG_FULL_RESOURCE_DESCRIPTOR,
#define hive_t_full_resource_description hive_t_REG_FULL_RESOURCE_DESCRIPTOR

  /* Resouce requirements list */
  hive_t_REG_RESOURCE_REQUIREMENTS_LIST,
#define hive_t_resource_requirements_list hive_t_REG_RESOURCE_REQUIREMENTS_LIST

  /* QWORD (64 bit integer), unspecified endianness but usually little endian */
  hive_t_REG_QWORD,
#define hive_t_qword hive_t_REG_QWORD

};

typedef enum hive_type hive_type;

/* Bitmask of flags passed to hivex_open. */
  /* Verbose messages */
#define HIVEX_OPEN_VERBOSE    1
  /* Debug messages */
#define HIVEX_OPEN_DEBUG      2
  /* Enable writes to the hive */
#define HIVEX_OPEN_WRITE      4
  /* Enable heuristics to allow read/write of corrupted hives */
#define HIVEX_OPEN_UNSAFE     8

/* Array of (key, value) pairs passed to hivex_node_set_values. */
struct hive_set_value {
  char *key;
  hive_type t;
  size_t len;
  char *value;
};
typedef struct hive_set_value hive_set_value;

/* Functions. */
extern hive_h *hivex_open (const char *filename, int flags);
extern int hivex_close (hive_h *h);
extern hive_node_h hivex_root (hive_h *h);
extern int64_t hivex_last_modified (hive_h *h);
extern char *hivex_node_name (hive_h *h, hive_node_h node);
extern size_t hivex_node_name_len (hive_h *h, hive_node_h node);
extern int64_t hivex_node_timestamp (hive_h *h, hive_node_h node);
extern hive_node_h *hivex_node_children (hive_h *h, hive_node_h node);
extern hive_node_h hivex_node_get_child (hive_h *h, hive_node_h node, const char *name);
extern size_t hivex_node_nr_children (hive_h *h, hive_node_h node);
extern hive_node_h hivex_node_parent (hive_h *h, hive_node_h node);
extern hive_value_h *hivex_node_values (hive_h *h, hive_node_h node);
extern hive_value_h hivex_node_get_value (hive_h *h, hive_node_h node, const char *key);
extern size_t hivex_node_nr_values (hive_h *h, hive_node_h node);
extern size_t hivex_value_key_len (hive_h *h, hive_value_h val);
extern char *hivex_value_key (hive_h *h, hive_value_h val);
extern int hivex_value_type (hive_h *h, hive_value_h val, hive_type *t, size_t *len);
extern size_t hivex_node_struct_length (hive_h *h, hive_node_h node);
extern size_t hivex_value_struct_length (hive_h *h, hive_value_h val);
extern hive_value_h hivex_value_data_cell_offset (hive_h *h, hive_value_h val, size_t *len);
extern char *hivex_value_value (hive_h *h, hive_value_h val, hive_type *t, size_t *len);
extern char *hivex_value_string (hive_h *h, hive_value_h val);
extern char **hivex_value_multiple_strings (hive_h *h, hive_value_h val);
extern int32_t hivex_value_dword (hive_h *h, hive_value_h val);
extern int64_t hivex_value_qword (hive_h *h, hive_value_h val);
extern int hivex_commit (hive_h *h, const char *filename, int flags);
extern hive_node_h hivex_node_add_child (hive_h *h, hive_node_h parent, const char *name);
extern int hivex_node_delete_child (hive_h *h, hive_node_h node);
extern int hivex_node_set_values (hive_h *h, hive_node_h node, size_t nr_values, const hive_set_value *values, int flags);
extern int hivex_node_set_value (hive_h *h, hive_node_h node, const hive_set_value *val, int flags);
#ifdef ENABLE_UEFI
extern hive_h *hivex_open_uefi (EFI_FILE_PROTOCOL *dir, const char *filename, int flags);
extern int hivex_commit_uefi (hive_h *h, EFI_FILE_PROTOCOL *dir, const char *filename, int flags);
#endif

/* Visit all nodes.  This is specific to the C API and is not made
 * available to other languages.  This is because of the complexity
 * of binding callbacks in other languages, but also because other
 * languages make it much simpler to iterate over a tree.
 */
struct hivex_visitor {
  int (*node_start) (hive_h *, void *opaque, hive_node_h, const char *name);
  int (*node_end) (hive_h *, void *opaque, hive_node_h, const char *name);
  int (*value_string) (hive_h *, void *opaque, hive_node_h, hive_value_h, hive_type t, size_t len, const char *key, const char *str);
  int (*value_multiple_strings) (hive_h *, void *opaque, hive_node_h, hive_value_h, hive_type t, size_t len, const char *key, char **argv);
  int (*value_string_invalid_utf16) (hive_h *, void *opaque, hive_node_h, hive_value_h, hive_type t, size_t len, const char *key, const char *str);
  int (*value_dword) (hive_h *, void *opaque, hive_node_h, hive_value_h, hive_type t, size_t len, const char *key, int32_t);
  int (*value_qword) (hive_h *, void *opaque, hive_node_h, hive_value_h, hive_type t, size_t len, const char *key, int64_t);
  int (*value_binary) (hive_h *, void *opaque, hive_node_h, hive_value_h, hive_type t, size_t len, const char *key, const char *value);
  int (*value_none) (hive_h *, void *opaque, hive_node_h, hive_value_h, hive_type t, size_t len, const char *key, const char *value);
  int (*value_other) (hive_h *, void *opaque, hive_node_h, hive_value_h, hive_type t, size_t len, const char *key, const char *value);
  int (*value_any) (hive_h *, void *opaque, hive_node_h, hive_value_h, hive_type t, size_t len, const char *key, const char *value);
};

#define HIVEX_VISIT_SKIP_BAD 1

extern int hivex_visit (hive_h *h, const struct hivex_visitor *visitor, size_t len, void *opaque, int flags);
extern int hivex_visit_node (hive_h *h, hive_node_h node, const struct hivex_visitor *visitor, size_t len, void *opaque, int flags);

#ifdef __cplusplus
}
#endif

#endif /* HIVEX_H_ */
