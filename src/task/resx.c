/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2018  <Fabien Bavent>
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
#include <kernel/core.h>
#include <kernel/device.h>
#include <kernel/task.h>
#include <kora/rwlock.h>
#include <errno.h>

struct stream {
    inode_t *ino;
    bbnode_t node; // TODO -- Is BBTree the best data structure !?
    rwlock_t lock; // TODO -- Usage
};

struct resx {
    bbtree_t tree; // TODO -- Is BBTree the best data structure !?
    rwlock_t lock;
    atomic32_t users;
};


resx_t *resx_create()
{
    resx_t *resx = (resx_t *)kalloc(sizeof(resx_t));
    bbtree_init(&resx->tree);
    rwlock_init(&resx->lock);
    atomic_inc(&resx->users);
    return resx;
}

resx_t *resx_rcu(resx_t *resx, int usage)
{
    if (usage == 1)
        atomic_inc(&resx->users);
    else {
        if (atomic32_xadd(&resx->users, -1) == 1) {
            // WE CAN DESALLOCATE THIS ONE !
            kfree(resx);
            return NULL;
        }
    }
    return resx;
}

inode_t *resx_get(resx_t *resx, int fd)
{
    rwlock_rdlock(&resx->lock);
    stream_t *stm = bbtree_search_eq(&resx->tree, (size_t)fd, stream_t, node);
    rwlock_rdunlock(&resx->lock);
    if (stm == NULL) {
        errno = EBADF;
        return NULL;
    }
    errno = 0;
    return stm->ino;
}

int resx_set(resx_t *resx, inode_t *ino)
{
    rwlock_wrlock(&resx->lock);
    stream_t *stm = (stream_t *)kalloc(sizeof(stream_t));
    stm->ino = vfs_open(ino);
    rwlock_init(&stm->lock);
    if (resx->tree.count_ == 0)
        stm->node.value_ = 0;

    else {
        stm->node.value_ = (size_t)(bbtree_last(&resx->tree, stream_t,
                                                node)->node.value_ + 1);
        if ((long)stm->node.value_ < 0) {
            rwlock_wrunlock(&resx->lock);
            kfree(stm);
            errno = ENOMEM; // TODO -- need a free list !
            return -1;
        }
    }
    bbtree_insert(&resx->tree, &stm->node);
    rwlock_wrunlock(&resx->lock);
    return stm->node.value_;
}

int resx_close(resx_t *resx, int fd)
{
    rwlock_wrlock(&resx->lock);
    stream_t *stm = bbtree_search_eq(&resx->tree, (size_t)fd, stream_t, node);
    if (stm == NULL) {
        errno = EBADF;
        return -1;
    }
    bbtree_remove(&resx->tree, (size_t)fd);
    rwlock_wrunlock(&resx->lock);
    // TODO -- free using RCU
    vfs_close(stm->ino);
    kfree(stm);
    errno = 0;
    return 0;
}

