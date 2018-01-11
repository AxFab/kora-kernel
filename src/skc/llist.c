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
 */
#include <skc/llist.h>
#include <assert.h>


/* Push an element at the front of a linked list */
void ll_push_front(llhead_t *list, llnode_t *node)
{
  assert(node->prev_ == NULL);
  assert(node->next_ == NULL);

  node->next_ = list->first_;
  if (list->first_ != NULL) {
    list->first_->prev_ = node;
  } else {
    list->last_ = node;
  }

  node->prev_ = NULL;
  list->first_ = node;
  ++list->count_;
}

/* Push an element at the end of a linked list */
void ll_push_back(llhead_t *list, llnode_t *node)
{
  assert(node->prev_ == NULL);
  assert(node->next_ == NULL);

  node->prev_ = list->last_;
  if (list->last_ != NULL) {
    list->last_->next_ = node;
  } else {
    list->first_ = node;
  }

  node->next_ = NULL;
  list->last_ = node;
  ++list->count_;
}

/* Retrun and remove an element from the start of a linked list */
llnode_t *ll_pop_front(llhead_t *list)
{
  llnode_t *first = list->first_;

  assert(first == NULL || first->prev_ == NULL);
  if (first == NULL) {
    return NULL;
  } else if (first->next_) {
    first->next_->prev_ = NULL;
  } else {
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

/* Retrun and remove an element from the end of a linked list */
llnode_t *ll_pop_back(llhead_t *list)
{
  llnode_t *last = list->last_;

  assert(last == NULL || last->next_ == NULL);
  if (last == NULL) {
    return NULL;
  } else if (last->prev_) {
    last->prev_->next_ = NULL;
  } else {
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

/* Remove an item from the linked list, whitout checking presence or not */
void ll_remove(llhead_t *list, llnode_t *node)
{
#if !defined(NDEBUG)
  struct llnode *w = node;
  while (w->prev_) w = w->prev_;
  assert(w == list->first_);
  w = node;
  while (w->next_) w = w->next_;
  assert(w == list->last_);
#endif

  if (node->prev_) {
    node->prev_->next_ = node->next_;
  } else {
    assert(list->first_ == node);
    list->first_ = node->next_;
  }

  if (node->next_) {
    node->next_->prev_ = node->prev_;
  } else {
    assert(list->last_ == node);
    list->last_ = node->prev_;
  }

  node->prev_ = NULL;
  node->next_ = NULL;
  --list->count_;
}
