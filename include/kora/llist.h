/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2019  <Fabien Bavent>
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
#ifndef _KORA_LLIST_H
#define _KORA_LLIST_H 1

#include <stddef.h>
#include <assert.h>

#ifndef itemof
#  undef offsetof
#  define offsetof(t,m)   ((size_t)&(((t*)0)->m))
#  define itemof(p,t,m)   ((t*)itemof_((p), offsetof(t,m)))
static inline void *itemof_(void *ptr, int off)
{
    return ptr ? (char *)ptr - off : 0;
}
#endif

typedef struct llhead llhead_t;
typedef struct llnode llnode_t;

/* Doubled linked list head */
struct llhead {
    llnode_t *first_;
    llnode_t *last_;
    int count_;
};

/* Doubled linked list node */
struct llnode {
    llnode_t *prev_;
    llnode_t *next_;
};

#define INIT_LLHEAD  {NULL,NULL,0}

#define ll_append(h,n)  ll_push_back(h,n)
#define ll_enqueue(h,n)  ll_push_front(h,n)
#define ll_dequeue(h,t,m)  itemof(ll_pop_back(h),t,m)

#define ll_first(h,t,m)  itemof((h)->first_,t,m)
#define ll_last(h,t,m)  itemof((h)->last_,t,m)
#define ll_previous(n,t,m)  itemof((n)->prev_,t,m)
#define ll_next(n,t,m)  itemof((n)->next_,t,m)

#define ll_index(h,v,t,m)  itemof(ll_index_(h,v),t,m)

#define ll_each(h,v,t,m)  ((v)=ll_first(h,t,m);(v);(v)=(t*)ll_next(&(v)->m,t,m))
#define ll_each_reverse(h,v,t,m)  ((v)=ll_last(h,t,m);(v);(v)=(t*)ll_previous(&(v)->m,t,m))

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

/* Push an element at the front of a linked list */
static inline void ll_push_front(llhead_t *list, llnode_t *node)
{
    assert(node->prev_ == NULL);
    assert(node->next_ == NULL);

    node->next_ = list->first_;
    if (list->first_ != NULL)
        list->first_->prev_ = node;

    else
        list->last_ = node;

    node->prev_ = NULL;
    list->first_ = node;
    ++list->count_;
}

/* Push an element at the end of a linked list */
static inline void ll_push_back(llhead_t *list, llnode_t *node)
{
    assert(node->prev_ == NULL);
    assert(node->next_ == NULL);

    node->prev_ = list->last_;
    if (list->last_ != NULL)
        list->last_->next_ = node;

    else
        list->first_ = node;

    node->next_ = NULL;
    list->last_ = node;
    ++list->count_;
}

/* Return and remove an element from the start of a linked list */
static inline llnode_t *ll_pop_front(llhead_t *list)
{
    llnode_t *first = list->first_;

    assert(first == NULL || first->prev_ == NULL);
    if (first == NULL)
        return NULL;

    else if (first->next_)
        first->next_->prev_ = NULL;

    else {
        assert(list->last_ == first);
        assert(first->next_ == NULL);
        list->last_ = NULL;
    }

    list->first_ = first->next_;
    first->prev_ = NULL;
    first->next_ = NULL;
    --list->count_;
    return first;
}

/* Return and remove an element from the end of a linked list */
static inline llnode_t *ll_pop_back(llhead_t *list)
{
    llnode_t *last = list->last_;

    assert(last == NULL || last->next_ == NULL);
    if (last == NULL)
        return NULL;

    else if (last->prev_)
        last->prev_->next_ = NULL;

    else {
        assert(list->first_ == last);
        assert(last->prev_ == NULL);
        list->first_ = NULL;
    }

    list->last_ = last->prev_;
    last->prev_ = NULL;
    last->next_ = NULL;
    --list->count_;
    return last;
}

/* Remove an item from the linked list, without checking presence or not */
static inline void ll_remove(llhead_t *list, llnode_t *node)
{
#if !defined(NDEBUG)
    struct llnode *w = node;
    while (w->prev_)
        w = w->prev_;
    assert(w == list->first_);
    w = node;
    while (w->next_)
        w = w->next_;
    assert(w == list->last_);
#endif

    if (node->prev_)
        node->prev_->next_ = node->next_;

    else {
        assert(list->first_ == node);
        list->first_ = node->next_;
    }

    if (node->next_)
        node->next_->prev_ = node->prev_;

    else {
        assert(list->last_ == node);
        list->last_ = node->prev_;
    }

    node->prev_ = NULL;
    node->next_ = NULL;
    --list->count_;
}

static inline llnode_t *ll_index_(llhead_t *list, int idx)
{
    llnode_t *node = list->first_;
    while (node && idx--)
        node = node->next_;
    return node;
}

static inline void llist_init(llhead_t *list)
{
    list->first_ = list->last_ = NULL;
    list->count_ = 0;
}

#include "stdbool.h"

#define MAX_ELMTS 2000

static inline bool quick_sort(int *arr, int elmts)
{
    int piv, beg[MAX_ELMTS], end[MAX_ELMTS], i = 0, L, R;

    beg[0] = 0;
    end[0] = elmts;
    while (i >= 0) {
        L = beg[i];
        R = end[i] - 1;
        if (L < R) {
            piv = arr[L];
            if (i == MAX_ELMTS)
                return false;
            while (L < R) {
                while(arr[R] >= piv && L < R)
                    R--;
                if (L < R)
                    arr[L++] = arr[R];
                while(arr[L] <= piv && L < R)
                    L++;
                if (L < R)
                    arr[R--] = arr[L];
            }
            arr[L] = piv;
            beg[i + 1] = L + 1;
            end[i + 1] = end[i];
            end[i] = L;
            i++;
        } else
            i--;
    }
    return true;
}

static inline bool llist_check(llhead_t *list)
{
    llnode_t *node = list->first_;
    if (node == NULL)
        return list->count_ == 0;
    if (node->prev_)
        return false;
    int i = 0;
    while (node->next_) {
        llnode_t *next = node->next_;
        ++i;
        if (node->prev_ != node)
            return false;
        node = next;
    }
    if (node != list->last_)
        return false;
    return list->count_ == i;
}

static inline void llist_swap(llhead_t *list, llnode_t *a, llnode_t *b)
{
    llnode_t *tmp;

    tmp = a->next_;
    a->next_ = b->next_;
    b->next_ = tmp;
    if (a->next_ != NULL)
        a->next_->prev_ = a;
    else
        list->last_ = a;
    if (b->next_ != NULL)
        b->next_->prev_ = b;
    else
        list->last_ = b;

    tmp = a->prev_;
    a->prev_ = b->prev_;
    b->prev_ = tmp;
    if (a->prev_ != NULL)
        a->prev_->next_ = a;
    else
        list->first_ = a;
    if (b->prev_ != NULL)
        b->prev_->next_ = b;
    else
        list->first_ = b;
}

static inline void llist_insert_sort(llhead_t *list, llnode_t *node, int off, int(*compare)(void *, void *))
{
    /* if list is empty */
    if (list->first_ == NULL) {
        node->prev_ = NULL;
        node->next_ = NULL;
        list->first_ = node;
        list->last_ = node;
        list->count_ = 1;
        return;
    }

    /* if the node is to,be inserted at the beginning */
    if (compare(itemof_(list->first_, off), itemof_(node, off)) >= 0)
    {
        node->prev_ = NULL;
        node->next_ = list->first_;
        list->first_ = node;
        list->count_++;
        return;
    }

    llnode_t *cursor = list->first_;
    /* locate the node after which the node is to be inserted */
    while (cursor->next_ != NULL && compare(itemof_(cursor->next_, off), itemof_(node, off)) < 0)
        cursor = cursor->next_;
    /* Make the appropriate links */
    node->next_ = cursor->next_;
    cursor->next_ = node;
    if (node->next_ != NULL)
        node->next_->prev_ = node;
    else
        list->last_ = node;
    node->prev_ = cursor;
    list->count_++;
}



static inline void llist_sort(llhead_t *list, int off, int (*compare)(void *, void *))
{
    /* Initialize 'sorted' double linked list */
    llhead_t sorted = INIT_LLHEAD;

    /* Traverse and reinsert nodes on sorted list */
    llnode_t *next;
    for (llnode_t *node = list->first_; node != NULL; node = next) {
        /* store for next iteration */
        next = node->next_;
        /* Insert node into sorted list */
        node->prev_ = node->next_ = NULL;
        llist_insert_sort(&sorted, node, off, compare);
    }

    /* Update list to point on new list */
    list->first_ = sorted.first_;
    list->last_ = sorted.last_;
    list->count_ = sorted.count_;
}

#endif  /* _KORA_LLIST_H */
