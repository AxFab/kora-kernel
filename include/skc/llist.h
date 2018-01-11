/*
 *      This file is part of the SmokeOS project.
 *  Copyright (C) 2015  <Fabien Bavent>
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
 *
 *      Linked list implementation.
 */
#ifndef _SKC_LLIST_H
#define _SKC_LLIST_H 1

#include <skc/stddef.h>

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


void ll_push_back(llhead_t *list, llnode_t *node);
void ll_push_front(llhead_t *list, llnode_t *node);
llnode_t *ll_pop_back(llhead_t *list);
llnode_t *ll_pop_front(llhead_t *list);
void ll_remove(llhead_t *list, llnode_t *node);

#define ll_append(h,n)  ll_push_back(h,n)
#define ll_enqueue(h,n)  ll_push_front(h,n)
#define ll_dequeue(h,t,m)  itemof(ll_pop_back(h),t,m)

#define ll_first(h,t,m)  itemof((h)->first_,t,m)
#define ll_last(h,t,m)  itemof((h)->last_,t,m)
#define ll_previous(n,t,m)  itemof((n)->prev_,t,m)
#define ll_next(n,t,m)  itemof((n)->next_,t,m)

#define ll_each(h,v,t,m)  ((v)=ll_first(h,t,m);(v);(v)=(t*)ll_next(&(v)->m,t,m))
#define ll_each_reverse(h,v,t,m)  ((v)=ll_last(h,t,m);(v);(v)=(t*)ll_previous(&(v)->m,t,m))

#endif  /* _SKC_LLIST_H */
