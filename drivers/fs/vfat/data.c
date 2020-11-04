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
#include <assert.h>

int fat_read(inode_t *ino, void *buffer, size_t length, xoff_t offset, int flags)
{
    FAT_volume_t * volume = ino->drv_data;

    int lba = FAT_CLUSTER_TO_LBA(volume, ino->lba);

    int fL = ((volume->FirstDataSector) * volume->BytsPerSec) / PAGE_SIZE;
    int eL = ALIGN_UP((volume->FirstDataSector + volume->SecPerClus) * volume->BytsPerSec, PAGE_SIZE) / PAGE_SIZE;
    int sL = eL - fL;
    size_t map_size = sL * PAGE_SIZE;
    size_t sec_size = volume->BytsPerSec;

    int page = (lba * sec_size) / PAGE_SIZE;
    int off = (lba * sec_size) % PAGE_SIZE;
    void *cluster = kmap(map_size, ino->dev->underlying, page * PAGE_SIZE, VM_RD);

    // int cluster = offset / (volume->BytsPerSec * volume->SecPerClus);
    // int lba = ino->lba;
    while (length > 0) {
        if (lba == 0) {
            return -1; // EOF
        }
        assert(false);
        // vfs_read();
    }
    return 0;
}

int fat_write(inode_t *ino, const void *buffer, size_t length, xoff_t offset, int flags)
{
    assert(false);
    return 0;
}

