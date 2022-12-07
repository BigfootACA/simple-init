/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2022 Sanpe <sanpeqf@gmail.com>
 */

#include "logger.h"
#include "rbtree.h"
#define TAG "rbtree"

#ifdef DEBUG_RBTREE
bool rb_debug_link_check(struct rb_node *parent, struct rb_node **link, struct rb_node *node)
{
    if (unlikely(*link == node)) {
        telog_crit("rb_insert corruption (%p) *link should not be node (%p)\n",
            link, node);
        return false;
    }

    return true;
}

bool rb_debug_delete_check(struct rb_node *node)
{
    if (unlikely(node->left == POISON_RBNODE1)) {
        telog_crit("rb_delete corruption (%p) node->left should not be POISON_RBNODE1 (%p)\n",
            node, POISON_RBNODE1);
        return false;
    }

    if (unlikely(node->right == POISON_RBNODE2)) {
        telog_crit("rb_delete corruption (%p) node->right should not be POISON_RBNODE2 (%p)\n",
            node, POISON_RBNODE2);
        return false;
    }

    if (unlikely(node->parent == POISON_RBNODE3)) {
        telog_crit("rb_delete corruption (%p) node->parent should not be POISON_RBNODE3 (%p)\n",
            node, POISON_RBNODE3);
        return false;
    }

    return true;
}
#endif
