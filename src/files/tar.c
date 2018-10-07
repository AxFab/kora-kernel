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
 */
#include <kernel/core.h>
#include <kernel/device.h>
#include <kernel/vfs.h>
#include <string.h>
#include <errno.h>


#define TAR_BLOCK_SIZE  512
typedef struct tar_entry tar_entry_t;

struct tar_entry {
    char name[100];
    char filemode[8];
    char user_id[8];
    char group_id[8];
    char file_size[12];
    char last_mode_time[12];
    char checksum[8];
    char type_flag[1];
    char symlink[100];
    char magik[6];
    char version[2];
    char owner_user[32];
    char owner_group[32];
    char dev_major[8];
    char dev_minor[8];
    char filename_prefix[155];
};

static int tar_read_octal(char* count)
{
    int i;
    int val = 0;
    for (i = 0; count[i] != '\0'; ++i) {
        val *= 8;
        val += count[i] - '0';
    }
    return val;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

struct tar_info {
    void *start;
    int length;

} tinfo;

inode_t *tar_inode(tar_entry_t *entry, int length)
{
    int lba = ((void*)entry - tinfo.start);
    inode_t *ino = vfs_inode(lba / TAR_BLOCK_SIZE + 2, S_IFREG | 0777, NULL, 0);
    ino->length = length;
    ino->lba = lba;
    return ino;
}

int tar_umount(inode_t *ino)
{
    return -1;
}


int tar_read(inode_t *ino, void *buf, size_t len, off_t off)
{
    tar_entry_t *entry = ADDR_OFF(tinfo.start, ino->lba);
    uint8_t *data = ADDR_OFF(entry, TAR_BLOCK_SIZE);
    int cap = len;
    if (len + off > ino->length)
        cap = ino->length - off;
    memcpy(buf, &data[off], cap);
    return 0;
}

int tar_write(inode_t *ino, const void *buf, size_t len, off_t off)
{
    tar_entry_t *entry = ADDR_OFF(tinfo.start, ino->lba);
    uint8_t *data = ADDR_OFF(entry, TAR_BLOCK_SIZE);
    int cap = len;
    if (ino->length > len + off)
        cap = ino->length - off;
    memcpy(&data[off], buf, cap);
    return 0;
}


inode_t *tar_open(inode_t *dir, CSTR name, int mode, acl_t *acl, int flags)
{
    return NULL;
}


void *tar_opendir(inode_t *dir)
{
    int *idx = kalloc(sizeof(int));
    *idx = 0;
    return idx;
}

inode_t *tar_readdir(inode_t *dir, char *name, void *ctx)
{
    int length = 0;
    tar_entry_t *entry;
    for (;;) {
        int *idx = (int*)ctx;
        entry = (tar_entry_t *)ADDR_OFF(tinfo.start, *idx);

        length = tar_read_octal(entry->file_size);
        *idx += ALIGN_UP(length + TAR_BLOCK_SIZE, TAR_BLOCK_SIZE);
        if (*idx > tinfo.length)
            return NULL;
        if (length != 0)
            break;
    }
    strncpy(name, entry->name, 255);
    inode_t *ino = tar_inode(entry, length);
    return ino;
}

int tar_closedir(inode_t *dir, void *ctx)
{
    kfree(ctx);
    return 0;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

inode_t *tar_mount(void *base, void *end, CSTR name)
{
    tinfo.start = base;
    tinfo.length = end - base;

    inode_t *ino = vfs_inode(1, S_IFDIR | 0777, NULL, 0);
    ino->length = 0;
    ino->lba = 0;
    // info

    fsvolume_t *fs = (fsvolume_t *)kalloc(sizeof(fsvolume_t));
    fs->open = (fs_open)tar_open;
    fs->read = (fs_read)tar_read;
    fs->umount = (fs_umount)tar_umount;
    fs->opendir = (fs_opendir)tar_opendir;
    fs->readdir = (fs_readdir)tar_readdir;
    fs->closedir = (fs_closedir)tar_closedir;

    vfs_mountpt(name, "tarfs", fs, (inode_t *)ino);
    errno = 0;
    return ino;
}
