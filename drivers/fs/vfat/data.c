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
    size_t clusterSize = volume->SecPerClus * volume->BytsPerSec;
    assert(POW2(clusterSize) && (offset & (clusterSize - 1)) == 0);

    unsigned clustNo = ino->lba;
    int foff = 0;

    while (length > 0) {
        while (clustNo != 0 && offset - foff >= clusterSize) {
            clustNo = fat_cluster_next_16(ino->dev->underlying, volume, clustNo);
            foff += clusterSize;
        }

        size_t cap = clusterSize;
        if (clustNo) {
            int lba = FAT_CLUSTER_TO_LBA(volume, clustNo);
            struct bkmap bm;
            char* data = bkmap(&bm, lba, 512, clusterSize, ino->dev->underlying, VM_RD);
            cap = MIN(cap, length);
            memcpy(buffer, data, cap);
            bkunmap(&bm);
        }
        else {
            cap = MIN(cap, length);
            memset(buffer, 0, cap);
        }

        length -= cap;
        offset += cap;
        buffer = ((char*)buffer) + cap;
    }
    return 0;
}

int fat_write(inode_t *ino, const void *buffer, size_t length, xoff_t offset, int flags)
{
    assert(false);
    return 0;
}

