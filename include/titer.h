/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2022 Sanpe <sanpeqf@gmail.com>
 */

#ifndef _TITER_H_
#define _TITER_H_

#include <stddef.h>

#define TITER_BASE_DEFINE(TISTATIC, TINAME, TISTRUCT, TILEFT, TIRIGHT)  \
TISTATIC TISTRUCT *TINAME##_left_far(const TISTRUCT *node)              \
{                                                                       \
    /* Go left as we can */                                             \
    while (node->TILEFT)                                                \
        node = node->TILEFT;                                            \
                                                                        \
    return (TISTRUCT *)node;                                            \
}                                                                       \
                                                                        \
TISTATIC TISTRUCT *TINAME##_right_far(const TISTRUCT *node)             \
{                                                                       \
    /* Go right as we can */                                            \
    while (node->TIRIGHT)                                               \
        node = node->TIRIGHT;                                           \
                                                                        \
    return (TISTRUCT *)node;                                            \
}                                                                       \
                                                                        \
TISTATIC TISTRUCT *TINAME##_left_deep(const TISTRUCT *node)             \
{                                                                       \
    /* Go left deep as we can */                                        \
    while (node) {                                                      \
        if (node->TILEFT)                                               \
            node = node->TILEFT;                                        \
        else if (node->TIRIGHT)                                         \
            node = node->TIRIGHT;                                       \
        else                                                            \
            return (TISTRUCT *)node;                                    \
    }                                                                   \
                                                                        \
    return NULL;                                                        \
}                                                                       \
                                                                        \
TISTATIC TISTRUCT *TINAME##_right_deep(const TISTRUCT *node)            \
{                                                                       \
    /* Go right deep as we can */                                       \
    while (node) {                                                      \
        if (node->TIRIGHT)                                              \
            node = node->TIRIGHT;                                       \
        else if (node->TILEFT)                                          \
            node = node->TILEFT;                                        \
        else                                                            \
            return (TISTRUCT *)node;                                    \
    }                                                                   \
                                                                        \
    return NULL;                                                        \
}

#define TITER_INORDER_DEFINE(TISTATIC, TINAME, TIROOT, TINODE,          \
                             TISTRUCT, TIPARENT, TILEFT, TIRIGHT)       \
TISTATIC TISTRUCT *TINAME##_first(const TIROOT *root)                   \
{                                                                       \
    TISTRUCT *node = root->TINODE;                                      \
                                                                        \
    if (!root || !node)                                                 \
        return NULL;                                                    \
                                                                        \
    /* Get the leftmost node */                                         \
    node = TINAME##_left_far(node);                                     \
    return node;                                                        \
}                                                                       \
                                                                        \
TISTATIC TISTRUCT *TINAME##_last(const TIROOT *root)                    \
{                                                                       \
    TISTRUCT *node = root->TINODE;                                      \
                                                                        \
    if (!root || !node)                                                 \
        return NULL;                                                    \
                                                                        \
    /* Get the rightmost node */                                        \
    node = TINAME##_right_far(node);                                    \
    return node;                                                        \
}                                                                       \
                                                                        \
TISTATIC TISTRUCT *TINAME##_prev(const TISTRUCT *node)                  \
{                                                                       \
    TISTRUCT *parent;                                                   \
                                                                        \
    if (!node)                                                          \
        return NULL;                                                    \
                                                                        \
    /*                                                                  \
     * If there is a left-hand node, go down                            \
     * and then as far right as possible.                               \
     */                                                                 \
    if (node->TILEFT) {                                                 \
        node = node->TILEFT;                                            \
        return TINAME##_right_far(node);                                \
    }                                                                   \
                                                                        \
    /*                                                                  \
     * No left-hand children. Go up till we find an ancestor            \
     * which is a right-hand child of its parent.                       \
     */                                                                 \
    while ((parent = node->TIPARENT) && node != parent->TIRIGHT)        \
        node = parent;                                                  \
                                                                        \
    return parent;                                                      \
}                                                                       \
                                                                        \
TISTATIC TISTRUCT *TINAME##_next(const TISTRUCT *node)                  \
{                                                                       \
    TISTRUCT *parent;                                                   \
                                                                        \
    if (!node)                                                          \
        return NULL;                                                    \
                                                                        \
    /*                                                                  \
     * If there is a right-hand node, go down                           \
     * and then as far left as possible.                                \
     */                                                                 \
    if (node->TIRIGHT) {                                                \
        node = node->TIRIGHT;                                           \
        return TINAME##_left_far(node);                                 \
    }                                                                   \
                                                                        \
    /*                                                                  \
     * No right-hand children. Go up till we find an ancestor           \
     * which is a left-hand child of its parent.                        \
     */                                                                 \
    while ((parent = node->TIPARENT) && node != parent->TILEFT)         \
        node = parent;                                                  \
                                                                        \
    return parent;                                                      \
}

#define TITER_PREORDER_DEFINE(TISTATIC, TINAME, TIROOT, TINODE,         \
                              TISTRUCT, TIPARENT, TILEFT, TIRIGHT)      \
TISTATIC TISTRUCT *TINAME##_pre_first(const TIROOT *root)               \
{                                                                       \
    return root->TINODE;                                                \
}                                                                       \
                                                                        \
TISTATIC TISTRUCT *TINAME##_pre_next(const TISTRUCT *node)              \
{                                                                       \
    TISTRUCT *parent;                                                   \
                                                                        \
    if (!node)                                                          \
        return NULL;                                                    \
                                                                        \
    /**                                                                 \
     * If there are left and right child nodes,                         \
     * then we iterate directly.                                        \
     */                                                                 \
    if (node->TILEFT)                                                   \
        return node->TILEFT;                                            \
                                                                        \
    if (node->TIRIGHT)                                                  \
        return node->TIRIGHT;                                           \
                                                                        \
    /**                                                                 \
     * if we have no children, Go up till we find an ancestor           \
     * which have a another right-hand child.                           \
     */                                                                 \
    while ((parent = node->TIPARENT) &&                                 \
           (!parent->TIRIGHT || node == parent->TIRIGHT))               \
        node = parent;                                                  \
                                                                        \
    return parent ? parent->TIRIGHT : NULL;                             \
}

#define TITER_POSTORDER_DEFINE(TISTATIC, TINAME, TIROOT, TINODE,        \
                               TISTRUCT, TIPARENT, TILEFT, TIRIGHT)     \
TISTATIC TISTRUCT *TINAME##_post_first(const TIROOT *root)              \
{                                                                       \
    TISTRUCT *node = root->TINODE;                                      \
                                                                        \
    if (!root || !node)                                                 \
        return NULL;                                                    \
                                                                        \
    return TINAME##_left_deep(node);                                    \
}                                                                       \
                                                                        \
TISTATIC TISTRUCT *TINAME##_post_next(const TISTRUCT *node)             \
{                                                                       \
    const TISTRUCT *parent;                                             \
                                                                        \
    if (!node)                                                          \
        return NULL;                                                    \
                                                                        \
    parent = node->TIPARENT;                                            \
                                                                        \
    if (parent && node == parent->TILEFT && parent->TIRIGHT)            \
        return TINAME##_left_deep(parent->TIRIGHT);                     \
    else                                                                \
        return (TISTRUCT *)parent;                                      \
}

#define TITER_LEVELORDER_DEFINE(TISTATIC, TINAME, TIROOT, TINODE,       \
                                TISTRUCT, TILEFT, TIRIGHT)              \
TISTATIC TISTRUCT *                                                     \
TINAME##_level_first(const TIROOT *root, unsigned long *index)          \
{                                                                       \
    *index = 0;                                                         \
    return root->TINODE;                                                \
}                                                                       \
                                                                        \
TISTATIC TISTRUCT *                                                     \
TINAME##_level_next(const TIROOT *root, unsigned long *index)           \
{                                                                       \
    unsigned int depth = 63 - __builtin_clzll(++*index + 1);            \
    TISTRUCT *node = root->TINODE;                                      \
                                                                        \
    while (node && depth--) {                                           \
        if ((*index + 1) & (1UL << depth))                              \
            node = node->TIRIGHT;                                       \
        else                                                            \
            node = node->TILEFT;                                        \
    };                                                                  \
                                                                        \
    return node;                                                        \
}

#endif  /* _TITER_H_ */
