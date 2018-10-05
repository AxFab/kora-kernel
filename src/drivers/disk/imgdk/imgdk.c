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
 *
 *      Fake driver which simulate IDE ATA using regular files.
 */
#include <kernel/device.h>
#include <kora/mcrs.h>
#include <kora/splock.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#define SEEK_SET 0 /* Seek from beginning of file.  */
#define SEEK_CUR 1 /* Seek from current position.  */
#define SEEK_END 2 /* Seek from end of file.  */

#if defined _WIN32
int open(const char *name, int flags);
#else
#define O_BINARY 0
#endif
int read(int fd, char *buf, int len);
int write(int fd, const char *buf, int len);
int lseek(int fd, off_t off, int whence);
void close(int fd);

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

struct IMGDK_Drive {
    blkdev_t dev;
    int fd;
    splock_t lock;
};

struct IMGDK_Drive sdx[4];

const char *sdNames[] = { "sdA", "sdB", "sdC", "sdD" };
const char *exts[] = { "img", "iso", };
const char *clazz[] = { "IDE ATA", "IDE ATAPI", };
const int sdSize[] = { 512, 2048, };

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int imgdk_read(inode_t *ino, void *data, size_t size, off_t offset);
int imgdk_write(inode_t *ino, const void *data, size_t size, off_t offset);

static void imgdk_open(int i)
{
    int e;
    char fname[16];
    for (e = 0; e < 2; ++e) {
        snprintf(fname, 16, "sd%c.%s", 'A' + i, exts[e]);
        int fd = open(fname, O_RDWR | O_BINARY);
        if (fd == -1) {
            sdx[i].fd = -1;
            continue;
        }

        size_t sz = lseek(fd, 0, SEEK_END);
        sdx[i].fd = fd;

        inode_t *blk = vfs_inode(i, S_IFBLK | 0500, NULL, 0);
        blk->length = sz;
        blk->lba = i;

        sdx[i].dev.dev.read_only = e > 0;
        sdx[i].dev.dev.is_detached = false;
        sdx[i].dev.block = sdSize[e];
        sdx[i].dev.dev.vendor = "HostSimul";
        sdx[i].dev.class = clazz[e];
        sdx[i].dev.read = imgdk_read;
        sdx[i].dev.write = imgdk_write;

        vfs_mkdev(sdNames[i], (device_t *)&sdx[i].dev, blk);
        vfs_close(blk);
        break;
    }
}

static void imgdk_exit(int i)
{
    if (sdx[i].fd >= 0) {
        close(sdx[i].fd);
        memset(&sdx[i], 0, sizeof(*sdx));
        vfs_rmdev(sdNames[i]);
    }
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int imgdk_read(inode_t *ino, void *data, size_t size, off_t offset)
{
    int fd = sdx[ino->lba].fd;
    if (fd == 0) {
        errno = ENODEV;
        return -1;
    }
    errno = 0;
    splock_lock(&sdx[ino->lba].lock);
    lseek(fd, offset, SEEK_SET);
    int r = read(fd, data, size);
    if (errno != 0 || r != (int)size)
        kprintf(KLOG_ERR, "[IMG ] Read err: %s\n", strerror(errno));
    splock_unlock(&sdx[ino->lba].lock);
    return 0;
}

int imgdk_write(inode_t *ino, const void *data, size_t size, off_t offset)
{
    int fd = sdx[ino->lba].fd;
    if (fd == 0) {
        errno = ENODEV;
        return -1;
    } else if (sdx[ino->lba].dev.dev.read_only) {
        errno = EROFS;
        return -1;
    }
    errno = 0;
    splock_lock(&sdx[ino->lba].lock);
    lseek(fd, offset, SEEK_SET);
    int r = write(fd, data, size);
    if (errno != 0 || r != (int)size)
        kprintf(KLOG_ERR, "[IMG ] Write err: %s\n", strerror(errno));
    splock_unlock(&sdx[ino->lba].lock);
    return 0;
}

int imgdk_ioctl(inode_t *ino, int cmd, long *params)
{
    errno = 0;
    return 0;
}

void imgdk_release_dev(struct IMGDK_Drive *dev)
{
    close(dev->fd);
    dev->fd = -1;
}

void imgdk_create(CSTR name, size_t size) {
    int zero = 0;
    int fd = open(name, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC);
    if (fd != -1) {
        lseek(fd, size - 1, SEEK_SET);
        write(fd, &zero, 1);
        close(fd);
    } else {
        
    }
}

void imgdk_setup()
{
    imgdk_create("sdA.img", 16 * _Mib_);
    int i;
    for (i = 0; i < 4; ++i)
        imgdk_open(i);
}

void imgdk_teardown()
{
    int i;
    for (i = 0; i < 4; ++i)
        imgdk_exit(i);
}

MODULE(imgdk, imgdk_setup, imgdk_teardown);
