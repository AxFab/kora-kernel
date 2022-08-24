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
#include "fnode.h"

struct diterator {
    int mode;
    void *ctx;
    char name[256];
};

diterator_t *vfs_opendir(fnode_t *dir, void *acl)
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

static fnode_t *vfs_readdir_std(fnode_t *dir, diterator_t *it)
{
    inode_t *ino = dir->ino->ops->readdir(dir->ino, it->name, it->ctx);
    if (ino == NULL)
        return NULL;

    fnode_t *node = vfs_fsnode_from(dir, it->name);
    if (node->mode == FN_OK) {
        vfs_close_inode(ino);
        return node;
    }

    mtx_lock(&node->mtx);
    // TODO - Check for already resolved fnode_t
    vfs_resolve(node, ino);
    mtx_unlock(&node->mtx);
    vfs_close_inode(ino);
    return node;
}

fnode_t *vfs_readdir(fnode_t *dir, diterator_t *it)
{
    assert(dir->mode == FN_OK);
    if (it == NULL || it->ctx == NULL) {
        errno = EINVAL;
        return NULL;
    }

    fnode_t *node = NULL;
    if (it->mode == 0)
        node = vfs_readdir_std(dir, it);
    if (node != NULL)
        return node;

    // List mounted point!
    it->mode++;
    splock_lock(&dir->lock);
    node = ll_index(&dir->mnt, it->mode - 1, fnode_t, nmt);
    if (node != NULL)
        vfs_open_fnode(node);
    splock_unlock(&dir->lock);
    return node;
}

int vfs_closedir(fnode_t *dir, diterator_t *it)
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
