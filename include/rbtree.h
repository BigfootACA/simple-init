/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2021 Sanpe <sanpeqf@gmail.com>
 */

#ifndef _RBTREE_H_
#define _RBTREE_H_

#include <stddef.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>

#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define RB_RED      (0)
#define RB_BLACK    (1)
#define RB_NSET     (2)

struct rb_node {
    struct rb_node *parent;
    struct rb_node *left;
    struct rb_node *right;
    bool color;
};

struct rb_root {
    struct rb_node *rb_node;
};

struct rb_root_cached {
    struct rb_root root;
    struct rb_node *rb_leftmost;
};

#define RB_INIT \
    (struct rb_root) {NULL}

#define RB_CACHED_INIT \
    (struct rb_root_cached) {{NULL}, NULL}

#define RB_ROOT(name) \
    struct rb_root name = RB_INIT

#define RB_ROOT_CACHED(name) \
    struct rb_root_cached name = RB_CACHED_INIT

#define RB_EMPTY_NODE(node) \
    ((node)->parent == (node))

#define RB_CLEAR_NODE(node) \
    ((node)->parent = (node))

#define rb_entry(ptr, type, member) ({                  \
    const typeof(((type *)0)->member) *__mptr = (ptr);  \
    (type *)((char *)__mptr - offsetof(type,member));   \
})

#define rb_entry_safe(ptr, type, member) ({             \
    typeof(ptr) _ptr = (ptr);                           \
    _ptr ? rb_entry(_ptr, type, member) : NULL;         \
})

#ifndef POISON_OFFSET
# define POISON_OFFSET 0
#endif

#define POISON_RBNODE1 ((void *) POISON_OFFSET + 0x10)
#define POISON_RBNODE2 ((void *) POISON_OFFSET + 0x20)
#define POISON_RBNODE3 ((void *) POISON_OFFSET + 0x30)

#ifdef DEBUG_RBTREE
extern bool rb_debug_link_check(struct rb_node *parent, struct rb_node **link, struct rb_node *node);
extern bool rb_debug_delete_check(struct rb_node *node);
#endif

typedef long (*rb_find_t)(const struct rb_node *, const void *key);
typedef long (*rb_cmp_t)(const struct rb_node *, const struct rb_node *);

extern void rb_fixup(struct rb_root *root, struct rb_node *node);
extern void rb_erase(struct rb_root *root, struct rb_node *parent);
extern struct rb_node *rb_remove(struct rb_root *root, struct rb_node *node);
extern void rb_replace(struct rb_root *root, struct rb_node *old, struct rb_node *new);
extern struct rb_node *rb_find(const struct rb_root *root, const void *key, rb_find_t);
extern struct rb_node *rb_find_last(struct rb_root *root, const void *key, rb_find_t cmp, struct rb_node **parentp, struct rb_node ***linkp);
extern struct rb_node **rb_parent(struct rb_root *root, struct rb_node **parentp, struct rb_node *node, rb_cmp_t, bool *leftmost);
extern struct rb_node **rb_parent_conflict(struct rb_root *root, struct rb_node **parentp, struct rb_node *node, rb_cmp_t, bool *leftmost);

extern struct rb_node *rb_left_far(const struct rb_node *node);
extern struct rb_node *rb_right_far(const struct rb_node *node);
extern struct rb_node *rb_left_deep(const struct rb_node *node);
extern struct rb_node *rb_right_deep(const struct rb_node *node);

/* Middle iteration (Sequential) - find logical next and previous nodes */
extern struct rb_node *rb_first(const struct rb_root *root);
extern struct rb_node *rb_last(const struct rb_root *root);
extern struct rb_node *rb_prev(const struct rb_node *node);
extern struct rb_node *rb_next(const struct rb_node *node);

#define rb_first_entry(ptr, type, member) \
    rb_entry_safe(rb_first(ptr), type, member)

#define rb_next_entry(pos, member) \
    rb_entry_safe(rb_next(&(pos)->member), typeof(*(pos)), member)

#define rb_for_each_entry(pos, root, member)                    \
    for (pos = rb_first_entry(root, typeof(*pos), member);      \
         pos; pos = rb_next_entry(pos, member))

/* Postorder iteration (Depth-first) - always visit the parent after its children */
extern struct rb_node *rb_post_first(const struct rb_root *root);
extern struct rb_node *rb_post_next(const struct rb_node *node);

#define rb_post_first_entry(ptr, type, member) \
    rb_entry_safe(rb_post_first(ptr), type, member)

#define rb_post_next_entry(pos, member) \
    rb_entry_safe(rb_post_next(&(pos)->member), typeof(*(pos)), member)

#define rb_post_for_each_entry(pos, root, member)               \
    for (pos = rb_post_first_entry(root, typeof(*pos), member); \
         pos; pos = rb_post_next_entry(pos, member))

#define rb_post_for_each_entry_safe(pos, next, root, member)    \
    for (pos = rb_post_first_entry(root, typeof(*pos), member); \
         pos && ({ next = rb_post_next_entry(pos, member);      \
         1; }); pos = next)

/**
 * rb_link - link node to parent.
 * @parent: point to parent node.
 * @link: point to pointer to child node.
 * @node: new node to link.
 */
static inline void rb_link(struct rb_node *parent, struct rb_node **link, struct rb_node *node)
{
#ifdef DEBUG_RBTREE
    if (unlikely(!rb_debug_link_check(parent, link, node)))
        return;
#endif

    /* link = &parent->left/right */
    *link = node;
    node->parent = parent;
    node->color = RB_RED;
    node->left = node->right = NULL;
}

/**
 * rb_insert_node - link node to parent and fixup rbtree.
 * @root: rbtree root of node.
 * @parent: parent node of node.
 * @link: point to pointer to child node.
 * @node: new node to link.
 */
static inline void rb_insert_node(struct rb_root *root, struct rb_node *parent,
                                  struct rb_node **link, struct rb_node *node)
{
    rb_link(parent, link, node);
    rb_fixup(root, node);
}

/**
 * rb_insert - find the parent node and insert new node.
 * @root: rbtree root of node.
 * @node: new node to insert.
 * @cmp: operator defining the node order.
 */
static inline void rb_insert(struct rb_root *root, struct rb_node *node, rb_cmp_t cmp)
{
    struct rb_node *parent, **link;

    link = rb_parent(root, &parent, node, cmp, NULL);
    rb_insert_node(root, parent, link, node);
}

/**
 * rb_insert_conflict - find the parent node and insert new node or conflict.
 * @root: rbtree root of node.
 * @node: new node to insert.
 * @cmp: operator defining the node order.
 */
static inline int rb_insert_conflict(struct rb_root *root, struct rb_node *node, rb_cmp_t cmp)
{
    struct rb_node *parent, **link;

    link = rb_parent_conflict(root, &parent, node, cmp, NULL);
    if (!link)
        return -EFAULT;

    rb_insert_node(root, parent, link, node);
    return 0;
}

/**
 * rb_delete - delete node and fixup rbtree.
 * @root: rbtree root of node.
 * @node: node to delete.
 */
static inline void rb_delete(struct rb_root *root, struct rb_node *node)
{
    struct rb_node *rebalance;

#ifdef DEBUG_RBTREE
    if (unlikely(!rb_debug_delete_check(node)))
        return;
#endif

    if ((rebalance = rb_remove(root, node)))
        rb_erase(root, rebalance);

    node->left = POISON_RBNODE1;
    node->right = POISON_RBNODE2;
    node->parent = POISON_RBNODE3;
}

/* Same as rb_first(), but O(1) */
#define rb_cached_first(cached) (cached)->rb_leftmost

#define rb_cached_first_entry(ptr, type, member) \
    rb_entry_safe(rb_cached_first(ptr), type, member)

#define rb_cached_for_each_entry(pos, cached, member) \
    for (pos = rb_cached_first_entry(cached, typeof(*pos), member); \
         pos; pos = rb_next_entry(pos, member))

#define rb_cached_post_for_each_entry(pos, cached, member) \
    rb_post_for_each_entry(pos, &(cached)->root, member)

#define rb_cached_post_for_each_entry_safe(pos, next, cached, member) \
    rb_post_for_each_entry_safe(pos, next, &(cached)->root, member)

/**
 * rb_cached_fixup - balance after insert cached node.
 * @cached: rbtree cached root of node.
 * @node: new inserted node.
 * @leftmost: is it the leftmost node.
 */
static inline void rb_cached_fixup(struct rb_root_cached *cached,
                                   struct rb_node *node, bool leftmost)
{
    if (leftmost)
        cached->rb_leftmost = node;

    rb_fixup(&cached->root, node);
}

/**
 * rb_cached_insert_node - link cached node to parent and fixup rbtree.
 * @cached: rbtree cached root of node.
 * @parent: parent node of node.
 * @link: point to pointer to child node.
 * @node: new node to link.
 * @leftmost: is it the leftmost node.
 */
static inline void rb_cached_insert_node(struct rb_root_cached *cached, struct rb_node *parent,
                                         struct rb_node **link, struct rb_node *node, bool leftmost)
{
    rb_link(parent, link, node);
    rb_cached_fixup(cached, node, leftmost);
}

/**
 * rb_cached_insert - find the parent node and insert new cached node.
 * @cached: rbtree cached root of node.
 * @node: new node to insert.
 * @cmp: operator defining the node order.
 */
static inline void rb_cached_insert(struct rb_root_cached *cached, struct rb_node *node, rb_cmp_t cmp)
{
    struct rb_node *parent, **link;
    bool leftmost = true;

    link = rb_parent(&cached->root, &parent, node, cmp, &leftmost);
    rb_cached_insert_node(cached, parent, link, node, leftmost);
}

/**
 * rb_cached_insert_conflict - find the parent node and insert new cached node or conflict.
 * @cached: rbtree cached root of node.
 * @node: new node to insert.
 * @cmp: operator defining the node order.
 */
static inline int rb_cached_insert_conflict(struct rb_root_cached *cached, struct rb_node *node, rb_cmp_t cmp)
{
    struct rb_node *parent, **link;
    bool leftmost = true;

    link = rb_parent_conflict(&cached->root, &parent, node, cmp, &leftmost);
    if (!link)
        return -EFAULT;

    rb_cached_insert_node(cached, parent, link, node, leftmost);
    return 0;
}

/**
 * rb_cached_delete - delete cached node and fixup rbtree.
 * @cached: rbtree cached root of node.
 * @node: node to delete.
 */
static inline struct rb_node *rb_cached_delete(struct rb_root_cached *cached, struct rb_node *node)
{
    struct rb_node *leftmost = NULL;

    if (cached->rb_leftmost == node)
        leftmost = cached->rb_leftmost = rb_next(node);

    rb_delete(&cached->root, node);
    return leftmost;
}

/**
 * rb_cached_replace - replace old cached node by new cached one.
 * @root: rbtree root of node.
 * @old: node to be replaced.
 * @new: new node to insert.
 */
static inline void rb_cached_replace(struct rb_root_cached *cached, struct rb_node *old, struct rb_node *new)
{
    if (cached->rb_leftmost == old)
        cached->rb_leftmost = new;

    rb_replace(&cached->root, old, new);
}

#endif  /* _RBTREE_H_ */
