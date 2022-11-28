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
    struct rb_node *node;
};

struct rb_root_cached {
    struct rb_root root;
    struct rb_node *leftmost;
};

struct rb_callbacks {
    void (*rotate)(struct rb_node *node, struct rb_node *successor);
    void (*copy)(struct rb_node *node, struct rb_node *successor);
    void (*propagate)(struct rb_node *node, struct rb_node *stop);
};

#define RB_STATIC \
    {NULL}

#define RB_CACHED_STATIC \
    {{NULL}, NULL}

#define RB_INIT \
    (struct rb_root) RB_STATIC

#define RB_CACHED_INIT \
    (struct rb_root_cached) RB_CACHED_STATIC

#define RB_ROOT(name) \
    struct rb_root name = RB_INIT

#define RB_ROOT_CACHED(name) \
    struct rb_root_cached name = RB_CACHED_INIT

#define RB_EMPTY_ROOT(root) \
    ((root)->node == NULL)

#define RB_EMPTY_ROOT_CACHED(cached) \
    ((cached)->root.node == NULL)

#define RB_EMPTY_NODE(node) \
    ((node)->parent == (node))

#define RB_CLEAR_NODE(node) \
    ((node)->parent = (node))

/**
 * rb_entry - get the struct for this entry.
 * @ptr: the &struct rb_node pointer.
 * @type: the type of the struct this is embedded in.
 * @member: the name of the rb_node within the struct.
 */
#define rb_entry(ptr, type, member) ({                  \
    const typeof(((type *)0)->member) *__mptr = (ptr);  \
    (type *)((char *)__mptr - offsetof(type,member));   \
})

/**
 * rb_entry_safe - get the struct for this entry or null.
 * @ptr: the &struct rb_node pointer.
 * @type: the type of the struct this is embedded in.
 * @member: the name of the rb_node within the struct.
 */
#define rb_entry_safe(ptr, type, member) ({             \
    typeof(ptr) _ptr = (ptr);                           \
    _ptr ? rb_entry(_ptr, type, member) : NULL;         \
})

#ifndef likely
# define likely(x) __builtin_expect(!!(x), 1)
# define unlikely(x) __builtin_expect(!!(x), 0)
#endif

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

typedef long (*rb_find_t)(const struct rb_node *node, const void *key);
typedef long (*rb_cmp_t)(const struct rb_node *nodea, const struct rb_node *nodeb);

extern void rb_fixup_augmented(struct rb_root *root, struct rb_node *node, const struct rb_callbacks *callbacks);
extern void rb_erase_augmented(struct rb_root *root, struct rb_node *parent, const struct rb_callbacks *callbacks);
extern struct rb_node *rb_remove_augmented(struct rb_root *root, struct rb_node *node, const struct rb_callbacks *callbacks);
extern void rb_fixup(struct rb_root *root, struct rb_node *node);
extern void rb_erase(struct rb_root *root, struct rb_node *parent);
extern struct rb_node *rb_remove(struct rb_root *root, struct rb_node *node);
extern void rb_replace(struct rb_root *root, struct rb_node *old, struct rb_node *new);
extern struct rb_node *rb_find(const struct rb_root *root, const void *key, rb_find_t cmp);
extern struct rb_node *rb_find_last(struct rb_root *root, const void *key, rb_find_t cmp, struct rb_node **parentp, struct rb_node ***linkp);
extern struct rb_node **rb_parent(struct rb_root *root, struct rb_node **parentp, struct rb_node *node, rb_cmp_t cmp, bool *leftmost);
extern struct rb_node **rb_parent_conflict(struct rb_root *root, struct rb_node **parentp, struct rb_node *node, rb_cmp_t cmp, bool *leftmost);

#define rb_cached_erase_augmented(cached, parent, callbacks) rb_erase_augmented(&(cached)->root, parent, callbacks)
#define rb_cached_remove_augmented(cached, node, callbacks) rb_remove_augmented(&(cached)->root, node, callbacks)
#define rb_cached_erase(cached, parent) rb_erase(&(cached)->root, parent)
#define rb_cached_remove(cached, node) rb_remove(&(cached)->root, node)
#define rb_cached_find(cached, key, cmp) rb_find(&(cached)->root, key, cmp)
#define rb_cached_find_last(cached, key, cmp, parentp, linkp) rb_find_last(&(cached)->root, key, cmp, parentp, linkp)
#define rb_cached_parent(cached, parentp, node, cmp, leftmost) rb_parent(&(cached)->root, parentp, node, cmp, leftmost)
#define rb_cached_parent_conflict(cached, parentp, node, cmp, leftmost) rb_parent_conflict(&(cached)->root, parentp, node, cmp, leftmost)

extern struct rb_node *rb_left_far(const struct rb_node *node);
extern struct rb_node *rb_right_far(const struct rb_node *node);
extern struct rb_node *rb_left_deep(const struct rb_node *node);
extern struct rb_node *rb_right_deep(const struct rb_node *node);

/* Inorder iteration (Sequential) - find logical next and previous nodes */
extern struct rb_node *rb_first(const struct rb_root *root);
extern struct rb_node *rb_last(const struct rb_root *root);
extern struct rb_node *rb_prev(const struct rb_node *node);
extern struct rb_node *rb_next(const struct rb_node *node);

/**
 * rb_first_entry - get the first element from a rbtree.
 * @ptr: the rbtree root to take the element from.
 * @type: the type of the struct this is embedded in.
 * @member: the name of the rb_node within the struct.
 */
#define rb_first_entry(ptr, type, member) \
    rb_entry_safe(rb_first(ptr), type, member)

/**
 * rb_last_entry - get the last element from a rbtree.
 * @ptr: the rbtree root to take the element from.
 * @type: the type of the struct this is embedded in.
 * @member: the name of the rb_node within the struct.
 */
#define rb_last_entry(ptr, type, member) \
    rb_entry_safe(rb_last(ptr), type, member)

/**
 * rb_next_entry - get the next element in rbtree.
 * @pos: the type * to cursor.
 * @member: the name of the rb_node within the struct.
 */
#define rb_next_entry(pos, member) \
    rb_entry_safe(rb_next(&(pos)->member), typeof(*(pos)), member)

/**
 * rb_prev_entry - get the prev element in rbtree.
 * @pos: the type * to cursor.
 * @member: the name of the rb_node within the struct.
 */
#define rb_prev_entry(pos, member) \
    rb_entry_safe(rb_prev(&(pos)->member), typeof(*(pos)), member)

/**
 * rb_for_each - iterate over a rbtree.
 * @pos: the &struct rb_node to use as a loop cursor.
 * @root: the root for your rbtree.
 */
#define rb_for_each(pos, root) \
    for (pos = rb_first(root); pos; pos = rb_next(pos))

/**
 * rb_for_each_reverse - iterate over a rbtree backwards.
 * @pos: the &struct rb_node to use as a loop cursor.
 * @root: the root for your rbtree.
 */
#define rb_for_each_reverse(pos, root) \
    for (pos = rb_last(root); pos; pos = rb_prev(pos))

/**
 * rb_for_each_from - iterate over a rbtree from the current point.
 * @pos: the &struct rb_node to use as a loop cursor.
 */
#define rb_for_each_from(pos) \
    for (; pos; pos = rb_next(pos))

/**
 * rb_for_each_reverse_from - iterate over a rbtree backwards from the current point.
 * @pos: the &struct rb_node to use as a loop cursor.
 */
#define rb_for_each_reverse_from(pos) \
    for (; pos; pos = rb_prev(pos))

/**
 * rb_for_each_continue - continue iteration over a rbtree.
 * @pos: the &struct rb_node to use as a loop cursor.
 */
#define rb_for_each_continue(pos) \
    for (pos = rb_next(pos); pos; pos = rb_next(pos))

/**
 * rb_for_each_reverse_continue - continue iteration over a rbtree backwards.
 * @pos: the &struct rb_node to use as a loop cursor.
 */
#define rb_for_each_reverse_continue(pos) \
    for (pos = rb_prev(pos); pos; pos = rb_prev(pos))

/**
 * rb_for_each_entry - iterate over rbtree of given type.
 * @pos: the type * to use as a loop cursor.
 * @root: the root for your rbtree.
 * @member: the name of the rb_node within the struct.
 */
#define rb_for_each_entry(pos, root, member) \
    for (pos = rb_first_entry(root, typeof(*pos), member); \
         pos; pos = rb_next_entry(pos, member))

/**
 * rb_for_each_entry_reverse - iterate backwards over rbtree of given type.
 * @pos: the type * to use as a loop cursor.
 * @root: the root for your rbtree.
 * @member: the name of the rb_node within the struct.
 */
#define rb_for_each_entry_reverse(pos, root, member) \
    for (pos = rb_last_entry(root, typeof(*pos), member); \
         pos; pos = rb_prev_entry(pos, member))

/**
 * rb_for_each_entry_from - iterate over rbtree of given type from the current point.
 * @pos: the type * to use as a loop cursor.
 * @member: the name of the rb_node within the struct.
 */
#define rb_for_each_entry_from(pos, member) \
    for (; pos; pos = rb_next_entry(pos, member))

/**
 * rb_for_each_entry_reverse_from - iterate backwards over rbtree of given type from the current point.
 * @pos: the type * to use as a loop cursor.
 * @member: the name of the rb_node within the struct.
 */
#define rb_for_each_entry_reverse_from(pos, member) \
    for (; pos; pos = rb_prev_entry(pos, member))

/**
 * rb_for_each_entry_continue - continue iteration over rbtree of given type.
 * @pos: the type * to use as a loop cursor.
 * @member: the name of the rb_node within the struct.
 */
#define rb_for_each_entry_continue(pos, member) \
    for (pos = rb_next_entry(pos, member); pos; \
         pos = rb_next_entry(pos, member))

/**
 * rb_for_each_entry_reverse_continue - iterate backwards from the given point.
 * @pos: the type * to use as a loop cursor.
 * @member: the name of the rb_node within the struct.
 */
#define rb_for_each_entry_reverse_continue(pos, member) \
    for (pos = rb_prev_entry(pos, member); pos; \
         pos = rb_prev_entry(pos, member))

/* Preorder iteration (Root-first) - always access the left node first */
extern struct rb_node *rb_pre_first(const struct rb_root *root);
extern struct rb_node *rb_pre_next(const struct rb_node *node);

/**
 * rb_pre_first_entry - get the preorder first element from a rbtree.
 * @ptr: the rbtree root to take the element from.
 * @type: the type of the struct this is embedded in.
 * @member: the name of the rb_node within the struct.
 */
#define rb_pre_first_entry(root, type, member) \
    rb_entry_safe(rb_pre_first(root), type, member)

/**
 * rb_pre_next_entry - get the preorder next element in rbtree.
 * @pos: the type * to cursor.
 * @member: the name of the rb_node within the struct.
 */
#define rb_pre_next_entry(pos, member) \
    rb_entry_safe(rb_pre_next(&(pos)->member), typeof(*(pos)), member)

/**
 * rb_pre_for_each - preorder iterate over a rbtree.
 * @pos: the &struct rb_node to use as a loop cursor.
 * @root: the root for your rbtree.
 */
#define rb_pre_for_each(pos, root) \
    for (pos = rb_pre_first(root); pos; pos = rb_pre_next(pos))

/**
 * rb_pre_for_each_from - preorder iterate over a rbtree from the current point.
 * @pos: the &struct rb_node to use as a loop cursor.
 */
#define rb_pre_for_each_from(pos) \
    for (; pos; pos = rb_pre_next(pos))

/**
 * rb_pre_for_each_continue - continue preorder iteration over a rbtree.
 * @pos: the &struct rb_node to use as a loop cursor.
 */
#define rb_pre_for_each_continue(pos) \
    for (pos = rb_pre_next(pos); pos; pos = rb_pre_next(pos))

/**
 * rb_pre_for_each_entry - preorder iterate over rbtree of given type.
 * @pos: the type * to use as a loop cursor.
 * @root: the root for your rbtree.
 * @member: the name of the rb_node within the struct.
 */
#define rb_pre_for_each_entry(pos, root, member) \
    for (pos = rb_pre_first_entry(root, typeof(*pos), member); \
         pos; pos = rb_pre_next_entry(pos, member))

/**
 * rb_pre_for_each_entry_from - preorder iterate over rbtree of given type from the current point.
 * @pos: the type * to use as a loop cursor.
 * @member: the name of the rb_node within the struct.
 */
#define rb_pre_for_each_entry_from(pos, member) \
    for (; pos; pos = rb_pre_next_entry(pos, member))

/**
 * rb_pre_for_each_entry_continue - continue preorder iteration over rbtree of given type.
 * @pos: the type * to use as a loop cursor.
 * @member: the name of the rb_node within the struct.
 */
#define rb_pre_for_each_entry_continue(pos, member) \
    for (pos = rb_pre_next_entry(pos, member); \
         pos; pos = rb_pre_next_entry(pos, member))

/* Postorder iteration (Depth-first) - always visit the parent after its children */
extern struct rb_node *rb_post_first(const struct rb_root *root);
extern struct rb_node *rb_post_next(const struct rb_node *node);

/**
 * rb_post_first_entry - get the postorder first element from a rbtree.
 * @ptr: the rbtree root to take the element from.
 * @type: the type of the struct this is embedded in.
 * @member: the name of the rb_node within the struct.
 */
#define rb_post_first_entry(ptr, type, member) \
    rb_entry_safe(rb_post_first(ptr), type, member)

/**
 * rb_post_next_entry - get the postorder next element in rbtree.
 * @pos: the type * to cursor.
 * @member: the name of the rb_node within the struct.
 */
#define rb_post_next_entry(pos, member) \
    rb_entry_safe(rb_post_next(&(pos)->member), typeof(*(pos)), member)

/**
 * rb_post_for_each - postorder iterate over a rbtree.
 * @pos: the &struct rb_node to use as a loop cursor.
 * @root: the root for your rbtree.
 */
#define rb_post_for_each(pos, root) \
    for (pos = rb_post_first(root); pos; pos = rb_post_next(pos))

/**
 * rb_post_for_each_from - postorder iterate over a rbtree from the current point.
 * @pos: the &struct rb_node to use as a loop cursor.
 */
#define rb_post_for_each_from(pos) \
    for (; pos; pos = rb_post_next(pos))

/**
 * rb_post_for_each_continue - continue postorder iteration over a rbtree.
 * @pos: the &struct rb_node to use as a loop cursor.
 */
#define rb_post_for_each_continue(pos) \
    for (pos = rb_post_next(pos); pos; pos = rb_post_next(pos))

/**
 * rb_post_for_each_safe - postorder iterate over a rbtree safe against removal of rbtree entry.
 * @pos: the &struct rb_node to use as a loop cursor.
 * @tmp: another rb_node to use as temporary storage.
 * @root: the root for your rbtree.
 */
#define rb_post_for_each_safe(pos, tmp, root) \
    for (pos = rb_post_first(root); pos && \
        ({tmp = rb_post_next(pos); 1; }); pos = tmp)

/**
 * rb_post_for_each_safe_from - postorder iterate over a rbtree safe against removal of rbtree entry from the current point.
 * @pos: the &struct rb_node to use as a loop cursor.
 * @tmp: another rb_node to use as temporary storage.
 */
#define rb_post_for_each_safe_from(pos, tmp) \
    for (; pos && ({tmp = rb_post_next(pos); 1; }); pos = tmp)

/**
 * rb_post_for_each_safe_continue - continue rbtree postorder iteration safe against removal.
 * @pos: the &struct rb_node to use as a loop cursor.
 * @tmp: another rb_node to use as temporary storage.
 */
#define rb_post_for_each_safe_continue(pos, tmp) \
    for (pos = rb_post_next(pos); pos && \
        ({tmp = rb_post_next(pos); 1; }); pos = tmp)

/**
 * rb_post_for_each_entry - postorder iterate over rbtree of given type.
 * @pos: the type * to use as a loop cursor.
 * @root: the root for your rbtree.
 * @member: the name of the rb_node within the struct.
 */
#define rb_post_for_each_entry(pos, root, member) \
    for (pos = rb_post_first_entry(root, typeof(*pos), member); \
         pos; pos = rb_post_next_entry(pos, member))

/**
 * rb_post_for_each_entry_from - postorder iterate over rbtree of given type from the current point.
 * @pos: the type * to use as a loop cursor.
 * @member: the name of the rb_node within the struct.
 */
#define rb_post_for_each_entry_from(pos, member) \
    for (; pos; pos = rb_post_next_entry(pos, member))

/**
 * rb_post_for_each_entry_continue - continue postorder iteration over rbtree of given type.
 * @pos: the type * to use as a loop cursor.
 * @member: the name of the rb_node within the struct.
 */
#define rb_post_for_each_entry_continue(pos, member) \
    for (pos = rb_post_next_entry(pos, member); \
         pos; pos = rb_post_next_entry(pos, member))

/**
 * rb_post_for_each_entry_safe - postorder iterate over rbtree of given type safe against removal of rbtree entry.
 * @pos: the type * to use as a loop cursor.
 * @tmp: another type * to use as temporary storage.
 * @root: the root for your rbtree.
 * @member: the name of the rb_node within the struct.
 */
#define rb_post_for_each_entry_safe(pos, tmp, root, member) \
    for (pos = rb_post_first_entry(root, typeof(*pos), member); \
         pos && ({ tmp = rb_post_next_entry(pos, member); \
         1; }); pos = tmp)

/**
 * rb_post_for_each_entry_from_safe - postorder iterate over rbtree from current point safe against removal.
 * @pos: the type * to use as a loop cursor.
 * @tmp: another type * to use as temporary storage.
 * @member: the name of the rb_node within the struct.
 */
#define rb_post_for_each_entry_from_safe(pos, tmp, member) \
    for (; pos && ({ tmp = rb_post_next_entry(pos, member); \
         1; }); pos = tmp)

/**
 * rb_post_for_each_entry_continue_safe - continue postorder rbtree iteration safe against removal.
 * @pos: the type * to use as a loop cursor.
 * @tmp: another type * to use as temporary storage.
 * @member: the name of the rb_node within the struct.
 */
#define rb_post_for_each_entry_continue_safe(pos, tmp, member) \
    for (pos = rb_post_next_entry(pos, member); \
         pos && ({ tmp = rb_post_next_entry(pos, member); \
         1; }); pos = tmp)

/* Levelorder iteration (Shallow-first) - always visit the shallow after deep. */
extern struct rb_node *rb_level_first(const struct rb_root *root, unsigned long *level);
extern struct rb_node *rb_level_next(const struct rb_root *root, unsigned long *level);

/**
 * rb_level_first_entry - get the levelorder first element from a rbtree.
 * @ptr: the rbtree root to take the element from.
 * @index: iteration counter.
 * @type: the type of the struct this is embedded in.
 * @member: the name of the rb_node within the struct.
 */
#define rb_level_first_entry(root, index, type, member) \
    rb_entry_safe(rb_level_first(root, index), type, member)

/**
 * rb_level_next_entry - get the levelorder next element in rbtree.
 * @pos: the type * to cursor.
 * @index: iteration counter.
 * @type: the type of the struct this is embedded in.
 * @member: the name of the rb_node within the struct.
 */
#define rb_level_next_entry(root, index, type, member) \
    rb_entry_safe(rb_level_next(root, index), type, member)

/**
 * rb_level_for_each - levelorder iterate over a rbtree.
 * @pos: the &struct rb_node to use as a loop cursor.
 * @index: iteration counter.
 * @root: the root for your rbtree.
 */
#define rb_level_for_each(pos, index, root) \
    for (pos = rb_level_first(root, index); pos; pos = rb_level_next(root, index))

/**
 * rb_level_for_each_from - levelorder iterate over a rbtree from the current point.
 * @pos: the &struct rb_node to use as a loop cursor.
 * @index: iteration counter.
 * @root: the root for your rbtree.
 */
#define rb_level_for_each_from(pos, index, root) \
    for (; pos; pos = rb_level_next(root, index))

/**
 * rb_level_for_each_continue - continue levelorder iteration over a rbtree.
 * @pos: the &struct rb_node to use as a loop cursor.
 * @index: iteration counter.
 * @root: the root for your rbtree.
 */
#define rb_level_for_each_continue(pos, index, root) \
    for (pos = rb_level_next(root, index); pos; pos = rb_level_next(root, index))

/**
 * rb_level_for_each_entry - levelorder iterate over rbtree of given type.
 * @pos: the type * to use as a loop cursor.
 * @index: iteration counter.
 * @root: the root for your rbtree.
 * @member: the name of the rb_node within the struct.
 */
#define rb_level_for_each_entry(pos, index, root, member) \
    for (pos = rb_level_first_entry(root, index, typeof(*pos), member); \
         pos; pos = rb_level_next_entry(root, index, typeof(*pos), member))

/**
 * rb_level_for_each_entry_from - levelorder iterate over rbtree of given type from the current point.
 * @pos: the type * to use as a loop cursor.
 * @index: iteration counter.
 * @root: the root for your rbtree.
 * @member: the name of the rb_node within the struct.
 */
#define rb_level_for_each_entry_from(pos, index, root, member) \
    for (; pos; pos = rb_level_next_entry(root, index, typeof(*pos), member))

/**
 * rb_level_for_each_entry_continue - continue levelorder iteration over rbtree of given type.
 * @pos: the type * to use as a loop cursor.
 * @index: iteration counter.
 * @root: the root for your rbtree.
 * @member: the name of the rb_node within the struct.
 */
#define rb_level_for_each_entry_continue(pos, index, root, member) \
    for (pos = rb_level_next_entry(root, index, typeof(*pos), member); \
         pos; pos = rb_level_next_entry(root, index, typeof(*pos), member))

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

/**
 * rb_insert_node_augmented - augmented link node to parent and fixup rbtree.
 * @root: rbtree root of node.
 * @parent: parent node of node.
 * @link: point to pointer to child node.
 * @node: new node to link.
 * @callbacks: augmented callback function.
 */
static inline void rb_insert_node_augmented(struct rb_root *root, struct rb_node *parent,
                                            struct rb_node **link, struct rb_node *node,
                                            const struct rb_callbacks *callbacks)
{
    rb_link(parent, link, node);
    rb_fixup_augmented(root, node, callbacks);
}

/**
 * rb_insert_augmented - augmented find the parent node and insert new node.
 * @root: rbtree root of node.
 * @node: new node to insert.
 * @cmp: operator defining the node order.
 * @callbacks: augmented callback function.
 */
static inline void rb_insert_augmented(struct rb_root *root, struct rb_node *node, rb_cmp_t cmp,
                                       const struct rb_callbacks *callbacks)
{
    struct rb_node *parent, **link;

    link = rb_parent(root, &parent, node, cmp, NULL);
    rb_insert_node_augmented(root, parent, link, node, callbacks);
}

/**
 * rb_insert_conflict_augmented - augmented find the parent node and insert new node or conflict.
 * @root: rbtree root of node.
 * @node: new node to insert.
 * @cmp: operator defining the node order.
 * @callbacks: augmented callback function.
 */
static inline int rb_insert_conflict_augmented(struct rb_root *root, struct rb_node *node, rb_cmp_t cmp,
                                                 const struct rb_callbacks *callbacks)
{
    struct rb_node *parent, **link;

    link = rb_parent_conflict(root, &parent, node, cmp, NULL);
    if (!link)
        return -EFAULT;

    rb_insert_node_augmented(root, parent, link, node, callbacks);
    return 0;
}

/**
 * rb_delete_augmented - augmented delete node and fixup rbtree.
 * @root: rbtree root of node.
 * @node: node to delete.
 * @callbacks: augmented callback function.
 */
static inline void rb_delete_augmented(struct rb_root *root, struct rb_node *node,
                                       const struct rb_callbacks *callbacks)
{
    struct rb_node *rebalance;

#ifdef DEBUG_RBTREE
    if (unlikely(!rb_debug_delete_check(node)))
        return;
#endif

    if ((rebalance = rb_remove_augmented(root, node, callbacks)))
        rb_erase_augmented(root, rebalance, callbacks);

    node->left = POISON_RBNODE1;
    node->right = POISON_RBNODE2;
    node->parent = POISON_RBNODE3;
}

/**
 * rb_cached_first - get the first rb_node from a cached rbtree.
 * @cached: the rbtree root to take the rb_node from.
 */
#define rb_cached_first(cached) \
    ((cached)->leftmost)

/**
 * rb_cached_first_entry - get the first element from a cached rbtree.
 * @ptr: the rbtree root to take the element from.
 * @type: the type of the struct this is embedded in.
 * @member: the name of the rb_node within the struct.
 */
#define rb_cached_first_entry(ptr, type, member) \
    rb_entry_safe(rb_cached_first(ptr), type, member)

/**
 * rb_cached_for_each - iterate over a cached rbtree.
 * @pos: the &struct rb_node to use as a loop cursor.
 * @cached: the cached root for your rbtree.
 */
#define rb_cached_for_each(pos, cached) \
    for (pos = rb_cached_first(cached); pos; pos = rb_next(pos))

/**
 * rb_cached_for_each_reverse - iterate over a cached rbtree backwards.
 * @pos: the &struct rb_node to use as a loop cursor.
 * @cached: the cached root for your rbtree.
 */
#define rb_cached_for_each_reverse(pos, cached) \
    rb_for_each_reverse(pos, &(cached)->root)

/**
 * rb_cached_for_each_entry - iterate over cached rbtree of given type.
 * @pos: the type * to use as a loop cursor.
 * @cached: the cached root for your rbtree.
 * @member: the name of the rb_node within the struct.
 */
#define rb_cached_for_each_entry(pos, cached, member) \
    for (pos = rb_cached_first_entry(cached, typeof(*pos), member); \
         pos; pos = rb_next_entry(pos, member))

/**
 * rb_cached_for_each_entry_reverse - iterate backwards over cached rbtree of given type.
 * @pos: the type * to use as a loop cursor.
 * @cached: the cached root for your rbtree.
 * @member: the name of the rb_node within the struct.
 */
#define rb_cached_for_each_entry_reverse(pos, cached, member) \
    rb_for_each_entry_reverse(pos, &(cached)->root, member)

/**
 * rb_cached_pre_for_each - preorder iterate over a cached rbtree.
 * @pos: the &struct rb_node to use as a loop cursor.
 * @cached: the cached root for your rbtree.
 */
#define rb_cached_pre_for_each(pos, cached) \
    rb_pre_for_each(pos, &(cached)->root)

/**
 * rb_cached_pre_for_each_entry - preorder iterate over cached rbtree of given type.
 * @pos: the type * to use as a loop cursor.
 * @cached: the cached root for your rbtree.
 * @member: the name of the rb_node within the struct.
 */
#define rb_cached_pre_for_each_entry(pos, cached, member) \
    rb_pre_for_each_entry(pos, &(cached)->root, member)

/**
 * rb_cached_post_for_each - postorder iterate over a cached rbtree.
 * @pos: the &struct rb_node to use as a loop cursor.
 * @cached: the cached root for your rbtree.
 */
#define rb_cached_post_for_each(pos, cached) \
    rb_post_for_each(pos, &(cached)->root)

/**
 * rb_cached_post_for_each_safe - postorder iterate over a cached rbtree safe against removal of rbtree entry.
 * @pos: the &struct rb_node to use as a loop cursor.
 * @tmp: another rb_node to use as temporary storage.
 * @cached: the cached root for your rbtree.
 */
#define rb_cached_post_for_each_safe(pos, tmp, cached) \
    rb_post_for_each_safe(pos, tmp, &(cached)->root)

/**
 * rb_cached_post_for_each_entry - postorder iterate over cached rbtree of given type.
 * @pos: the type * to use as a loop cursor.
 * @cached: the cached root for your rbtree.
 * @member: the name of the rb_node within the struct.
 */
#define rb_cached_post_for_each_entry(pos, cached, member) \
    rb_post_for_each_entry(pos, &(cached)->root, member)

/**
 * rb_cached_post_for_each_entry_safe - postorder iterate over cached rbtree of given type safe against removal of rbtree entry.
 * @pos: the type * to use as a loop cursor.
 * @tmp: another type * to use as temporary storage.
 * @cached: the cached root for your rbtree.
 * @member: the name of the rb_node within the struct.
 */
#define rb_cached_post_for_each_entry_safe(pos, tmp, cached, member) \
    rb_post_for_each_entry_safe(pos, tmp, &(cached)->root, member)

/**
 * rb_cached_level_for_each - levelorder iterate over a cached rbtree.
 * @pos: the &struct rb_node to use as a loop cursor.
 * @index: iteration counter.
 * @cached: the cached root for your rbtree.
 */
#define rb_cached_level_for_each(pos, index, cached) \
    for (pos = rb_level_first(&(cached)->root, index); \
         pos; pos = rb_level_next(&(cached)->root, index))

/**
 * rb_cached_level_for_each_from - levelorder iterate over a cached rbtree from the current point.
 * @pos: the &struct rb_node to use as a loop cursor.
 * @index: iteration counter.
 * @cached: the cached root for your rbtree.
 */
#define rb_cached_level_for_each_from(pos, index, cached) \
    for (; pos; pos = rb_level_next(&(cached)->root, index))

/**
 * rb_cached_level_for_each_continue - continue levelorder iteration over a cached rbtree.
 * @pos: the &struct rb_node to use as a loop cursor.
 * @index: iteration counter.
 * @cached: the cached root for your rbtree.
 */
#define rb_cached_level_for_each_continue(pos, index, cached) \
    for (pos = rb_level_next(&(cached)->root, index); \
         pos; pos = rb_level_next(&(cached)->root, index))

/**
 * rb_cached_level_for_each_entry - levelorder iterate over rbtree of given type.
 * @pos: the type * to use as a loop cursor.
 * @index: iteration counter.
 * @cached: the cached root for your rbtree.
 * @member: the name of the rb_node within the struct.
 */
#define rb_cached_level_for_each_entry(pos, index, cached, member) \
    for (pos = rb_level_first_entry(&(cached)->root, index, typeof(*pos), member); \
         pos; pos = rb_level_next_entry(&(cached)->root, index, typeof(*pos), member))

/**
 * rb_cached_level_for_each_entry_from - levelorder iterate over rbtree of given type from the current point.
 * @pos: the type * to use as a loop cursor.
 * @index: iteration counter.
 * @cached: the cached root for your rbtree.
 * @member: the name of the rb_node within the struct.
 */
#define rb_cached_level_for_each_entry_from(pos, index, cached, member) \
    for (; pos; pos = rb_level_next_entry(&(cached)->root, index, typeof(*pos), member))

/**
 * rb_cached_level_for_each_entry_continue - continue levelorder iteration over rbtree of given type.
 * @pos: the type * to use as a loop cursor.
 * @index: iteration counter.
 * @cached: the cached root for your rbtree.
 * @member: the name of the rb_node within the struct.
 */
#define rb_cached_level_for_each_entry_continue(pos, index, cached, member) \
    for (pos = rb_level_next_entry(&(cached)->root, index, typeof(*pos), member); \
         pos; pos = rb_level_next_entry(&(cached)->root, index, typeof(*pos), member))

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
        cached->leftmost = node;

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

    link = rb_cached_parent(cached, &parent, node, cmp, &leftmost);
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

    link = rb_cached_parent_conflict(cached, &parent, node, cmp, &leftmost);
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

    if (cached->leftmost == node)
        leftmost = cached->leftmost = rb_next(node);

    rb_delete(&cached->root, node);
    return leftmost;
}

/**
 * rb_cached_fixup_augmented - augmented balance after insert cached node.
 * @cached: rbtree cached root of node.
 * @node: new inserted node.
 * @leftmost: is it the leftmost node.
 * @callbacks: augmented callback function.
 */
static inline void rb_cached_fixup_augmented(struct rb_root_cached *cached, struct rb_node *node,
                                             bool leftmost, const struct rb_callbacks *callbacks)
{
    if (leftmost)
        cached->leftmost = node;

    rb_fixup_augmented(&cached->root, node, callbacks);
}

/**
 * rb_cached_insert_node - augmented link cached node to parent and fixup rbtree.
 * @cached: rbtree cached root of node.
 * @parent: parent node of node.
 * @link: point to pointer to child node.
 * @node: new node to link.
 * @leftmost: is it the leftmost node.
 * @callbacks: augmented callback function.
 */
static inline void rb_cached_insert_node_augmented(struct rb_root_cached *cached, struct rb_node *parent,
                                                   struct rb_node **link, struct rb_node *node,
                                                   bool leftmost, const struct rb_callbacks *callbacks)
{
    rb_link(parent, link, node);
    rb_cached_fixup_augmented(cached, node, leftmost, callbacks);
}

/**
 * rb_cached_insert - augmented find the parent node and insert new cached node.
 * @cached: rbtree cached root of node.
 * @node: new node to insert.
 * @cmp: operator defining the node order.
 * @callbacks: augmented callback function.
 */
static inline void rb_cached_insert_augmented(struct rb_root_cached *cached, struct rb_node *node,
                                              rb_cmp_t cmp, const struct rb_callbacks *callbacks)
{
    struct rb_node *parent, **link;
    bool leftmost = true;

    link = rb_cached_parent(cached, &parent, node, cmp, &leftmost);
    rb_cached_insert_node_augmented(cached, parent, link, node, leftmost, callbacks);
}

/**
 * rb_cached_insert_conflict - find the parent node and insert new cached node or conflict.
 * @cached: rbtree cached root of node.
 * @node: new node to insert.
 * @cmp: operator defining the node order.
 * @callbacks: augmented callback function.
 */
static inline int rb_cached_insert_conflict_augmented(struct rb_root_cached *cached, struct rb_node *node,
                                                      rb_cmp_t cmp, const struct rb_callbacks *callbacks)
{
    struct rb_node *parent, **link;
    bool leftmost = true;

    link = rb_cached_parent_conflict(cached, &parent, node, cmp, &leftmost);
    if (!link)
        return -EFAULT;

    rb_cached_insert_node_augmented(cached, parent, link, node, leftmost, callbacks);
    return 0;
}

/**
 * rb_cached_delete - delete cached node and fixup rbtree.
 * @cached: rbtree cached root of node.
 * @node: node to delete.
 * @callbacks: augmented callback function.
 */
static inline struct rb_node *rb_cached_delete_augmented(struct rb_root_cached *cached, struct rb_node *node,
                                                         const struct rb_callbacks *callbacks)
{
    struct rb_node *leftmost = NULL;

    if (cached->leftmost == node)
        leftmost = cached->leftmost = rb_next(node);

    rb_delete_augmented(&cached->root, node, callbacks);
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
    if (cached->leftmost == old)
        cached->leftmost = new;

    rb_replace(&cached->root, old, new);
}

#define RB_DECLARE_CALLBACKS(RBSTATIC, RBNAME, RBSTRUCT, RBFIELD, RBAUGMENTED, RBCOMPUTE)   \
static void RBNAME##_rotate(struct rb_node *rb_node, struct rb_node *rb_successor)          \
{                                                                                           \
    RBSTRUCT *node = rb_entry(rb_node, RBSTRUCT, RBFIELD);                                  \
    RBSTRUCT *successor = rb_entry(rb_successor, RBSTRUCT, RBFIELD);                        \
    successor->RBAUGMENTED = node->RBAUGMENTED;                                             \
    RBCOMPUTE(node, false);                                                                 \
}                                                                                           \
                                                                                            \
static void RBNAME##_copy(struct rb_node *rb_node, struct rb_node *rb_successor)            \
{                                                                                           \
    RBSTRUCT *node = rb_entry(rb_node, RBSTRUCT, RBFIELD);                                  \
    RBSTRUCT *successor = rb_entry(rb_successor, RBSTRUCT, RBFIELD);                        \
    successor->RBAUGMENTED = node->RBAUGMENTED;                                             \
}                                                                                           \
                                                                                            \
static void RBNAME##_propagate(struct rb_node *rb_node, struct rb_node *rb_stop)            \
{                                                                                           \
    while (rb_node != rb_stop) {                                                            \
        RBSTRUCT *node = rb_entry(rb_node, RBSTRUCT, RBFIELD);                              \
        if (RBCOMPUTE(node, true))                                                          \
            break;                                                                          \
        rb_node = node->RBFIELD.parent;                                                     \
    }                                                                                       \
}                                                                                           \
                                                                                            \
RBSTATIC struct rb_callbacks RBNAME = {                                                     \
    .rotate = RBNAME##_rotate,                                                              \
    .copy = RBNAME##_copy,                                                                  \
    .propagate = RBNAME##_propagate,                                                        \
}

#define RB_DECLARE_CALLBACKS_MAX(RBSTATIC, RBNAME, RBSTRUCT, RBFIELD, RBTYPE, RBAUGMENTED, RBCOMPUTE)   \
static inline bool RBNAME##_compute_max(RBSTRUCT *node, bool exit)                                      \
{                                                                                                       \
    RBSTRUCT *child;                                                                                    \
    RBTYPE max = RBCOMPUTE(node);                                                                       \
    if (node->RBFIELD.left) {                                                                           \
        child = rb_entry(node->RBFIELD.left, RBSTRUCT, RBFIELD);                                        \
        if (child->RBAUGMENTED > max)                                                                   \
            max = child->RBAUGMENTED;                                                                   \
    }                                                                                                   \
    if (node->RBFIELD.right) {                                                                          \
        child = rb_entry(node->RBFIELD.right, RBSTRUCT, RBFIELD);                                       \
        if (child->RBAUGMENTED > max)                                                                   \
            max = child->RBAUGMENTED;                                                                   \
    }                                                                                                   \
    if (exit && node->RBAUGMENTED == max)                                                               \
        return true;                                                                                    \
    node->RBAUGMENTED = max;                                                                            \
    return false;                                                                                       \
}                                                                                                       \
RB_DECLARE_CALLBACKS(RBSTATIC, RBNAME, RBSTRUCT, RBFIELD, RBAUGMENTED, RBNAME##_compute_max)

#endif  /* _RBTREE_H_ */
