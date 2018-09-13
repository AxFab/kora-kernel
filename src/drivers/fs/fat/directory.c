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

typedef struct FAT_diterator  FAT_diterator_t;

struct FAT_diterator {
	uint8_t *ptr;
	struct FAT_ShortEntry *entry;
	int cluster_size;
	int idx;
	off_t lba;
	bio_t *io;
};

FAT_diterator *fatfs_diterator_open(FAT_inode_t *dir, bool write)
{
	assert(dir != NULL);
	assert(S_ISDIR(dir->ino.mode));
	assert(dir->ino.lba != 0);
	
	FAT_diterator_t *it = (FAT_diterator_t*)kalloc(sizeof(FAT_diterator_t));
	it->cluster_size = dir->vol->SecPerClus * dir->vol->BytsPerSec;
	it->lba = dir->ino->lba;
	it->io = write ? dir->vol->io_data_rw : dir->vol->io_data_ro;
	it->ptr = bio_access(it->io, it->lba);
	it->entry = (struct FAT_ShortEntry*)it->ptr;
	it->idx = 0;
	it->entry--;
	return it;
}

int fatfs_diterator_next_cluster(FAT_diterator_t *it)
{
	return -1;
}

struct FAT_ShortEntry *fatfs_diterator_next(FAT_diterator_t *it)
{
	it->entry++;
	for (;; it->idx++, it->entry++) {
		if ((uint8_t*)it->entry - it->ptr > it->cluster_size) {
			if (fatfs_diterator_next_cluster(it) != 0)
			    return NULL;
		}
		
		if (it->entry->DIR_Attr == ATTR_VOLUME_ID || it->entry->DIR_Attr == 0xE5)
		    continue;
		if (it->entry->DIR_Attr == 0)
		    return NULL;
		if ((it->entry->DIR_Attr & ATTR_LONG_NAME_MASK) == ATTR_LONG_NAME)
		    continue; // TODO - Prepare long name
		return it->entry;
	}
}

void fatfs_diterator_close(FAT_diterator_t *it)
{
	if (it->lba != 0)
	    bio_clean(it->io, it->lba);
	kfree(it);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

FAT_inode_t *fatfs_open(FAT_inode_t *ino, CSTR name, int mode, acl_t *acl, int flags)
{
	FAT_inode_t *ino;
	FAT_diterator_t *it = fatfs_diterator_open(dir, !!(flags | VFS_CREAT));
	const int entries_per_cluster = dir->vol->BytsPerSec / sizeof(struct FAT_ShortEntry);
	struct FAT_ShortEntry *entry;
	while ((entry = fatfs_diterator_next(it)) != NULL) {
		// TODO - Keep track of available space
		char shortname[14];
		fatfs_read_shortname(entry, shortname);
		if (strcmp(name, shortname) != 0) // TODO - long name
		    continue;
		
		// The file exist
		if (!(flags & VFS_OPEN)) {
			errno = EEXIST;
			return NULL;
		}
		
		ino = fatfs_inode(it->lba * entries_per_cluster + it->idx, entry, dir->vol);
		fatfs_diterator_close(it);
		errno = 0;
		return ino;
	}
	
	if (!(flags & VFS_CREAT)) {
		fatfs_diterator_close(it);
		errno = ENOENT;
		return NULL;
	}
	
	// Create a new entry at the end
	entry = it->entry;
	if (entry == NULL) {
		// TODO - alloc cluster
	}
	int alloc_lba = S_ISDIR(mode) ? fatfs_mkdir(dir->vol, dir) : 0;
	
	fatfs_short_entry(entry, alloc_lba, mode);
	fatfs_write_shortname(entry, name);
	memset(&entry[1], 0, sizeof(*entry)); // TODO - not behind cluster

    ino = fatfs_inode(it->lba * entries_per_cluster + it->idx, entry, dir->vol);
    fatfs_diterator_close(it);
    errno = 0;
    return ino;
}


