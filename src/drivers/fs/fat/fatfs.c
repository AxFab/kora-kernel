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
#include "fatfs.h"

int fatfs_truncate(inode_t *ino, off_t length)
{
    FAT_volume_t *info = (FAT_volume_t *)ino->info;
    const int entry_per_cluster = info->BytsPerSec / sizeof(struct FAT_ShortEntry);
    int lba = ino->no / entry_per_cluster;
    struct FAT_ShortEntry *entry = bio_access(info->io_data_rw, lba);
    entry += ino->no % entry_per_cluster;
    assert((entry->DIR_Attr & ATTR_ARCHIVE) != 0);

    unsigned cluster_sz = info->SecPerClus * 512;
    unsigned n, i = 0;
    unsigned lg = ALIGN_UP(entry->DIR_FileSize, cluster_sz) / cluster_sz;
    unsigned mx = ALIGN_UP(length, cluster_sz) / cluster_sz;

    if (mx > info->CountofClusters) {
        // TODO - Count used space
        bio_clean(info->io_data_rw, lba);
        errno = ENOSPC;
        return -1;
    } else if (lg == mx) {
        entry->DIR_FileSize = length;
        fatfs_settime(&entry->DIR_WrtDate, &entry->DIR_WrtTime, time64());
        // fatfs_settime(&entry->DIR_CrtDate, &entry->DIR_CrtTime, time64()); -- on unix but not windows
        bio_clean(info->io_data_rw, lba);
        return 0;
    }

    int cluster = entry->DIR_FstClusLo;
    if (cluster == 0) {
        assert(lg == 0);
        cluster = fatfs_alloc_cluster_16(info, -1);
        entry->DIR_FstClusLo = cluster;
        i = 1;
    }

    // int prev = -1;
    for (n = MIN(lg, mx); i < n; ++i) {
        // prev = cluster;
        // cluster = fatfs_next_cluster_16(info, cluster);
    }
    for (; i < mx; ++i)
        cluster = fatfs_alloc_cluster_16(info, cluster);
    for (; i < lg; ++i) {
        // cluster = fatfs_release_cluster_16(info, cluster);
    }

    entry->DIR_FileSize = length;
    fatfs_settime(&entry->DIR_WrtDate, &entry->DIR_WrtTime, time64());
    // fatfs_settime(&entry->DIR_CrtDate, &entry->DIR_CrtTime, time64()); -- on unix but not windows
    bio_clean(info->io_data_rw, lba);
    bio_sync(info->io_data_rw);
    return 0;
}

