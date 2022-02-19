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
#include <stdio.h>
#include <kernel/vfs.h>
#include <kora/mcrs.h>
#include <kora/splock.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#ifndef O_BINARY
# define O_BINARY 0
#endif

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

const char *sdNames[] = { "sdA", "sdB", "sdC", "sdD" };
const char *exts[] = { "img", "iso", };
const char *clazz[] = { "IDE ATA", "IDE ATAPI", };
const int sdSize[] = { 512, 2048, };

int imgdk_read(inode_t *ino, void *data, size_t size, xoff_t offset);
int imgdk_write(inode_t *ino, const void *data, size_t size, xoff_t offset);
int imgdk_ioctl(inode_t *ino, int cmd, void **params);
int imgdk_close(inode_t* ino);

int vhd_read(inode_t *ino, void *data, size_t size, xoff_t offset);
int vhd_write(inode_t *ino, const void *data, size_t size, xoff_t offset);

ino_ops_t imgdk_ino_ops = {
    .close = (void *)imgdk_close,
    .read = (void *)imgdk_read,
    .write = (void *)imgdk_write,
    .ioctl = imgdk_ioctl,
};

ino_ops_t vhd_ino_ops = {
    .close = (void *)imgdk_close,
    .read = (void *)vhd_read,
    .write = (void *)vhd_write,
    .ioctl = imgdk_ioctl,
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
    uint64_t disk_size;
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

    inode_t *ino = NULL;
    if (strcmp(strrchr(path, '.'), ".img") == 0) {
        ino = vfs_inode(fd, FL_BLK, NULL, &imgdk_ino_ops);
        ino->length = lseek(fd, 0, SEEK_END);
        if (ino->length == 0) {
            struct stat st;
            int ret = stat(path, &st);
            if (ret == 0)
                ino->length = st.st_size;
        }
        ino->dev->block = 512;
        if (ino->length == 1440 * 1024)
            ino->dev->model = kstrdup("Floppy-img");
        else
            ino->dev->model = kstrdup("RAW-image");
    } else if (strcmp(strrchr(path, '.'), ".iso") == 0) {
        ino = vfs_inode(fd, FL_BLK, NULL, &imgdk_ino_ops);
        ino->length = lseek(fd, 0, SEEK_END);
        ino->dev->block = 2048;
        ino->dev->flags = FD_RDONLY;
        ino->dev->model = kstrdup("ISO-image");
    } else if (strcmp(strrchr(path, '.'), ".vhd") == 0) {
        ino = vfs_inode(fd, FL_BLK, NULL, &vhd_ino_ops);
        ino->dev->block = 512;

        struct footer_vhd footer;
        // Might be of size 511 only (like what the fuck !!!)
        // size_t pos =
        lseek(fd, -512, SEEK_END);
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
        memcpy(ino->dev->uuid, footer.uuid, 16);
        // if offset == 0xFFFFFFFF => Fixed disk !!!
        // Build Vendor string
        char vname[5];
        memcpy(vname, footer.creator_app, 4);
        int i = 3;
        vname[4] = '\0';
        while (i > 0 && vname[i] == ' ')
            vname[i--] = '\0';
        char vendor[64];
        snprintf(vendor, 64, "%s_%d.%d", vname, footer.creator_vers_maj, footer.creator_vers_min);
        ino->dev->vendor = kstrdup(vendor);
        // Check cookie and checksum

        if (footer.type == 2) {
            ino->dev->model = kstrdup("VHD-Fixed");
            ino->length -= 512;
        } else if (footer.type == 3) {
            ino->length = footer.actual_size;
            ino->dev->model = kstrdup("VHD-Dynamic");

            struct header_vhd header;

            // size_t pos =
            lseek(fd, footer.offset, SEEK_SET);
            read(fd, &header, sizeof(header));

            // Check cookie and checksum

            header.data_off = __swap64(header.data_off);
            header.table_off = __swap64(header.table_off);
            header.vers_maj = __swap16(header.vers_maj);
            header.vers_min = __swap16(header.vers_min);
            header.entries_count = __swap32(header.entries_count);
            header.block_size = __swap32(header.block_size); // Should be 2 Mb !

            struct vhd_info *info = malloc(sizeof(struct vhd_info));
            ino->drv_data = info;
            info->data_off = header.data_off;
            info->table_off = header.table_off;
            info->block_size = header.block_size;
            info->disk_size = footer.actual_size;
        } else {
            vfs_close_inode(ino);
            close(fd);
            return -1;
        }
    }

    if (ino == NULL) {
        close(fd);
        return -1;
    }

    ino->btime = xtime_read(XTIME_CLOCK);
    ino->ctime = ino->btime;
    ino->mtime = ino->btime;
    ino->atime = ino->btime;
    ino->mode = 0644;
    ino->dev->devclass = kstrdup("Image disk");

    const char *dname = strchr(path, '/');
    dname = dname == NULL ? path : dname + 1;
    ino->dev->devname = kstrdup(dname);
    // O ino->dev->devname
    // O ino->dev->model
    // O ino->dev->vendor

    vfs_mkdev(ino, name);
    vfs_close_inode(ino);
    return 0;
}

int imgdk_close(inode_t* ino)
{
    close(ino->no);
    return 0;
}

int imgdk_ioctl(inode_t *ino, int cmd, void **params)
{
    errno = 0;
    return 0;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int imgdk_read(inode_t *ino, void *data, size_t size, xoff_t offset)
{
    int fd = ino->no;
    errno = 0;
    splock_lock(&ino->lock);
    lseek(fd, offset, SEEK_SET);
    int r = read(fd, data, size);
    if (errno != 0 || r != (int)size)
        kprintf(KL_ERR, "[IMG ] Read err: %s\n", strerror(errno));
    splock_unlock(&ino->lock);
    return 0;
}

int imgdk_write(inode_t *ino, const void *data, size_t size, xoff_t offset)
{
    assert(ino->dev->block != 2048);
    assert((ino->dev->flags & FD_RDONLY) == 0);
    int fd = ino->no;
    errno = 0;
    splock_lock(&ino->lock);
    lseek(fd, offset, SEEK_SET);
    // kprintf(-1, "IMG %2d write %012llx %6x\n", fd, offset, size);
    int r = write(fd, data, size);
    if (errno != 0 || r != (int)size)
        kprintf(KL_ERR, "[IMG ] Write err: %s\n", strerror(errno));
    splock_unlock(&ino->lock);
    return 0;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

#if 0
static struct vhd_info *vhd_open(int fd) {

    struct footer_vhd footer;
    // Might be of size 511 only (like what the fuck !!!)
    // size_t pos =
    lseek(fd, -512, SEEK_END);
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
    if (footer.type != 2 && footer.type != 3)
        return NULL;

    // memcpy(ino->dev->uuid, footer.uuid, 16);
    // if offset == 0xFFFFFFFF => Fixed disk !!!
    // Build Vendor string
    char vname[5];
    memcpy(vname, footer.creator_app, 4);
    int i = 3;
    vname[4] = '\0';
    while (i > 0 && vname[i] == ' ')
        vname[i--] = '\0';
    char vendor[64];
    snprintf(vendor, 64, "%s_%d.%d", vname, footer.creator_vers_maj, footer.creator_vers_min);
    // ino->dev->vendor = strdup(vendor);
    // Check cookie and checksum

    struct vhd_info* info = malloc(sizeof(struct vhd_info));

    if (footer.type == 2) {
        // ino->dev->model = strdup("VHD-Fixed");
        // ino->length -= 512;
        return info;
    }

    // ino->length = footer.actual_size;
    // ino->dev->model = strdup("VHD-Dynamic");

    struct header_vhd header;

    pos = lseek(fd, footer.offset, SEEK_SET);
    read(fd, &header, sizeof(header));

    // Check cookie and checksum

    header.data_off = __swap64(header.data_off);
    header.table_off = __swap64(header.table_off);
    header.vers_maj = __swap16(header.vers_maj);
    header.vers_min = __swap16(header.vers_min);
    header.entries_count = __swap32(header.entries_count);
    header.block_size = __swap32(header.block_size); // Should be 2 Mb !

    // ino->drv_data = info;
    info->data_off = header.data_off;
    info->table_off = header.table_off;
    info->block_size = header.block_size;
    info->disk_size = footer.actual_size;
    return info;

}
#endif

static uint32_t vhd_checksum(void* buf, int len)
{
    unsigned char* b = buf;
    uint32_t checksum = 0;
    for (int i = 0; i < len; ++i)
        checksum += b[i];
    return __swap32(~checksum);
}

static void vhd_fill_footer(int fd, struct footer_vhd* footer, uint64_t disk_size)
{
    int i;
    uint64_t sectors = MIN(disk_size / 512, 65535 * 16 * 255);
    memcpy(footer->cookie, "conectix", 8);
    footer->features = 0x2000000;
    footer->vers_maj = __swap16(1);
    footer->vers_min = __swap16(0);
    footer->offset = __swap64(512);
    footer->timestamp = 0; // TODO - UNIX TIMESTAMP !!!
    memcpy(footer->creator_app, "kora", 4);
    footer->creator_vers_maj = __swap16(1);
    footer->creator_vers_min = __swap16(0);
    memcpy(footer->creator_host, "kora", 4);
    footer->original_size = __swap64(disk_size);
    footer->actual_size = __swap64(disk_size);
    if (sectors > 65535 * 16 * 63) {
        footer->cylinders = __swap16(MIN(sectors / 16 / 255, 65535));
        footer->heads = 16;
        footer->sectors = 255;
    }
    else {
        int cys = (int)sectors / 17;
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
    footer->checksum = 0;
    footer->checksum = vhd_checksum(footer, sizeof(struct footer_vhd));
}

static uint32_t vhd_alloc_block(int fd, struct vhd_info* info, uint64_t bat_off)
{
    // Extends the disk image
    unsigned imgSize = lseek(fd, -512, SEEK_END);
    unsigned lba = imgSize / 512;
    char buf[512];
    memset(buf, 0, 512);
    for (unsigned i = 0; i < info->block_size / 512 + 1; ++i)
        write(fd, buf, 512);

    // Rewrite footer
    // struct footer_vhd* footer = buf;
    //footer. TODO
    write(fd, buf, 512);
    lseek(fd, 0, SEEK_SET);
    write(fd, buf, 512);

    // Write BAT entry
    lseek(fd, bat_off, SEEK_SET);
    write(fd, &lba, 4);

    return lba / 512; // ~0;
}

int vhd_read(inode_t *ino, void *data, size_t size, xoff_t offset)
{
    int fd = ino->no;
    struct vhd_info *info = ino->drv_data;
    splock_lock(&ino->lock);
    while (size > 0) {
        uint64_t idx = offset / info->block_size; // Block number
        uint64_t bat_off = idx * 4 + info->table_off;
        uint32_t bat_val;
        lseek(fd, bat_off, SEEK_SET);
        read(fd, &bat_val, 4);
        size_t avail = MIN(size, info->block_size - (size_t)(offset % (xoff_t)info->block_size));
        if (bat_val == ~0U)
            memset(data, 0, avail);
        else {
            size_t dk_off = bat_val * 512 + 512 + (size_t)(offset % (xoff_t)info->block_size); // BAT [BlockNumber] + BlockBitmapSectorCount + SectorInBlock
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

int vhd_write(inode_t *ino, const void *data, size_t size, xoff_t offset)
{
    int fd = ino->no;
    struct vhd_info *info = ino->drv_data;
    splock_lock(&ino->lock);
    while (size > 0) {
        uint64_t idx = offset / info->block_size;
        uint64_t bat_off = idx * 4 + info->table_off;
        uint32_t bat_val;
        lseek(fd, (off_t)bat_off, SEEK_SET);
        read(fd, &bat_val, 4);
        size_t avail = MIN(size, info->block_size - (size_t)(offset % (xoff_t)info->block_size));
        if (bat_val == ~0U) {
            bat_val = vhd_alloc_block(fd, info, bat_off);
            if (bat_val == ~0U) {
                splock_unlock(&ino->lock);
                return -1;
            }
        }

        size_t dk_off = bat_val * 512 + 512 + (size_t)(offset % (xoff_t)info->block_size); // TODO -> blockBitmapSize = (ALIGN_UP(blockSize,2Mb)/2Mb)*512
        lseek(fd, dk_off, SEEK_SET);
        write(fd, data, avail);

        size -= avail;
        data = ADDR_OFF(data, avail);
        offset -= avail;
        // TODO -- Write 1 on block bitmap!
    }
    splock_unlock(&ino->lock);
    return 0;
}


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
void imgdk_create(const char *name, size_t size)
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

void vhd_create_dyn(const char *name, uint64_t size)
{
    unsigned i;
    char buf[512];
    unsigned bsize = 2 * _Mib_;
    unsigned blocks = size / bsize;
    unsigned cnt = 4 + ALIGN_UP(blocks * 4, 512) / 512;
    int fd = open(name, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, 0644);

    // Write block table
    printf("write %d sectors\n", cnt);
    memset(buf, 0xff, 512);
    for (i = 0; i < cnt; ++i)
        write(fd, buf, 512);

    // Write footer (first and last sector)
    struct footer_vhd *footer = (void *)&buf;
    memset(buf, 0, 512);
    vhd_fill_footer(fd, footer, size);

    lseek(fd, 0, SEEK_SET);
    write(fd, buf, 512);
    lseek(fd, (cnt - 1) * 512, SEEK_SET);
    write(fd, buf, 512);
    lseek(fd, 512, SEEK_SET);

    // Write header
    struct header_vhd *header = (void *)&buf;
    memset(buf, 0, 512);
    memcpy(header->cookie, "cxsparse", 8);
    header->data_off = ~0ULL;
    header->table_off = __swap64(1536); // Offset of BAT in bytes
    header->vers_maj = __swap16(1);
    header->vers_min = __swap16(0);
    header->entries_count = __swap32(blocks);
    header->block_size = __swap32(bsize);
    header->checksum = vhd_checksum(header, sizeof(struct header_vhd));
    write(fd, buf, 512);

    // Write sector
    memset(buf, 0, 512);
    write(fd, buf, 512);

    close(fd);
}
