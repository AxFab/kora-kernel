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
#include "fatfs.h"

int fatfs_umount(inode_t *ino)
{
    FAT_volume_t *info = (FAT_volume_t *)ino->info;
    bio_destroy(info->io_data_ro);
    bio_destroy(info->io_data_rw);
    bio_destroy(info->io_head);
    kfree(info);
    return 0;
}

fs_ops_t fatfs_ops = {
    .open = fatfs_open,
    .unlink = fatfs_unlink,
};

ino_ops_t fatfs_reg_ops = {
    .close = fatfs_close,
    .truncate = fatfs_truncate,
};

ino_ops_t fatfs_dir_ops = {
    .close = fatfs_close,
    .truncate = fatfs_truncate,
    .opendir = (void *)fatfs_opendir,
    .readdir = (void *)fatfs_readdir,
    .closedir = (void *)fatfs_closedir,
};

ino_ops_t fatfs_vol_ops = {
    .close = fatfs_umount,
    .truncate = fatfs_truncate,
    .opendir = (void *)fatfs_opendir,
    .readdir = (void *)fatfs_readdir,
    .closedir = (void *)fatfs_closedir,
};

inode_t *fatfs_mount(inode_t *dev)
{
    if (dev == NULL) {
        errno = ENODEV;
        return NULL;
    }

    uint8_t *ptr = (uint8_t *)kmap(PAGE_SIZE, dev, 0, VMA_FILE_RO);
    FAT_volume_t *info = fatfs_init(ptr);
    kunmap(ptr, PAGE_SIZE);
    if (info == NULL) {
        errno = EBADF;
        return NULL;
    }

    info->io_head = bio_create(dev, VMA_FILE_RW, info->BytsPerSec, 0);
    const char *fsName = info->FATType == FAT32 ? "fat32" : (info->FATType == FAT16 ? "fat16" : "fat12");
    inode_t *ino = vfs_inode(0, FL_VOL, NULL);
    ino->length = 0;
    ino->lba = 1;
    ino->und.vol->info = info;
    ino->und.vol->ops = &fatfs_ops;
    ino->und.vol->volfs = (char *)fsName;
    ino->und.vol->volname = strdup(info->name);
    ino->info = info;
    ino->ops = &fatfs_dir_ops;

    int origin_sector = info->FirstDataSector - 2 * info->SecPerClus;
    info->io_data_rw = bio_create(dev, VMA_FILE_RW, info->BytsPerSec * info->SecPerClus, origin_sector * info->BytsPerSec);
    info->io_data_ro = bio_create(dev, VMA_FILE_RO, info->BytsPerSec * info->SecPerClus, origin_sector * info->BytsPerSec);
    errno = 0;
    return ino;
}



void fatfs_setup()
{
    register_fs("fatfs", (fs_mount)fatfs_mount);
}

void fatfs_teardown()
{
    unregister_fs("fatfs");
}

MODULE(fatfs, fatfs_setup, fatfs_teardown);

