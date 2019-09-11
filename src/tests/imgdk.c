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
#include <kernel/files.h>
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

int vhd_read(inode_t *ino, void *data, size_t size, off_t offset);
int vhd_write(inode_t *ino, const void *data, size_t size, off_t offset);
page_t vhd_fetch(inode_t *ino, off_t off);
void vhd_sync(inode_t *ino, off_t off, page_t pg);
void vhd_release(inode_t *ino, off_t off, page_t pg);


ino_ops_t imgdk_ino_ops = {
    .close = imgdk_close,
    .fetch = imgdk_fetch,
    .sync = imgdk_sync,
    .release = imgdk_release,
};

ino_ops_t vhd_ino_ops = {
    .close = imgdk_close,
    .fetch = vhd_fetch,
    .sync = vhd_sync,
    .release = vhd_release,
};

dev_ops_t imgdk_dev_ops = {
    .ioctl = (void*)imgdk_ioctl,
};

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

PACK(struct footer_vhd {
    char cookie[8];
    uint32_t features;
    uint16_t vers_maj;
    uint16_t vers_min;
    uint64_t offset;
    uint32_t timestamp;
    char creator_app[4];
    uint16_t creator_vers_maj;
    uint16_t creator_vers_min;
    char creator_host[4];
    uint64_t original_size;
    uint64_t actual_size;
    uint16_t cylinders;
    uint8_t heads;
    uint8_t sectors;
    uint32_t type;
    uint32_t checksum;
    char uuid[16];
    char state;
});


PACK(struct header_vhd {
    char cookie[8];
    uint64_t data_off;
    uint64_t table_off;
    uint16_t vers_maj;
    uint16_t vers_min;
    uint32_t entries_count;
    uint32_t block_size;
    uint32_t checksum;
});


struct vhd_info {
    uint32_t block_size;
    uint64_t data_off;
    uint64_t table_off;
    void *cache;
};


#define __swap16(v)  (  \
            ((v & 0xff) << 8) |  \
            ((v & 0xff00) >> 8) )

#define __swap32(v)  (  \
            ((v & 0xff) << 24) |  \
            ((v & 0xff00) << 8) |  \
            ((v & 0xff0000) >> 8) |  \
            ((v & 0xff000000) >> 24) )

#define __swap64(v)  (  \
            ((v & 0xffLL) << 48) |  \
            ((v & 0xff00LL) << 40) |  \
            ((v & 0xff0000LL) << 24) |  \
            ((v & 0xff000000LL) << 8) |  \
            ((v & 0xff00000000LL) >> 8) |  \
            ((v & 0xff0000000000LL) >> 24) |  \
            ((v & 0xff000000000000) >> 40) |  \
            ((v & 0xff00000000000000) >> 48) )


int imgdk_open(const char *path, const char *name)
{
    int fd = open(path, O_RDWR | O_BINARY);
    if (fd == -1)
        return -1;

    inode_t *ino = vfs_inode(fd, FL_BLK, NULL);
    ino->length = lseek(fd, 0, SEEK_END); // Optional
    // ino->lba = fd;

    ino->btime = cpu_clock(0);
    ino->ctime = ino->btime;
    ino->mtime = ino->btime;
    ino->atime = ino->btime;

    ino->ops = &imgdk_ino_ops;
    ino->dev->ops = &imgdk_dev_ops;

    if (strcmp(strrchr(path, '.'), ".img") == 0) {
        ino->dev->block = 512;
    } else if (strcmp(strrchr(path, '.'), ".iso") == 0) {
        ino->dev->block = 2048;
        ino->dev->flags = VFS_RDONLY;
    } else if (strcmp(strrchr(path, '.'), ".vhd") == 0) {
        ino->dev->block = 512;

        struct footer_vhd footer;
        size_t pos = lseek(fd, -512, SEEK_END);
        read(fd, &footer, sizeof(footer));

        footer.offset = __swap64(footer.offset);
        footer.vers_maj = __swap16(footer.vers_maj);
        footer.vers_min = __swap16(footer.vers_min);
        footer.timestamp = __swap32(footer.timestamp);
        footer.creator_vers_maj = __swap16(footer.creator_vers_maj);
        footer.creator_vers_min = __swap16(footer.creator_vers_min);
        footer.original_size = __swap64(footer.original_size);
        footer.actual_size = __swap64(footer.actual_size);
        footer.cylinders = __swap16(footer.cylinders);
        footer.type = __swap32(footer.type);
        memcpy(ino->dev->id, footer.uuid, 16);

        // Build Vendor string
        char vname[5];
        memcpy(vname, footer.creator_app, 4);
        int i = 3;
        vname[4] = '\0';
        while (i > 0 && vname[i] == ' ')
            vname[i--] = '\0';
        char vendor[64];
        snprintf(vendor, 64, "%s_%d.%d", vname, footer.creator_vers_maj, footer.creator_vers_min);
        ino->dev->vendor = strdup(vendor);

        if (footer.type == 2) {
            ino->dev->model = strdup("VHD-Fixed");
            ino->length -= 512;
        } else if (footer.type == 3) {
            ino->length = footer.actual_size;
            ino->dev->model = strdup("VHD-Dynamic");

            struct header_vhd header;

            size_t pos = lseek(fd, footer.offset, SEEK_SET);
            read(fd, &header, sizeof(header));

            header.data_off = __swap64(header.data_off);
            header.table_off = __swap64(header.table_off);
            header.vers_maj = __swap16(header.vers_maj);
            header.vers_min = __swap16(header.vers_min);
            header.entries_count = __swap32(header.entries_count);
            header.block_size = __swap32(header.block_size);

            struct vhd_info *info = malloc(sizeof(struct vhd_info));
            ino->info = info;
            info->data_off = header.data_off;
            info->table_off = header.table_off;
            info->block_size = header.block_size;
            info->cache = blk_create(ino, vhd_read, vhd_write);
            ino->ops = &vhd_ino_ops;
        } else {
            vfs_close(ino);
            close(fd);
            return -1;
        }
    }

    ino->dev->devclass = strdup("Image disk"); // Mandatory

    char *dname = strchr(path, '/');
    dname = dname == NULL ? path : dname + 1;
    ino->dev->devname = strdup(dname);
    // O ino->dev->devname
    // O ino->dev->model
    // O ino->dev->vendor

    if (ino->ops == &imgdk_ino_ops)
        ino->info = blk_create(ino, imgdk_read, imgdk_write);

    vfs_mkdev(ino, name);
    vfs_close(ino);
    return 0;
}

static void imgdk_tryopen(int i)
{
    int e;
    char fname[16];
    for (e = 0; e < 2; ++e) {
        snprintf(fname, 16, "sd%c.%s", 'A' + i, exts[e]);
        if (imgdk_open(fname, sdNames[i]) == 0)
            return;
    }
}

int imgdk_close(inode_t *ino)
{
    blk_destroy(ino->info);
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
    return blk_fetch(ino->info, off);
}

void imgdk_sync(inode_t *ino, off_t off, page_t pg)
{
    blk_sync(ino->info, off, pg);
}

void imgdk_release(inode_t *ino, off_t off, page_t pg)
{
    blk_release(ino->info, off, pg);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int vhd_read(inode_t *ino, void *data, size_t size, off_t offset)
{
    int fd = ino->no;
    struct vhd_info *info = ino->info;
    splock_lock(&ino->lock);
    while (size > 0) {
        size_t idx = offset / info->block_size;
        size_t bat_off = idx * 4 + info->table_off;
        uint32_t bat_val;
        lseek(fd, bat_off, SEEK_SET);
        read(fd, &bat_val, 4);
        size_t avail = MIN(size, info->block_size - offset % info->block_size);
        if (bat_val == ~0) {
            memset(data, 0, avail);
        } else {
            size_t dk_off = bat_val * 512 + 512 + offset % info->block_size;
            lseek(fd, dk_off, SEEK_SET);
            read(fd, data, avail);
        }
        size -= avail;
        data = ADDR_OFF(data, avail);
        offset -= avail;
    }
    splock_unlock(&ino->lock);
    return 0;
}

int vhd_write(inode_t *ino, const void *data, size_t size, off_t offset)
{
    int fd = ino->no;
    struct vhd_info *info = ino->info;
    assert((ino->flags & VFS_RDONLY) == 0);
    splock_lock(&ino->lock);
    while (size > 0) {
        size_t idx = offset / info->block_size;
        size_t bat_off = idx * 4 + info->table_off;
        uint32_t bat_val;
        lseek(fd, bat_off, SEEK_SET);
        read(fd, &bat_val, 4);
        size_t avail = MIN(size, info->block_size - offset % info->block_size);
        if (bat_val == ~0) {
            // ... Need allocate !
            // lseek -512 SEEK_END
            // WRITE block_size !
            // COPY FOOTER
            // WRITE BAT entry !
            return -1;
        }

        size_t dk_off = bat_val * 512 + 512 + offset % info->block_size;
        lseek(fd, dk_off, SEEK_SET);
        write(fd, data, avail);

        size -= avail;
        data = ADDR_OFF(data, avail);
        offset -= avail;
    }
    splock_unlock(&ino->lock);
    return 0;
}

page_t vhd_fetch(inode_t *ino, off_t off)
{
    struct vhd_info *info = ino->info;
    return blk_fetch(info->cache, off);
}

void vhd_sync(inode_t *ino, off_t off, page_t pg)
{
    struct vhd_info *info = ino->info;
    blk_sync(info->cache, off, pg);
}

void vhd_release(inode_t *ino, off_t off, page_t pg)
{
    struct vhd_info *info = ino->info;
    blk_release(info->cache, off, pg);
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

uint32_t vhd_checksum(char *buf, int len)
{
    uint32_t checksum = 0;
    for (int i = 0; i < len; ++i) {
        checksum += buf[i];
    }
    return __swap32(~checksum);
}

void vhd_create_dyn(CSTR name, size_t size)
{
    int i;
    char buf[512];
    int fd = open(name, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, 0644);

    int blocks = size / (2 * _Mib_);
    int cnt = 4 + ALIGN_UP(blocks * 4, 512) / 512;
    printf("write %d sectors\n", cnt);
    memset(buf, 0xff, 512);
    for (i = 0; i < cnt; ++i)
        write(fd, buf, 512);

    struct footer_vhd *footer = (void*)&buf;

    int sectors = MIN(size / 512, 65535 * 16 * 255);
    memset(buf, 0, 512);
    memcpy(footer->cookie, "conectix", 8);
    footer->features = 0x2000000;
    footer->vers_maj = __swap16(1);
    footer->vers_min = __swap16(0);
    footer->offset = __swap64(512);
    footer->timestamp = 0;
    memcpy(footer->creator_app, "kora", 4);
    footer->creator_vers_maj = __swap16(1);
    footer->creator_vers_min = __swap16(0);
    memcpy(footer->creator_host, "kora", 4);
    footer->original_size = __swap64(size);
    footer->actual_size = __swap64(size);
    if (sectors > 65535 * 16 * 63) {
        footer->cylinders = __swap16(sectors / 16 / 255);
        footer->heads = 16;
        footer->sectors = 255;
    } else {
        int cys = sectors / 17;
        footer->sectors = 17;
        footer->heads = MAX(4, (cys + 1023) / 1024);
        if (footer->heads > 16 || cys > 1024 * footer->heads) {
            footer->sectors = 31;
            footer->heads = 16;
            cys = sectors / 31;
        }
        if (cys > 1024 * footer->heads) {
            footer->sectors = 63;
            footer->heads = 16;
            cys = sectors / 63;
        }
        footer->cylinders = __swap16(cys / footer->heads);
    }
    footer->type = __swap32(3);
    for (i = 0; i < 16; ++i)
        footer->uuid[i] = rand8();
    footer->checksum = vhd_checksum(footer, sizeof(struct footer_vhd));

    lseek(fd, 0, SEEK_SET);
    write(fd, buf, 512);
    lseek(fd, (cnt - 1) * 512, SEEK_SET);
    write(fd, buf, 512);
    lseek(fd, 512, SEEK_SET);

    struct header_vhd *header = (void*)&buf;

    memset(buf, 0, 512);
    memcpy(header->cookie, "cxsparse", 8);

    header->data_off = ~0ULL;
    header->table_off = __swap64(1536);
    header->vers_maj = __swap16(1);
    header->vers_min = __swap16(0);
    header->entries_count = __swap32(4);
    header->block_size = __swap32(2 * _Mib_);
    header->checksum = vhd_checksum(header, sizeof(struct header_vhd));
    write(fd, buf, 512);

    memset(buf, 0, 512);
    write(fd, buf, 512);

    close(fd);
}

void imgdk_setup()
{
    imgdk_create("sdA.img", 16 * _Mib_);
    int i;
    for (i = 0; i < 4; ++i)
        imgdk_tryopen(i);
}

void imgdk_teardown()
{
    int i;
    for (i = 0; i < 4; ++i)
        vfs_rmdev(sdNames[i]);

}

MODULE(imgdk, imgdk_setup, imgdk_teardown);
