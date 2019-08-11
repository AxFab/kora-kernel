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

#endif  /* _KORA_LLIST_H */
