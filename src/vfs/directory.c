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
#include <kernel/vfs.h>
#include <assert.h>
#include <errno.h>


void *vfs_opendir(fsnode_t *dir, void *acl)
{
    assert(dir->mode == FN_OK);
    if (dir->ino->type != FL_DIR) {
        errno = ENOTDIR;
        return NULL;
        // } else if (vfs_access(dir, R_OK, acl) != 0) {
        //        errno = EACCES;
        //        return NULL;
    }

    return dir->ino->ops->opendir(dir->ino);
}

fsnode_t *vfs_readdir(fsnode_t *dir, void *ctx)
{
    assert(dir->mode == FN_OK);
    if (ctx == NULL) {
        errno = EINVAL;
        return NULL;
    }

    char *name = kalloc(256);
    inode_t *ino = dir->ino->ops->readdir(dir->ino, name, ctx);
    if (ino == NULL) {
        kfree(name);
        return NULL;
    }

    fsnode_t *node = vfs_fsnode_from(dir, name);
    if (node->mode == FN_OK) {
        vfs_close_inode(ino);
        kfree(name);
        return node;
    }

    mtx_lock(&node->mtx);
    // TODO - Check for weirdness
    node->ino = ino;
    node->mode = FN_OK;
    mtx_unlock(&node->mtx);
    kfree(name);
    return node;
}

int vfs_closedir(fsnode_t *dir, void *ctx)
{
    assert(dir->mode == FN_OK);
    if (ctx == NULL) {
        errno = EINVAL;
        return -1;
    }

    dir->ino->ops->closedir(dir->ino, ctx);
    return 0;
}

