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
#include <string.h>
#include <errno.h>
#include <assert.h>

void *block_create();
void *pipe_create();
void *sock_create();
void *framebuffer_create();
void *socket_create();

extern fl_ops_t block_ops;
extern fl_ops_t pipe_ops;
extern fl_ops_t chr_ops;
extern fl_ops_t sock_ops;
extern fl_ops_t framebuffer_ops;
extern fl_ops_t socket_ops;

void vfs_createfile(inode_t *ino)
{
    switch (ino->type) {
    case FL_REG:
    case FL_BLK:
        ino->fl_data = block_create();
        ino->fops = &block_ops;
        break;
    case FL_PIPE:
        ino->fl_data = pipe_create();
        ino->fops = &pipe_ops;
        break;
    case FL_CHR:
        ino->fl_data = pipe_create();
        ino->fops = &chr_ops;
        break;
    case FL_SOCK:
    //ino->fl_data = socket_create();
    //ino->fops = &socket_ops;
    //break;
    case FL_FRM:
    case FL_NET:
    case FL_LNK:
    case FL_DIR:
    default:
        break;
    }
}



/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int vfs_read(inode_t *ino, char *buf, size_t size, xoff_t off, int flags)
{
    assert(ino != NULL && buf != NULL);
    if (ino->fops == NULL || ino->fops->read == NULL) {
        errno = ENOSYS;
        return -1;
    }

    return ino->fops->read(ino, buf, size, off, flags);
}
EXPORT_SYMBOL(vfs_read, 0);

int vfs_write(inode_t *ino, const char *buf, size_t size, xoff_t off, int flags)
{
    assert(ino != NULL && buf != NULL);
    if (ino->fops == NULL || ino->fops->write == NULL) {
        errno = ENOSYS;
        return -1;
    }

    return ino->fops->write(ino, buf, size, off, flags);
}
EXPORT_SYMBOL(vfs_write, 0);

int vfs_truncate(inode_t *ino, xoff_t off)
{
    assert(ino != NULL);
    if (ino->type == FL_DIR) {
        errno = EISDIR;
        return -1;
    } else if (ino->ops == NULL || ino->ops->truncate == NULL) {
        errno = ENOSYS;
        return -1;
    }

    int ret = ino->ops->truncate(ino, off);
    assert((ret != 0) != (ino->length == off));
    return ret;
}
EXPORT_SYMBOL(vfs_truncate, 0);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int vfs_ioctl(inode_t *ino, int cmd, void **args)
{
    assert(ino != NULL);
    if (ino->ops->ioctl == NULL) {
        errno = ENOSYS;
        return -1;
    }

    return ino->ops->ioctl(ino, cmd, args);
}
EXPORT_SYMBOL(vfs_ioctl, 0);


size_t vfs_fetch_page(inode_t *ino, xoff_t off, bool blocking)
{
    if (ino->ops->fetch)
        return ino->ops->fetch(ino, off, blocking);
    else if (ino->type == FL_REG || ino->type == FL_BLK)
        return block_fetch(ino, off, blocking);
    return 0;
}

int vfs_release_page(inode_t *ino, xoff_t off, size_t pg, bool dirty)
{
    if (ino->ops->release)
        return ino->ops->release(ino, off, pg, dirty);
    else if (ino->type == FL_REG || ino->type == FL_BLK)
        return block_release(ino, off, pg, dirty);
    return 0;
}

