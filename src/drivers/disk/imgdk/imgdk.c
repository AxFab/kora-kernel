/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2018  <Fabien Bavent>
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

const char *sdNames[] = { "sdA", "sdB", "sdC", "sdD" };
const char *exts[] = { "img", "iso", };
const char *clazz[] = { "IDE ATA", "IDE ATAPI", };
const int sdSize[] = { 512, 2048, };

int imgdk_read(inode_t *ino, void *data, size_t size, off_t offset);
int imgdk_write(inode_t *ino, const void *data, size_t size, off_t offset);
page_t imgdk_fetch(inode_t *ino, off_t off);
void imgdk_sync(inode_t *ino, off_t off, page_t pg);
void imgdk_release(inode_t *ino, off_t off, page_t pg);
int imgdk_ioctl(inode_t *ino, int cmd, long *params);
int imgdk_close(inode_t *ino);

ino_ops_t imgdk_ino_ops = {
    .read = imgdk_read,
    .write = imgdk_write,
    .close = imgdk_close,
    .fetch = imgdk_fetch,
    .sync = imgdk_sync,
    .release = imgdk_release,
};

dev_ops_t imgdk_dev_ops = {
    .ioctl = imgdk_ioctl,
};

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

static void imgdk_open(int i)
{
    int e;
    char fname[16];
    for (e = 0; e < 2; ++e) {
        snprintf(fname, 16, "sd%c.%s", 'A' + i, exts[e]);
        int fd = open(fname, O_RDWR | O_BINARY);
        if (fd == -1)
            continue;

        inode_t *blk = vfs_inode(fd, FL_BLK, NULL);
        blk->length = lseek(fd, 0, SEEK_END);
        blk->lba = i;
        if (e > 0)
            blk->und.dev->flags = VFS_RDONLY;
        blk->und.dev->block = sdSize[e];
        blk->und.dev->vendor = (char *)"HostSimul";
        blk->und.dev->model = "HostSimul";
        blk->und.dev->devclass = (char *)clazz[e];
        blk->und.dev->devname = sdNames[i];
        blk->und.dev->ops = &imgdk_dev_ops;
        blk->ops = &imgdk_ino_ops;
        blk->info = map_create(blk, imgdk_read, imgdk_write);
        vfs_mkdev(blk, sdNames[i]);
        vfs_close(blk);
        break;
    }
}

int imgdk_close(inode_t *ino)
{
    map_destroy(ino->info);
    close(ino->no);
}

int imgdk_ioctl(inode_t *ino, int cmd, long *params)
{
    errno = 0;
    return 0;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int imgdk_read(inode_t *ino, void *data, size_t size, off_t offset)
{
    int fd = ino->no;
    errno = 0;
    splock_lock(&ino->lock);
    lseek(fd, offset, SEEK_SET);
    int r = read(fd, data, size);
    if (errno != 0 || r != (int)size)
        kprintf(KLOG_ERR, "[IMG ] Read err: %s\n", strerror(errno));
    splock_unlock(&ino->lock);
    return 0;
}

int imgdk_write(inode_t *ino, const void *data, size_t size, off_t offset)
{
    int fd = ino->no;
    assert((ino->flags & VFS_RDONLY) == 0);
    errno = 0;
    splock_lock(&ino->lock);
    lseek(fd, offset, SEEK_SET);
    int r = write(fd, data, size);
    if (errno != 0 || r != (int)size)
        kprintf(KLOG_ERR, "[IMG ] Write err: %s\n", strerror(errno));
    splock_unlock(&ino->lock);
    return 0;
}

page_t imgdk_fetch(inode_t *ino, off_t off)
{
    return map_fetch(ino->info, off);
}

void imgdk_sync(inode_t *ino, off_t off, page_t pg)
{
    map_sync(ino->info, off, pg);
}

void imgdk_release(inode_t *ino, off_t off, page_t pg)
{
    map_release(ino->info, off, pg);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
void imgdk_create(CSTR name, size_t size)
{
    int zero = 0;
    int fd = open(name, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, 0644);
    if (fd != -1) {
        lseek(fd, size - 1, SEEK_SET);
        write(fd, (char *)&zero, 1);
        close(fd);
    } else
        kprintf(-1, "Unable to create image disk %s\n", name);
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
        vfs_rmdev(sdNames[i]);

}

MODULE(imgdk, imgdk_setup, imgdk_teardown);
