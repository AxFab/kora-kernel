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
 *
 *      File system driver FAT12, FAT16, FAT32 and exFAT.
 */
#include "fatfs.h"

int fatfs_truncate(FAT_inode_t *ino, off_t length)
{
	const int entry_per_cluster = ino->vol->BytsPerSec / sizeof(struct FAT_ShortEntry);
	int lba = ino->ino.no / entry_per_cluster;
	struct FAT_ShortEntry *entry = bio_access(ino->vol->io_data_rw, lba);
	entry += ino->ino.no % entry_per_cluster;
	assert((entry->DIR_Attr & ATTR_ARCHIVE) != 0);
	
	int cluster_sz = ino->vol->SecPerClus * 512;
	int n, i = 0;
	int lg = ALIGN_UP(entry->DIR_FileSize, cluster_sz) / cluster_sz;
	int mx = ALIGN_UP(length, cluster_sz) / cluster_sz;
	
	if (mx > dir->vol->CountOfClusters) {
	    // TODO - Count used space
	    errno = ENOSPC;
	    return -1;
    } else if (lg == mx) {
        entry->DIR_FileSize = length;
        fatfs_settime(&entry->DIR_WrtDate, &entry->DIR_WrtTime, time64());
        // fatfs_settime(&entry->DIR_CrtDate, &entry->DIR_CrtTime, time64()); -- on unix but not windows
        bio_clean(ino->vol->io_data_rw, lba);
        return 0;
    }
    
    int cluster = entry->DIR_FstClusLo;
    if (cluster == 0) {
        assert(lg == 0);
        cluster = fatfs_alloc_cluster_16(ino->vol, -1);
        entry->DIR_FstClusLo = cluster;
        i = 1;
    }
    
    int prev =
    for (n = MIN(lg, mx); i < n; ++i) {
        prev = cluster;
        // cluster = fatfs_next_cluster_16(cluster);
    }
    for (; i < mx; ++i) {
        cluster = fatfs_alloc_cluster_16(cluster);
    }
    for (; i < lg; ++i) {
        // cluster = fatfs_release_cluster_16(cluster);
    }
    
    entry->DIR_FileSize = length;
    fatfs_settime(&entry->DIR_WrtDate, &entry->DIR_WrtTime, time64());
    // fatfs_settime(&entry->DIR_CrtDate, &entry->DIR_CrtTime, time64()); -- on unix but not windows
    bio_clean(ino->vol->io_data_rw, lba);
    bio_sync(ino->vol->io_data_rw);
    return 0;
}

