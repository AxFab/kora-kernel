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
#include <kernel/files.h>
#include "fatfs.h"


int fatfs_read(inode_t *ino, void *buffer, size_t length, off_t offset)
{
    FAT_volume_t *info = (FAT_volume_t *)ino->und.vol->info;
    int cluster = offset / (info->BytsPerSec * info->SecPerClus);
    int lba = ino->lba;
    while (length > 0) {
        if (lba == 0) {
            return -1; // EOF
        }
        assert(false) ;
        // vfs_read();
    }
    return 0;
}

int fatfs_write(inode_t *ino, const void *buffer, size_t length, off_t offset)
{
    assert(false) ;
    return 0;
}

page_t fatfs_fetch(inode_t *ino, off_t off)
{
    return map_fetch(ino->info, off);
}

void fatfs_sync(inode_t *ino, off_t off, page_t pg)
{
    map_sync(ino->info, off, pg);
}

void fatfs_release(inode_t *ino, off_t off, page_t pg)
{
    map_release(ino->info, off, pg);
}


