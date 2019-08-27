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
#ifndef _KORA_BBTREE_H
#define _KORA_BBTREE_H 1

#include <stddef.h>
#include <kora/mcrs.h>

typedef struct bbtree bbtree_t;
typedef struct bbnode bbnode_t;

#ifndef itemof
#  undef offsetof
#  define offsetof(t,m)   ((size_t)&(((t*)0)->m))
#  define itemof(p,t,m)   ((t*)itemof_((p), offsetof(t,m)))
static inline void *itemof_(void *ptr, int off)
{
    return ptr ? (char *)ptr - off : 0;
}
#endif


/* BBTree (self-balancing binary tree) head */
struct bbtree {
    bbnode_t *root_;
    int count_;
};

/* BBTree (self-balancing binary tree) node */
struct bbnode {
    bbnode_t *parent_;
    bbnode_t *left_;
    bbnode_t *right_;
    size_t value_;
    int level_;
};

LIBAPI void bbtree_init(bbtree_t* tree);
LIBAPI int bbtree_check(bbnode_t *node);
LIBAPI int bbtree_insert(bbtree_t *tree, bbnode_t *node);
LIBAPI int bbtree_remove(bbtree_t *tree, size_t value);

LIBAPI bbnode_t *bbtree_left_(bbnode_t *node);
LIBAPI bbnode_t *bbtree_right_(bbnode_t *node);
LIBAPI bbnode_t *bbtree_next_(bbnode_t *node);
LIBAPI bbnode_t *bbtree_previous_(bbnode_t *node);

LIBAPI bbnode_t *bbtree_search_(bbnode_t *node, size_t value, int accept);

#define bbtree_first(t,s,m) (s*)itemof(bbtree_left_((t)->root_),s,m)
#define bbtree_last(t,s,m) (s*)itemof(bbtree_right_((t)->root_),s,m)

#define bbtree_next(n,s,m) (s*)itemof(bbtree_next_(n),s,m)
#define bbtree_previous(n,s,m) (s*)itemof(bbtree_previous_(n),s,m)
#define bbtree_left(n,s,m) (s*)itemof(bbtree_left_(n),s,m)
#define bbtree_right(n,s,m) (s*)itemof(bbtree_right_(n),s,m)

#define bbtree_each(t,n,s,m)  ((n)=bbtree_first(t,s,m);(n);(n)=bbtree_next(&(n)->m,s,m))

#define bbtree_search_eq(t,v,s,m) (s*)itemof(bbtree_search_((t)->root_,v,0),s,m)
#define bbtree_search_le(t,v,s,m) (s*)itemof(bbtree_search_((t)->root_,v,-1),s,m)
#define bbtree_search_ge(t,v,s,m) (s*)itemof(bbtree_search_((t)->root_,v,1),s,m)

#endif  /* _KORA_BBTREE_H */
