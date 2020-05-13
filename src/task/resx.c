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
#include <kernel/core.h>
#include <kernel/device.h>
#include <kernel/task.h>
#include <kora/rwlock.h>
#include <errno.h>

resx_fs_t *resx_fs_create()
{
    resx_fs_t *resx = (resx_fs_t *)kalloc(sizeof(resx_fs_t));
    if (kCPU.running) {
        resx->root = resx_fs_root(kCPU.running->resx_fs);
        resx->pwd = resx_fs_pwd(kCPU.running->resx_fs);
    } else {
        resx->root = vfs_open(kSYS.dev_ino, X_OK);
        resx->pwd = vfs_open(kSYS.dev_ino, X_OK);
    }
    resx->umask = 022;
    atomic_inc(&resx->users);
    return resx;
}

resx_fs_t *resx_fs_open(resx_fs_t *resx)
{
    atomic_inc(&resx->users);
    return resx;
}

void resx_fs_close(resx_fs_t *resx)
{
    if (atomic_fetch_sub(&resx->users, 1) == 1) {
        vfs_close(resx->root, X_OK);
        vfs_close(resx->pwd, X_OK);
    }
}

inode_t *resx_fs_root(resx_fs_t *resx)
{
    inode_t *ino = vfs_open(resx->root, X_OK);
    return ino;
}

inode_t *resx_fs_pwd(resx_fs_t *resx)
{
    inode_t *ino = vfs_open(resx->pwd, X_OK);
    return ino;
}

void resx_fs_chroot(resx_fs_t *resx, inode_t *ino)
{
    vfs_close(resx->root, X_OK);
    resx->root = vfs_open(ino, X_OK);
}

void resx_fs_chpwd(resx_fs_t *resx, inode_t *ino)
{
    vfs_close(resx->pwd, X_OK);
    resx->pwd = vfs_open(ino, X_OK);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

resx_t *resx_create()
{
    resx_t *resx = (resx_t *)kalloc(sizeof(resx_t));
    bbtree_init(&resx->tree);
    rwlock_init(&resx->lock);
    atomic_inc(&resx->users);
    return resx;
}

resx_t *resx_open(resx_t *resx)
{
    atomic_inc(&resx->users);
    return resx;
}

void resx_close(resx_t *resx)
{
    if (atomic_fetch_sub(&resx->users, 1) == 1) {
        while (resx->tree.count_ > 0)
            resx_rm(resx, resx->tree.root_->value_);
        kfree(resx);
    }
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

stream_t *resx_get(resx_t *resx, int fd)
{
    rwlock_rdlock(&resx->lock);
    stream_t *stm = bbtree_search_eq(&resx->tree, (size_t)fd, stream_t, node);
    rwlock_rdunlock(&resx->lock);
    if (stm == NULL) {
        errno = EBADF;
        return NULL;
    }
    errno = 0;
    return stm;
}

stream_t *resx_set(resx_t *resx, inode_t *ino, int access)
{
    rwlock_wrlock(&resx->lock);
    stream_t *stm = (stream_t *)kalloc(sizeof(stream_t));
    stm->ino = vfs_open(ino, access & 7);
    stm->flags = access & 7;
    rwlock_init(&stm->lock);
    if (resx->tree.count_ == 0)
        stm->node.value_ = 0;

    else {
        stream_t *last = bbtree_last(&resx->tree, stream_t, node);
        stm->node.value_ = (size_t)(last->node.value_ + 1);
        if ((long)stm->node.value_ < 0) {
            rwlock_wrunlock(&resx->lock);
            kfree(stm);
            errno = ENOMEM; // TODO -- need a free list !
            return NULL;
        }
    }
    bbtree_insert(&resx->tree, &stm->node);
    rwlock_wrunlock(&resx->lock);
    return stm;
}

int resx_rm(resx_t *resx, int fd)
{
    rwlock_wrlock(&resx->lock);
    stream_t *stm = bbtree_search_eq(&resx->tree, (size_t)fd, stream_t, node);
    if (stm == NULL) {
        rwlock_wrunlock(&resx->lock);
        errno = EBADF;
        return -1;
    }
    bbtree_remove(&resx->tree, (size_t)fd);
    rwlock_wrunlock(&resx->lock);

    // TODO -- free using RCU
    vfs_close(stm->ino, stm->flags & 7);
    kfree(stm);
    errno = 0;
    return 0;
}

char *env_open(char *envs)
{
    return NULL;
}

char *env_create(const char **envs)
{
    return NULL;
}


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


rxfs_t *rxfs_create(inode_t *ino)
{
    rxfs_t *fs = kalloc(sizeof(rxfs_t));
    fs->root = vfs_open(ino, X_OK);
    fs->pwd = vfs_open(ino, X_OK);
    fs->umask = 0022;
    atomic_store(&fs->rcu, 1);
    return fs;
}


rxfs_t *rxfs_open(rxfs_t *fs)
{
    atomic_fetch_add(&fs->rcu, 1);
    return fs;
}

rxfs_t *rxfs_clone(rxfs_t *fs)
{
    rxfs_t *copy = kalloc(sizeof(rxfs_t));
    copy->root = vfs_open(fs->root, X_OK);
    copy->pwd = vfs_open(fs->pwd, X_OK);
    copy->umask = fs->umask;
    atomic_store(&copy->rcu, 1);
    return copy;
}

void rxfs_close(rxfs_t *fs)
{
    int val = atomic_fetch_sub(&fs->rcu, 1);
    if (val == 1) {
        vfs_close(fs->root, X_OK);
        vfs_close(fs->pwd, X_OK);
        kfree(fs);
    }
}

void rxfs_chroot(rxfs_t *fs, inode_t *ino)
{
    vfs_close(fs->root, X_OK);
    fs->root = vfs_open(ino, X_OK);
}

void rxfs_chdir(rxfs_t *fs, inode_t *ino)
{
    vfs_close(fs->pwd, X_OK);
    fs->pwd = vfs_open(ino, X_OK);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


usr_t *usr_open(usr_t *usr)
{
    return NULL;
}



