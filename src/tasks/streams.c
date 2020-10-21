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

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

static void stream_destroy(fstream_t *stm)
{
    vfs_usage(stm->file, stm->flags, -1);
    vfs_close_fsnode(stm->file);
    kfree(stm);
}

fstream_t *stream_put(streamset_t *strms, fsnode_t *file, int flags)
{
    fstream_t *stm = kalloc(sizeof(fstream_t));
    stm->file = file;
    stm->position = 0;
    stm->flags = flags;
    vfs_usage(file, flags, 1);
    splock_lock(&strms->lock);
    fstream_t *p = bbtree_first(&strms->tree, fstream_t, node);
    stm->fd = p == NULL ? 0 : p->fd + 1;
    stm->node.value_ = stm->fd;
    bbtree_insert(&strms->tree, &stm->node);
    splock_unlock(&strms->lock);
    errno = 0;
    return stm;
}

fstream_t *stream_get(streamset_t *strms, int fd)
{
    splock_lock(&strms->lock);
    fstream_t *stm = bbtree_search_eq(&strms->tree, fd, fstream_t, node);
    splock_unlock(&strms->lock);
    errno = stm == NULL ? EBADF : 0;
    return stm;
}

void stream_remove(streamset_t *strms, fstream_t *stm)
{
    splock_lock(&strms->lock);
    bbtree_remove(&strms->tree, stm->node.value_);
    splock_unlock(&strms->lock);
    stream_destroy(stm);
    errno = 0;
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
    if (atomic_fetch_sub(&strms->rcu, 1) > 1)
        return;

    while (strms->tree.count_ > 0) {
        fstream_t *stream = bbtree_first(&strms->tree, fstream_t, node);
        stream_destroy(stream);
    }
    kfree(strms);
}

