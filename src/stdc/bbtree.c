/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2021  <Fabien Bavent>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   - - - - - - - - - - - - - - -
 */
#include <kora/bbtree.h>
#include <assert.h>

typedef struct bbrm bbrm_t;

struct bbrm {
    bbnode_t *last;
    bbnode_t *deleted;
};

#define __NIL (&_NIL)

bbnode_t _NIL = {
    &_NIL, &_NIL, &_NIL, 0, 0
};

void bbtree_init(bbtree_t *tree)
{
    tree->root_ = __NIL;
    tree->count_ = 0;
}

/* Swap the pointers of horizontal left links.
*         |             |
*    L <- T             L -> T
*   / \    \     =>    /    / \
*  A   B    R         A    B   R
*/
static bbnode_t *bbtree_skew(bbnode_t *node)
{
    bbnode_t *temp, *parent;
    if (node == __NIL || node->left_->level_ != node->level_)
        return node;

    parent = node->parent_;
    temp = node->left_;
    node->left_ = temp->right_;
    node->left_->parent_ = node;
    temp->right_ = node;
    node->parent_ = temp;
    temp->parent_ = parent;
    return temp;
}

/* If we have two horizontal right links.
* Take the middle node, elevate it, and return it.
*    |                      |
*    T -> R -> X            R
*   /    /         =>      / \
*  A    B                 T   X
*                        / \
*                       A   B
*/
static bbnode_t *bbtree_split(bbnode_t *node)
{
    bbnode_t *temp, *parent;
    if (node == __NIL || node->right_->right_->level_ != node->level_)
        return node;

    parent = node->parent_;
    temp = node->right_;
    node->right_ = temp->left_;
    node->right_->parent_ = node;
    temp->left_ = node;
    node->parent_ = temp;
    temp->level_++;
    temp->parent_ = parent;
    return temp;
}

static bbnode_t *bbtree_insert_(bbnode_t *root, bbnode_t *node, int *ok)
{
    if (root == __NIL) {
        node->level_ = 1;
        node->left_ = __NIL;
        node->right_ = __NIL;
        node->parent_ = __NIL;
        *ok = 1;
        return node;
    }

    if (node->value_ < root->value_) {
        root->left_ = bbtree_insert_(root->left_, node, ok);
        root->left_->parent_ = root;
    } else if (node->value_ > root->value_) {
        root->right_ = bbtree_insert_(root->right_, node, ok);
        root->right_->parent_ = root;
    } else {
        *ok = 0;
        return root; // No insert
    }

    root = bbtree_skew(root);
    root = bbtree_split(root);
    return root;
}

static bbnode_t *bbtree_rebalance(bbnode_t *root)
{
    if (root->left_->level_ < root->level_ - 1 ||
        root->right_->level_ < root->level_ - 1) {
        root->level_--;
        if (root->right_->level_ > root->level_)
            root->right_->level_ = root->level_;
        root = bbtree_skew(root);
        root->right_ = bbtree_skew(root->right_);
        root->right_->right_ = bbtree_skew(root->right_->right_);
        root = bbtree_split(root);
        root->right_ = bbtree_split(root->right_);
    }
    return root;
}

static bbnode_t *bbtree_remove_(bbnode_t *root, size_t value, bbrm_t *del,
                                int *ok)
{
    *ok = 0;
    if (root == __NIL)
        return __NIL;

    // Search down the tree and set pointers last and deleted
    del->last = root;
    if (value < root->value_) {
        root->left_ = bbtree_remove_(root->left_, value, del, ok);
        root->left_->parent_ = root;
    } else {
        del->deleted = root;
        root->right_ = bbtree_remove_(root->right_, value, del, ok);
        root->right_->parent_ = root;
    }

    // At the bottom of the tree we remove the element (if present)
    if (del->last == root && del->deleted != __NIL &&
        del->deleted->value_ == value) {
        *ok = 1;
        root->right_->parent_ = root->parent_;
        return root->right_;
    }

    // On the way back, we rebalance
    bbnode_t *node = root;
    root = bbtree_rebalance(root);
    bbtree_check(root);

    if (node != del->deleted)
        return root;

    // To remove internal nodes, we swap position with last node
    bbnode_t *parent = del->deleted->parent_;
    bbnode_t *left = del->deleted->left_;
    bbnode_t *right = del->deleted->right_;
    assert(del->last->left_ == __NIL && del->last != __NIL);

    del->last->level_ = del->deleted->level_;
    del->last->parent_ = parent;
    del->last->left_ = left;
    del->last->right_ = right;
    del->last->left_->parent_ = del->last;
    del->last->right_->parent_ = del->last;
    if (parent != __NIL) {
        if (parent->left_ == del->deleted)
            parent->left_ = del->last;

        else
            parent->right_ = del->last;
    }
    if (root == del->deleted)
        root = del->last;
    bbtree_check(root);
    return root;
}

int bbtree_insert(bbtree_t *tree, bbnode_t *node)
{
    int ok;
    tree->root_ = bbtree_insert_(tree->root_, node, &ok);
    if (ok) {
        tree->count_++;
        return 0;
    }
    return -1;
}

int bbtree_remove(bbtree_t *tree, size_t value)
{
    int ok;
    bbrm_t del;
    del.deleted = __NIL;
    tree->root_ = bbtree_remove_(tree->root_, value, &del, &ok);
    if (ok) {
        tree->count_--;
        return 0;
    }
    return -1;
}

/* Find the node on extreme left side  */
bbnode_t *bbtree_left_(bbnode_t *node)
{
    if (node == __NIL)
        return NULL;

    while (node->left_ != __NIL)
        node = node->left_;

    return node;
}

/* Find the node on extreme right side  */
bbnode_t *bbtree_right_(bbnode_t *node)
{
    if (node == __NIL)
        return NULL;

    while (node->right_ != __NIL)
        node = node->right_;

    return node;
}

bbnode_t *bbtree_next_(bbnode_t *root)
{
    if (root->right_ != __NIL)
        return bbtree_left_(root->right_);

    for (; root->parent_ != __NIL; root = root->parent_) {
        if (root->parent_->right_ != root)
            return root->parent_;
    }

    return NULL;
}

bbnode_t *bbtree_previous_(bbnode_t *root)
{
    if (root->left_ != __NIL)
        return bbtree_right_(root->left_);

    for (; root->parent_ != __NIL; root = root->parent_) {
        if (root->parent_->left_ != root)
            return root->parent_;
    }

    return NULL;
}

/* Look for a value */
bbnode_t *bbtree_search_(bbnode_t *root, size_t value, int accept)
{
    bbnode_t *best;
    if (root == __NIL)
        return NULL;

    else if (root->value_ == value)
        return root;

    if (root->value_ > value) {
        best = bbtree_search_(root->left_, value, accept);
        if (accept <= 0 || (best != NULL && root->value_ > best->value_))
            return best;
        return root;
    } else {
        best = bbtree_search_(root->right_, value, accept);
        if (accept >= 0 || (best != NULL && root->value_ < best->value_))
            return best;
        return root;
    }
}

int bbtree_check(bbnode_t *node)
{
    int count = 1, ret;
    if (node->left_ != __NIL) {
        if (node->left_->parent_ != node) {
            // __fxprintf(-1, "bbtree] Bad link with left child: %p\n", node);
            return -1;
        }
        ret = bbtree_check(node->left_);
        if (ret < 0)
            return -1;
        count += ret;
    }
    if (node->right_ != __NIL) {
        if (node->right_->parent_ != node) {
            // __fxprintf(-1, "bbtree] Bad link with right child: %p\n", node);
            return -1;
        }
        ret = bbtree_check(node->right_);
        if (ret < 0)
            return -1;
        count += ret;
    }
    if (node->left_ != __NIL && node->right_ != __NIL) {
        if (node->level_ == 1) {
            // __fxprintf(-1, "bbtree] Node is mark as leaf: %p\n", node);
            return -1;
        } else {
            if (node->level_ - 1 != (node->left_->level_ + node->right_->level_) / 2) {
                // __fxprintf(-1, "bbtree] Node as the wrong level: %p\n", node);
                return -1;
            } else if (node->left_->level_ > node->right_->level_) {
                // __fxprintf(-1, "bbtree] Only right-edge should be horizontal: %p\n", node);
                return -1;
            }
        }
    }
    return count;
}
