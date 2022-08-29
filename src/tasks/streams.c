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
#include <kernel/stdc.h>
#include <kernel/vfs.h>
#include <kora/splock.h>
#include <kora/bbtree.h>
#include <kernel/tasks.h>
#include <errno.h>

struct streamset {
    splock_t lock;
    bbtree_t tree;
    atomic_int rcu;
};

typedef struct fstream resx_t;
struct fstream
{
    void *data;
    int type;
    bbnode_t node;
    void(*close)(void *);
};

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


int resx_put(streamset_t *strms, int type, void *data, void(*close)(void *))
{
    resx_t *resx = kalloc(sizeof(resx_t));
    splock_lock(&strms->lock);
    resx_t *p = bbtree_last(&strms->tree, resx_t, node);
    size_t handle = p == NULL ? 0 : p->node.value_ + 1;
    resx->node.value_ = handle;
    resx->type = type;
    resx->data = data;
    resx->close = close;
    bbtree_insert(&strms->tree, &resx->node);
    splock_unlock(&strms->lock);
    return resx->node.value_;
}

void *resx_get(streamset_t *strms, int type, int handle)
{
    splock_lock(&strms->lock);
    resx_t *resx = bbtree_search_eq(&strms->tree, handle, resx_t, node);
    splock_unlock(&strms->lock);
    return resx->type == type ? resx : NULL;
}

void resx_remove(streamset_t *strms, int handle)
{
    splock_lock(&strms->lock);
    resx_t *resx = bbtree_search_eq(&strms->tree, handle, resx_t, node);
    if (resx != NULL) {
        bbtree_remove(&strms->tree, resx->node.value_);
        if (resx->close) {
            splock_unlock(&strms->lock);
            resx->close(resx->data);
            splock_lock(&strms->lock);
        }
        kfree(resx);
    }
    splock_unlock(&strms->lock);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

streamset_t *stream_create_set()
{
    streamset_t *strms = kalloc(sizeof(streamset_t));
    splock_init(&strms->lock);
    bbtree_init(&strms->tree);
    strms->rcu = 1;
    return strms;
}

streamset_t *stream_open_set(streamset_t *strms)
{
    atomic_inc(&strms->rcu);
    return strms;
}

void stream_close_set(streamset_t *strms)
{
    if (atomic_xadd(&strms->rcu, -1) > 1)
        return;

    while (strms->tree.count_ > 0) {
        resx_t *resx = bbtree_first(&strms->tree, resx_t, node);
        if (resx->close)
            resx->close(resx->data);
        kfree(resx);
    }
    kfree(strms);
}
