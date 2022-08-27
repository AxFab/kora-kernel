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
    inode_t *dir;
    fnode_t *node;
};


static inode_t *vfs_readdir_std(diterator_t *it)
{
    inode_t *ino = it->dir->ops->readdir(it->dir, it->name, it->ctx);
    if (ino == NULL)
        return NULL;

    fnode_t *child = vfs_fsnode_from(it->node, it->name);
    if (child->mode == FN_OK) {
        vfs_close_fnode(child);
        return ino;
    }

    mtx_lock(&child->mtx);
    if (child->mode == FN_EMPTY)
        vfs_resolve(child, ino);
    mtx_unlock(&child->mtx);
    vfs_close_fnode(child);
    return ino;
}

diterator_t *vfs_opendir(vfs_t *vfs, const char *name, user_t *user)
{
    fnode_t *node = vfs_search(vfs, name, user, true, true);
    if (node == NULL)
        return NULL;

    inode_t *dir = vfs_inodeof(node);
    if (dir == NULL) {
        errno = ENOENT;
        vfs_close_fnode(node);
        return NULL;
    }

    if (dir->type != FL_DIR) {
        errno = ENOTDIR;
        goto err;
    } else if (vfs_access(dir, user, VM_RD) != 0) {
        errno = EACCES;
        goto err;
    } else if (dir->ops->opendir == NULL) {
        errno = EPERM;
        goto err;
    }

    void *ctx = dir->ops->opendir(dir);
    if (ctx == NULL)
        goto err;

    diterator_t *it = kalloc(sizeof(diterator_t));
    it->ctx = ctx;
    it->mode = 0;
    it->dir = dir;
    it->node = node;
    return it;

err:
    assert(errno != 0);
    vfs_close_fnode(node);
    vfs_close_inode(dir);
    return NULL;
}

inode_t *vfs_readdir(diterator_t *it, char *buf, int len)
{
    if (it == NULL || it->ctx == NULL || it->dir == NULL) {
        errno = EINVAL;
        return NULL;
    }

    inode_t *ino;
    if (it->mode == 0) {
        ino = vfs_readdir_std(it);
        if (ino != NULL) {
            strncpy(buf, it->name, len);
            return ino;
        }
    }

    for (;;) {
        it->mode++;
        splock_lock(&it->node->lock);
        fnode_t *node = ll_index(&it->node->mnt, it->mode - 1, fnode_t, nmt);
        splock_unlock(&it->node->lock);
        if (node == NULL)
            return NULL;

        ino = vfs_inodeof(node);
        if (unlikely(ino == NULL))
            continue;
        strncpy(buf, node->name, len);
        return ino;
    }
}

int vfs_closedir(diterator_t *it)
{
    if (it == NULL || it->ctx == NULL || it->dir == NULL) {
        errno = EINVAL;
        return -1;
    }

    it->dir->ops->closedir(it->dir, it->ctx);
    vfs_close_fnode(it->node);
    vfs_close_inode(it->dir);
    kfree(it);
    return 0;
}

