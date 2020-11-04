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
#include "vfat.h"

ino_ops_t fatfs_reg_ops = {
    // .close = fatfs_close,
    .read = fat_read,
    .write = fat_write,
    .truncate = fatfs_truncate,
};

ino_ops_t fatfs_dir_ops = {
    .open = fat_open,
    // .close = fatfs_close,
    .unlink = fat_unlink,
    .opendir = fat_opendir,
    .readdir = fat_readdir,
    .closedir = fat_closedir,
};

inode_t *fat_mount(inode_t *dev, const char *options)
{
    if (dev == NULL) {
        errno = ENODEV;
        return NULL;
    }

    uint8_t *ptr = (uint8_t *)kmap(PAGE_SIZE, dev, 0, VM_RD);
    FAT_volume_t *info = fatfs_init(ptr);
    kunmap(ptr, PAGE_SIZE);
    if (info == NULL) {
        errno = EBADF;
        return NULL;
    }

    const char *fsName = info->FATType == FAT32 ? "fat32" : (info->FATType == FAT16 ? "fat16" : "fat12");
    inode_t *ino = vfs_inode(1, FL_DIR, NULL, &fatfs_dir_ops);
    ino->length = 0;
    ino->lba = info->RootEntry;
    ino->drv_data = info;
    ino->dev->devclass = strdup(fsName);
    ino->dev->devname = strdup(info->name);
    ino->dev->underlying = vfs_open_inode(dev);

    int origin_sector = info->FirstDataSector - 2 * info->SecPerClus;
    // info->io_data_rw = bio_create(dev, VMA_FILE_RW, info->BytsPerSec * info->SecPerClus, origin_sector * info->BytsPerSec);
    // info->io_data_ro = bio_create(dev, VMA_FILE_RO, info->BytsPerSec * info->SecPerClus, origin_sector * info->BytsPerSec);
    errno = 0;
    return ino;
}

// int fatfs_umount(inode_t *ino)
// {
//     FAT_volume_t *info = (FAT_volume_t *)ino->info;
//     kfree(info);
//     return 0;
// }


void fat_setup()
{
    vfs_addfs("fat", fat_mount);
    vfs_addfs("vfat", fat_mount);
    vfs_addfs("fat12", fat_mount);
    vfs_addfs("fat16", fat_mount);
    vfs_addfs("fat32", fat_mount);
}

void fat_teardown()
{
    vfs_rmfs("fat");
    vfs_rmfs("vfat");
    vfs_rmfs("fat12");
    vfs_rmfs("fat16");
    vfs_rmfs("fat32");
}

MODULE(vfat, fat_setup, fat_teardown);

