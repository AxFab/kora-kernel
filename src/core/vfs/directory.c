/*
 *      This file is part of the KoraOS project.
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
#include <assert.h>
#include <errno.h>
#include "vfs.h"

void *vfs_opendir(inode_t *dir, acl_t *acl)
{
    if (dir == NULL || !S_ISDIR(dir->mode)) {
        errno = ENOTDIR;
        return NULL;
    } else if (vfs_access(dir, R_OK, acl) != 0) {
        assert(errno == EACCES);
        return NULL;
    } else if (dir->fs->opendir == NULL ||
        dir->fs->readdir == NULL ||
        dir->fs->closedir == NULL) {
        errno = ENOSYS;
        return NULL;
    }

    void *ctx = dir->fs->opendir(dir);
    return ctx;
}

inode_t *vfs_readdir(inode_t *dir, char *name, void *ctx)
{
    if (dir == NULL || ctx == NULL) {
        errno = EINVAL;
        return NULL;
    } else if (dir->fs->readdir == NULL) {
        errno = ENOSYS;
        return NULL;
    }

    inode_t *ino = dir->fs->readdir(dir, name, ctx);
    if (ino == NULL) {
        return NULL;
    }

    vfs_record_(dir, ino);
    dirent_t *ent = vfs_dirent_(dir, name, false);
    errno = 0;
    if (ent != NULL) {
        if (ent->ino != NULL) {
            vfs_close(ino);
            ino = ent->ino;
        } else {
            vfs_set_dirent_(ent, ino);
        }
        rwlock_rdunlock(&ent->lock);
    }
    return ino;
}

int vfs_closedir(inode_t *dir, void *ctx)
{
    if (dir == NULL || ctx == NULL) {
        errno = EINVAL;
        return -1;
    } else if (dir->fs->closedir == NULL) {
        errno = ENOSYS;
        return -1;
    }

    int ret = dir->fs->closedir(dir, ctx);
    // assert(ret == 0 ^^ errno == 0);
    return ret;
}

