/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015  <Fabien Bavent>
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
 *      File system driver FAT12, FAT16, FAT32 and exFAT.
 */
#include "fatfs.h"


int fatfs_umount(FAT_inode_t *ino)
{
    // vfs_close(ino->vol->dev);
    kfree(ino->vol);
    return 0;
}

inode_t *fatfs_mount(inode_t *dev)
{
    if (dev == NULL) {
        errno = ENODEV;
        return NULL;
    }

    uint8_t *ptr = (uint8_t *)kmap(PAGE_SIZE, dev, 0, VMA_FILE_RO);
    struct FAT_volume *info = fatfs_init(ptr, dev);
    if (info == NULL) {
        kunmap(ptr, PAGE_SIZE);
        errno = EBADF;
        return NULL;
    }


    const char *fsName = info->FATType == FAT32 ? "fat32" :
                         (info->FATType == FAT16 ?
                          "fat16" : "fat12");
    FAT_inode_t *ino = (FAT_inode_t *)vfs_inode(info->RootEntry, S_IFDIR | 0755,
                       NULL, sizeof(FAT_inode_t));
    ino->ino.length = 0;
    // ino->ino.block = /*mount->SecPerClus */ mount->BytsPerSec;
    ino->ino.lba = info->RootEntry;
    ino->vol = info;
    // ino->vol->dev = vfs_open(dev);

    fsvolume_t *fs = (fsvolume_t *)kalloc(sizeof(fsvolume_t));
    // fs->lookup = (fs_lookup)isofs_lookup;
    // fs->read = (fs_read)isofs_read;
    fs->umount = (fs_umount)fatfs_umount;
    // fs->opendir = (fs_opendir)isofs_opendir;
    // fs->readdir = (fs_readdir)isofs_readdir;
    // fs->closedir = (fs_closedir)isofs_closedir;
    // fs->read_only = true;

    vfs_mountpt(info->name, fsName, fs, (inode_t *)ino);
    kunmap(ptr, PAGE_SIZE);
    return &ino->ino;
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

