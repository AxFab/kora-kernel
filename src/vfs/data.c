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
#include <kernel/device.h>
#include <kernel/files.h>
#include <string.h>
#include <errno.h>



int vfs_read(inode_t *ino, char *buf, size_t size, off_t off, int flags)
{
    assert(kCPU.irq_semaphore == 0);
    assert(ino != NULL);
    assert(buf != NULL);
    if (size == 0) {
        errno = 0;
        return 0;
    }

    if (ino->type == FL_DIR || ino->type == FL_VOL || ino->type == FL_NET || ino->type == FL_VDO)
        assert(ino->ops->read == NULL);

#ifdef KORA_KRN
    if ((ino->type == FL_REG || ino->type == FL_BLK) && ino->ops->read == NULL) {
        kprintf(-1, "Unset IO block operations at %p\n", ino->ops);
        ino->ops->read = blk_read;
        ino->ops->write = blk_write;
    }
#endif

    if (ino->ops->read == NULL) {
        errno = ENOSYS;
        return -1;
    }
    return ino->ops->read(ino, buf, size, flags, off);
}

int vfs_write(inode_t *ino, const char *buf, size_t size, off_t off, int flags)
{
    assert(kCPU.irq_semaphore == 0);
    assert(ino != NULL);
    assert(buf != NULL);
    if (size == 0) {
        errno = 0;
        return 0;
    }

    if (ino->type == FL_DIR || ino->type == FL_VOL || ino->type == FL_NET || ino->type == FL_VDO || ino->type == FL_WIN)
        assert(ino->ops->read == NULL);

#ifdef KORA_KRN
    if ((ino->type == FL_REG || ino->type == FL_BLK) && ino->ops->read == NULL) {
        kprintf(-1, "Unset IO block operations at %p\n", ino->ops);
        ino->ops->read = blk_read;
        ino->ops->write = blk_write;
    }
#endif

    if (ino->dev->flags & VFS_RDONLY) {
        errno = EROFS;
        return -1;
    } else if (ino->ops->write == NULL) {
        errno = ENOSYS;
        return -1;
    }
    return ino->ops->write(ino, buf, size, flags, off);
}

