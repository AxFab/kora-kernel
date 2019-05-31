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

static int vfs_read_block(inode_t *ino, char *buf, size_t len, off_t off)
{
    if (off >= ino->length && ino->length != 0)
        return 0;
    off_t poff = -1;
    char *map = NULL;
    int bytes = 0;
    while (len > 0) {
        off_t po = ALIGN_DW(off, PAGE_SIZE);
        if (poff != po) {
            if (map != NULL)
                kunmap(map, PAGE_SIZE);
            poff = po;
            map = kmap(PAGE_SIZE, ino, poff, VMA_FILE_RO | VMA_RESOLVE);
            if (map == NULL)
                return -1;
        }
        size_t disp = (size_t)(off & (PAGE_SIZE - 1));
        int cap = MIN3((size_t)len, PAGE_SIZE - disp, (size_t)(ino->length - off));
        if (cap == 0)
            return bytes;
        memcpy(buf, map + disp, cap);
        len -= cap;
        off += cap;
        buf += cap;
        bytes += cap;
    }
    kunmap(map, PAGE_SIZE);
    return bytes;
}

static int vfs_write_block(inode_t *ino, const char *buf, size_t len, off_t off)
{
    if (off >= ino->length && ino->length != 0)
        return 0;
    off_t poff = -1;
    char *map = NULL;
    int bytes = 0;
    while (len > 0) {
        off_t po = ALIGN_DW(off, PAGE_SIZE);
        if (poff != po) {
            if (map != NULL)
                kunmap(map, PAGE_SIZE);
            poff = po;
            map = kmap(PAGE_SIZE, ino, poff, VMA_FILE_RW | VMA_RESOLVE);
            if (map == NULL)
                return -1;
        }
        size_t disp = (size_t)(off & (PAGE_SIZE - 1));
        int cap = MIN3((size_t)len, PAGE_SIZE - disp, (size_t)(ino->length - off));
        if (cap == 0)
            return bytes;
        memcpy(map + disp, buf, cap);
        len -= cap;
        off += cap;
        buf += cap;
        bytes += cap;
    }
    kunmap(map, PAGE_SIZE);
    return bytes;
}


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


int vfs_read(inode_t *ino, char *buf, size_t size, off_t off, int flags)
{
    assert(kCPU.irq_semaphore == 0);
    assert(ino != NULL);
    assert(buf != NULL);
    if (size == 0) {
        errno = 0;
        return 0;
    }

    switch (ino->type) {
    case FL_REG:
    case FL_BLK:
        return vfs_read_block(ino, buf, size, off);
    case FL_PIPE:
    case FL_CHR:
    case FL_LNK:
    case FL_INFO:
    case FL_WIN:
    case FL_TTY:
        if (ino->ops->read == NULL) {
            errno = ENOSYS;
            return -1;
        }
        return ino->ops->read(ino, buf, size, flags);
    case FL_SOCK:
    default: // DIR, VOL, NET, VDO
        errno = ENOSYS;
        return -1;
    }
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

    switch (ino->type) {
    case FL_REG:
    case FL_BLK:
        return vfs_write_block(ino, buf, size, off);
    case FL_PIPE:
    case FL_CHR:
    case FL_LNK:
    case FL_INFO:
        if (ino->dev->flags & VFS_RDONLY) {
            errno = EROFS;
            return -1;
        } else if (ino->ops->write == NULL) {
            errno = ENOSYS;
            return -1;
        }
        return ino->ops->write(ino, buf, size, flags);
    case FL_TTY:
        if (ino->ops->write == NULL) {
            errno = ENOSYS;
            return -1;
        }
        return ino->ops->write(ino, buf, size, flags);
    case FL_SOCK:
    default: // DIR, VOL, NET, VDO, WIN
        errno = ENOSYS;
        return -1;
    }
}

