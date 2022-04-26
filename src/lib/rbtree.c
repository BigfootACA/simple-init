/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2021 Sanpe <sanpeqf@gmail.com>
 *          -- Based on Linux's rbtree :)
 */

#include "rbtree.h"

/**
 * child_change - replace old child by new one.
 * @root: rbtree root of node.
 * @parent: parent to change child.
 * @old: node to be replaced.
 * @new: new node to insert.
 */
static inline void
child_change(struct rb_root *root, struct rb_node *parent,
             struct rb_node *old, struct rb_node *new)
{
    if (!parent)
        root->rb_node = new;
    else if (parent->left == old)
        parent->left = new;
    else
        parent->right = new;
}

/**
 * rotate_set - replace old child by new one.
 * @root: rbtree root of node.
 * @node: parent to change child.
 * @new: node to be replaced.
 * @child: managed child node.
 * @color: color after rotation.
 * @ccolor: color of child.
 */
static inline void
rotate_set(struct rb_root *root, struct rb_node *node, struct rb_node *new,
           struct rb_node *child, unsigned int color, unsigned int ccolor)
{
    struct rb_node *parent = node->parent;

    new->parent = node->parent;
    node->parent = new;

    if (color != RB_NSET) {
        new->color = node->color;
        node->color = color;
    }

    if (child) {
        if (ccolor != RB_NSET)
            child->color = ccolor;
        child->parent = node;
    }

    child_change(root, parent, node, new);
}

/**
 * left_rotate - left rotation one node.
 * @root: rbtree root of node.
 * @node: node to rotation.
 * @color: color after rotation.
 * @ccolor: color of child.
 */
static inline struct rb_node *
left_rotate(struct rb_root *root, struct rb_node *node,
            unsigned int color, unsigned int ccolor)
{
    struct rb_node *child, *successor = node->right;

    /* change left child */
    child = node->right = successor->left;
    successor->left = node;

    rotate_set(root, node, successor, child, color, ccolor);
    return child;
}

/**
 * right_rotate - right rotation one node.
 * @root: rbtree root of node.
 * @node: node to rotation.
 * @color: color after rotation.
 * @ccolor: color of child.
 */
static inline struct rb_node *
right_rotate(struct rb_root *root, struct rb_node *node,
             unsigned int color, unsigned int ccolor)
{
    struct rb_node *child, *successor = node->left;

    /* change right child */
    child = node->left = successor->right;
    successor->right = node;

    rotate_set(root, node, successor, child, color, ccolor);
    return child;
}

/**
 * rb_fixup - balance after insert node.
 * @root: rbtree root of node.
 * @node: new inserted node.
 */
void rb_fixup(struct rb_root *root, struct rb_node *node)
{
    struct rb_node *parent, *gparent, *tmp;

    while (root && node) {
        parent = node->parent;

        /*
         * The inserted node is root. Either this is the
         * first node, or we recursed at Case 1 below and
         * are no longer violating 4).
         */

        if (unlikely(!parent)) {
            node->color = RB_BLACK;
            break;
        }

        /*
         * If there is a black parent, we are done.
         * Otherwise, take some corrective action as,
         * per 4), we don't want a red root or two
         * consecutive red nodes.
         */

        if (parent->color == RB_BLACK)
            break;

        gparent = parent->parent;
        tmp = gparent->right;

        if (tmp != parent) {
            /*
             * Case 1 - node's uncle is red (color flips).
             *
             *       G            g
             *      / \          / \
             *     p   t  -->   P   T
             *    /            /
             *   n            n
             *
             * However, since g's parent might be red, and
             * 4) does not allow this, we need to recurse
             * at g.
             */

            if (tmp && tmp->color == RB_RED) {
                parent->color = tmp->color = RB_BLACK;
                gparent->color = RB_RED;
                node = gparent;
                continue;
            }

            /*
             * Case 2 - node's uncle is black and node is
             * the parent's right child (left rotate at parent).
             *
             *      G             G
             *     / \           / \
             *    p   U  -->    n   U
             *     \           /
             *      n         p
             *     /           \
             *    c             C
             *
             * This still leaves us in violation of 4), the
             * continuation into Case 3 will fix that.
             */

            if (node == parent->right)
                left_rotate(root, parent, RB_NSET, RB_BLACK);

            /*
             * Case 3 - node's uncle is black and node is
             * the parent's left child (right rotate at gparent).
             *
             *        G           P
             *       / \         / \
             *      p   U  -->  n   g
             *     / \             / \
             *    n   s           S   U
             */

            right_rotate(root, gparent, RB_RED, RB_BLACK);
            break;
        } else {
            /* parent == gparent->right */
            tmp = gparent->left;

            /* Case 1 - color flips */
            if (tmp && tmp->color == RB_RED) {
                parent->color = tmp->color = RB_BLACK;
                gparent->color = RB_RED;
                node = gparent;
                continue;
            }

            /* Case 2 - right rotate at parent */
            if (node == parent->left)
                right_rotate(root, parent, RB_NSET, RB_BLACK);

            /* Case 3 - left rotate at gparent */
            left_rotate(root, gparent, RB_RED, RB_BLACK);
            break;
        }
    }
}

/**
 * rb_erase - balance after remove node.
 * @root: rbtree root of node.
 * @parent: parent of removed node.
 */
void rb_erase(struct rb_root *root, struct rb_node *parent)
{
    struct rb_node *tmp1, *tmp2, *sibling, *node = NULL;

    while (root && parent) {
        /*
         * Loop invariants:
         * - node is black (or NULL on first iteration)
         * - node is not the root (parent is not NULL)
         * - All leaf paths going through parent and node have a
         *   black node count that is 1 lower than other leaf paths.
         */

        sibling = parent->right;
        if (node != sibling) {
            /*
             * Case 1 - left rotate at parent
             *
             *     P               S
             *    / \             / \
             *   N   s    -->    p   Sr
             *      / \         / \
             *     Sl  Sr      N   Sl
             */

            if (sibling->color == RB_RED)
                sibling = left_rotate(root, parent, RB_RED, RB_BLACK);

            tmp2 = sibling->right;
            if (!tmp2 || tmp2->color == RB_BLACK) {
                tmp1 = sibling->left;

                /*
                 * Case 2 - sibling color flip
                 * (p could be either color here)
                 *
                 *    (p)           (p)
                 *    / \           / \
                 *   N   S    -->  N   s
                 *      / \           / \
                 *     Sl  Sr        Sl  Sr
                 *
                 * This leaves us violating 5) which
                 * can be fixed by flipping p to black
                 * if it was red, or by recursing at p.
                 * p is red when coming from Case 1.
                 */

                if (!tmp1 || tmp1->color == RB_BLACK) {
                    sibling->color = RB_RED;
                    if (parent->color == RB_RED)
                        parent->color = RB_BLACK;
                    else {
                        node = parent;
                        parent = node->parent;
                        if (parent)
                            continue;
                    }
                    break;
                }

                /*
                 * Case 3 - right rotate at sibling
                 * (p could be either color here)
                 *
                 *   (p)           (p)
                 *   / \           / \
                 *  N   S    -->  N   sl
                 *     / \             \
                 *    sl  Sr            S
                 *      \              / \
                 *       t            T   Sr
                 *
                 * Note: p might be red, and then both
                 * p and sl are red after rotation(which
                 * breaks property 4). This is fixed in
                 * Case 4 (in __rb_rotate_set_parents()
                 *         which set sl the color of p
                 *         and set p RB_BLACK)
                 *
                 *   (p)            (sl)
                 *   / \            /  \
                 *  N   sl   -->   P    S
                 *       \        /      \
                 *        S      N        Sr
                 *         \
                 *          Sr
                 */

                right_rotate(root, sibling, RB_NSET, RB_BLACK);
                tmp2 = sibling;
            }

            /*
             * Case 4 - left rotate at parent + color flips
             * (p and sl could be either color here.
             *  After rotation, p becomes black, s acquires
             *  p's color, and sl keeps its color)
             *
             *      (p)             (s)
             *      / \             / \
             *     N   S     -->   P   Sr
             *        / \         / \
             *      (sl) sr      N  (sl)
             */

            left_rotate(root, parent, RB_BLACK, RB_NSET);
            tmp2->color = RB_BLACK;
            break;
        } else {
            sibling = parent->left;

            /* Case 1 - right rotate at parent */
            if (sibling->color == RB_RED)
                sibling = right_rotate(root, parent, RB_RED, RB_BLACK);

            tmp1 = sibling->left;
            if (!tmp1 || tmp1->color == RB_BLACK) {
                tmp2 = sibling->right;

                /* Case 2 - sibling color flip */
                if (!tmp2 || tmp2->color == RB_BLACK) {
                    sibling->color = RB_RED;
                    if (parent->color == RB_RED)
                        parent->color = RB_BLACK;
                    else {
                        node = parent;
                        parent = node->parent;
                        if (parent)
                            continue;
                    }
                    break;
                }

                /* Case 3 - left rotate at sibling */
                left_rotate(root, sibling, RB_NSET, RB_BLACK);
                tmp1 = sibling;
            }

            /* Case 4 - right rotate at parent + color flips */
            right_rotate(root, parent, RB_BLACK, RB_NSET);
            tmp1->color = RB_BLACK;
            break;
        }
    }
}

/**
 * rb_remove - remove node form rbtree.
 * @root: rbtree root of node.
 * @node: node to remove.
 */
struct rb_node *rb_remove(struct rb_root *root, struct rb_node *node)
{
    struct rb_node *parent = node->parent, *rebalance = NULL;
    struct rb_node *child1 = node->left;
    struct rb_node *child2 = node->right;

    if (!child1 && !child2) {
        /*
         * Case 1: node to erase has no child.
         *
         *     (p)        (p)
         *     / \          \
         *   (n) (s)  ->    (s)
         *
         */

        if (node->color == RB_BLACK)
            rebalance = parent;
        child_change(root, parent, node, NULL);
    } else if (!child2) {
        /*
         * Case 1: node to erase only has left child.
         *
         *      (p)          (p)
         *      / \          / \
         *    (n) (s)  ->  (c) (s)
         *    /
         *  (c)
         *
         */

        child1->color = node->color;
        child1->parent = node->parent;
        child_change(root, parent, node, child1);
    } else if (!child1) {
        /*
         * Case 1: node to erase only has right child.
         *
         *    (p)          (p)
         *    / \          / \
         *  (n) (s)  ->  (c) (s)
         *    \
         *    (c)
         */

        child2->color = node->color;
        child2->parent = node->parent;
        child_change(root, parent, node, child2);
    } else { /* child1 && child2 */
        struct rb_node *tmp, *successor = child2;

        child1 = child2->left;
        if (!child1) {
            /*
             * Case 2: node's successor is its right child
             *
             *    (n)          (s)
             *    / \          / \
             *  (x) (s)  ->  (x) (c)
             *        \
             *        (c)
             */

            parent = successor;
            tmp = successor->right;
        } else {
            /*
             * Case 3: node's successor is leftmost under
             * node's right child subtree
             *
             *    (n)          (s)
             *    / \          / \
             *  (x) (y)  ->  (x) (y)
             *      /            /
             *    (p)          (p)
             *    /            /
             *  (s)          (c)
             *    \
             *    (c)
             */

            do {
                parent = successor;
                successor = child1;
                child1 = child1->left;
            } while (child1);

            tmp = successor->right;
            parent->left = tmp;
            successor->right = child2;
            child2->parent = successor;
        }

        child1 = node->left;
        successor->left = child1;
        child1->parent = successor;

        child1 = node->parent;
        child_change(root, child1, node, successor);

        if (tmp) {
            tmp->parent = parent;
            tmp->color = RB_BLACK;
        } else if (successor->color == RB_BLACK)
            rebalance = parent;

        successor->parent = child1;
        successor->color = node->color;
    }

    return rebalance;
}

/**
 * rb_replace - replace old node by new one.
 * @root: rbtree root of node.
 * @old: node to be replaced.
 * @new: new node to insert.
 */
void rb_replace(struct rb_root *root, struct rb_node *old, struct rb_node *new)
{
    struct rb_node *parent = old->parent;

    *new = *old;

    if (old->left)
        old->left->parent = new;
    if (old->right)
        old->right->parent = new;

    child_change(root, parent, old, new);
}

/**
 * rb_find - find @key in tree @root.
 * @root: rbtree want to search.
 * @key: key to match.
 * @cmp: operator defining the node order.
 */
struct rb_node *rb_find(const struct rb_root *root, const void *key, rb_find_t cmp)
{
    struct rb_node *node = root->rb_node;
    long ret;

    while (node) {
        ret = cmp(node, key);
        if (ret == LONG_MIN)
            return NULL;
        else if (ret < 0)
            node = node->left;
        else if (ret > 0)
            node = node->right;
        else
            return node;
    }

    return NULL;
}

/**
 * rb_find_last - find @key in tree @root and return parent.
 * @root: rbtree want to search.
 * @key: key to match.
 * @cmp: operator defining the node order.
 * @parentp: pointer used to modify the parent node pointer.
 * @linkp: pointer used to modify the point to pointer to child node.
 */
struct rb_node *rb_find_last(struct rb_root *root, const void *key, rb_find_t cmp,
                             struct rb_node **parentp, struct rb_node ***linkp)
{
    long ret;

    *linkp = &root->rb_node;
    if (unlikely(!**linkp)) {
        *parentp = NULL;
        return NULL;
    }

    do {
        ret = cmp((*parentp = **linkp), key);
        if (ret == LONG_MIN)
            return NULL;
        else if (ret < 0)
            *linkp = &(**linkp)->left;
        else if (ret > 0)
            *linkp = &(**linkp)->right;
        else
            return **linkp;
    } while (**linkp);

    return NULL;
}

/**
 * rb_parent - find the parent node.
 * @root: rbtree want to search.
 * @parentp: pointer used to modify the parent node pointer.
 * @node: new node to insert.
 * @cmp: operator defining the node order.
 * @leftmost: return whether it is the leftmost node.
 */
struct rb_node **rb_parent(struct rb_root *root, struct rb_node **parentp,
                           struct rb_node *node, rb_cmp_t cmp, bool *leftmost)
{
    struct rb_node **link;
    bool leftmost_none;
    long retval;

    if (!leftmost)
        leftmost = &leftmost_none;

    link = &root->rb_node;
    if (unlikely(!*link)) {
        *parentp = NULL;
        return link;
    }

    do {
        retval = cmp(node, (*parentp = *link));
        if (retval < 0)
            link = &(*link)->left;
        else {
            link = &(*link)->right;
            *leftmost = false;
        }
    } while (*link);

    return link;
}

/**
 * rb_parent_conflict - find the parent node or conflict.
 * @root: rbtree want to search.
 * @parentp: pointer used to modify the parent node pointer.
 * @node: new node to insert.
 * @cmp: operator defining the node order.
 * @leftmost: return whether it is the leftmost node.
 */
struct rb_node **rb_parent_conflict(struct rb_root *root, struct rb_node **parentp,
                                    struct rb_node *node, rb_cmp_t cmp, bool *leftmost)
{
    struct rb_node **link;
    bool leftmost_none;
    long retval;

    if (!leftmost)
        leftmost = &leftmost_none;

    link = &root->rb_node;
    if (unlikely(!*link)) {
        *parentp = NULL;
        return link;
    }

    do {
        retval = cmp(node, (*parentp = *link));
        if (retval < 0)
            link = &(*link)->left;
        else if (retval > 0) {
            link = &(*link)->right;
            *leftmost = false;
        } else
            return NULL;
    } while (*link);

    return link;
}

/**
 * rb_left_far - go left as we can.
 * @node: node to go left.
 */
struct rb_node *rb_left_far(const struct rb_node *node)
{
    while (node->left)
        node = node->left;

    return (struct rb_node *)node;
}

/**
 * rb_right_far - go right as we can.
 * @node: node to go right.
 */
struct rb_node *rb_right_far(const struct rb_node *node)
{
    while (node->right)
        node = node->right;

    return (struct rb_node *)node;
}

/**
 * rb_left_deep - go left deep as we can.
 * @node: node to go left deep.
 */
struct rb_node *rb_left_deep(const struct rb_node *node)
{
    while (node) {
        if (node->left)
            node = node->left;
        else if (node->right)
            node = node->right;
        else
            return (struct rb_node *)node;
    }

    return NULL;
}

/**
 * rb_right_deep - go right deep as we can.
 * @node: node to go right deep.
 */
struct rb_node *rb_right_deep(const struct rb_node *node)
{
    while (node) {
        if (node->right)
            node = node->right;
        else if (node->left)
            node = node->left;
        else
            return (struct rb_node *)node;
    }

    return NULL;
}

/**
 * rb_first/last/prev/next - Middle iteration (Sequential)
 * NOTE: find logical next and previous nodes
 */
struct rb_node *rb_first(const struct rb_root *root)
{
    struct rb_node *node = root->rb_node;

    if (!root || !node)
        return NULL;

    /* Get the leftmost node */
    node = rb_left_far(node);
    return node;
}

struct rb_node *rb_last(const struct rb_root *root)
{
    struct rb_node *node = root->rb_node;

    if (!root || !node)
        return NULL;

    /* Get the rightmost node */
    node = rb_right_far(node);
    return node;
}

struct rb_node *rb_prev(const struct rb_node *node)
{
    struct rb_node *parent;

    if (!node)
        return NULL;

    /*
     * If there is a left-hand node, go down
     * and then as far right as possible.
     */
    if (node->left) {
        node = node->left;
        return rb_right_far(node);
    }

    /*
     * No left-hand children. Go up till we find an ancestor
     * which is a right-hand child of its parent.
     */
    while ((parent = node->parent) && node != parent->right)
        node = parent;

    return parent;
}

struct rb_node *rb_next(const struct rb_node *node)
{
    struct rb_node *parent;

    if (!node)
        return NULL;

    /*
     * If there is a right-hand node, go down
     * and then as far left as possible.
     */
    if (node->right) {
        node = node->right;
        return rb_left_far(node);
    }

    /*
     * No right-hand children. Go up till we find an ancestor
     * which is a left-hand child of its parent.
     */
    while ((parent = node->parent) && node != parent->left)
        node = parent;

    return parent;
}

/**
 * rb_post_first/next - Postorder iteration (Depth-first)
 * NOTE: always visit the parent after its children
 */
struct rb_node *rb_post_first(const struct rb_root *root)
{
    struct rb_node *node = root->rb_node;

    if (!root || !node)
        return NULL;

    node = rb_left_deep(node);
    return node;
}

struct rb_node *rb_post_next(const struct rb_node *node)
{
    const struct rb_node *parent;

    if (!node)
        return NULL;

    parent = node->parent;

    if (parent && node == parent->left && parent->right)
        return rb_left_deep(parent->right);
    else
        return (struct rb_node *)parent;
}
