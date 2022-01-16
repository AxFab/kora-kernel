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
#include <kernel/vfs.h>
#include <assert.h>
#include <errno.h>

struct diterator {
    int mode;
    void *ctx;
    char name[256];
};

diterator_t *vfs_opendir(fsnode_t *dir, void *acl)
{
    assert(dir->mode == FN_OK);
    if (dir->ino->type != FL_DIR) {
        errno = ENOTDIR;
        return NULL;
        // } else if (vfs_access(dir, R_OK, acl) != 0) {
        //        errno = EACCES;
        //        return NULL;
    }

    void *ctx = dir->ino->ops->opendir(dir->ino);
    if (ctx == NULL)
        return NULL;
    diterator_t *it = kalloc(sizeof(diterator_t));
    it->ctx = ctx;
    it->mode = 0;
    return it;
}

static fsnode_t *vfs_readdir_std(fsnode_t *dir, diterator_t *it)
{
    inode_t *ino = dir->ino->ops->readdir(dir->ino, it->name, it->ctx);
    if (ino == NULL)
        return NULL;

    fsnode_t *node = vfs_fsnode_from(dir, it->name);
    if (node->mode == FN_OK) {
        vfs_close_inode(ino);
        return node;
    }

    mtx_lock(&node->mtx);
    // TODO - Check for already resolved fsnode_t
    node->ino = ino;
    node->mode = FN_OK;
    mtx_unlock(&node->mtx);
    return node;
}

fsnode_t *vfs_readdir(fsnode_t *dir, diterator_t *it)
{
    assert(dir->mode == FN_OK);
    if (it == NULL || it->ctx == NULL) {
        errno = EINVAL;
        return NULL;
    }

    fsnode_t *node = NULL;
    if (it->mode == 0)
        node = vfs_readdir_std(dir, it);
    if (node != NULL)
        return node;

    // List mounted point!
    it->mode++;
    splock_lock(&dir->lock);
    node = ll_index(&dir->mnt, it->mode - 1, fsnode_t, nmt);
    splock_unlock(&dir->lock);
    return node;
}

int vfs_closedir(fsnode_t *dir, diterator_t *it)
{
    assert(dir->mode == FN_OK);
    if (it == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (it->ctx != NULL)
        dir->ino->ops->closedir(dir->ino, it->ctx);
    kfree(it);
    return 0;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


int vfs_mkdir(fsnode_t *node, int mode, void *user)
{
    assert(node != NULL);
    mtx_lock(&node->mtx);
    if (node->mode != FN_EMPTY && node->mode != FN_NOENTRY) {
        mtx_unlock(&node->mtx);
        errno = EEXIST;
        return -1;
    }

    assert(node->parent);
    inode_t *dir = node->parent->ino;
    assert(dir != NULL);
    if (dir->ops->mkdir == NULL) {
        mtx_unlock(&node->mtx);
        errno = ENOSYS;
        return -1;
    }
    assert(irq_ready());
    inode_t *ino = dir->ops->mkdir(dir, node->name, mode, user);
    if (ino != NULL) {
        node->ino = ino;
        node->mode = FN_OK;
    }
    mtx_unlock(&node->mtx);
    return ino ? 0 : -1;
}


int vfs_rmdir(fsnode_t *node)
{
    mtx_lock(&node->mtx);
    if (node->mode != FN_OK) {
        mtx_unlock(&node->mtx);
        errno = ENOENT;
        return -1;
    }

    inode_t *dir = node->parent->ino;
    if (dir->ops->rmdir == NULL) {
        mtx_unlock(&node->mtx);
        errno = ENOSYS;
        return -1;
    }
    int ret = dir->ops->rmdir(dir, node->name);
    if (ret == 0) {
        vfs_close_inode(node->ino);
        node->ino = NULL;
        node->mode = FN_EMPTY;
    }
    mtx_unlock(&node->mtx);
    return ret;
}
