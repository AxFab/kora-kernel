/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2021  <Fabien Bavent>
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
#include "ext2.h"

/* Open a directory context for browsing files */
ext2_dir_iter_t *ext2_opendir(inode_t *dir)
{
    assert(dir != NULL);
    assert(dir->type == FL_DIR);

    ext2_dir_iter_t *it = (ext2_dir_iter_t *)kalloc(sizeof(ext2_dir_iter_t));
    ext2_volume_t *vol = (ext2_volume_t *)dir->drv_data;
    it->vol = vol;

    //uint32_t group = (dir->no - 1) / vol->sb->inodes_per_group;
    //uint32_t index = (dir->no - 1) % vol->sb->inodes_per_group;
    //// uint32_t block = (index * vol->sb->inode_size) / (1024 << vol->sb->log_block_size);
    //uint32_t blk = vol->grp[group].inode_table;

    //size_t off = (1024 << vol->sb->log_block_size) * blk + index * sizeof(ext2_ino_t);
    //size_t lba = ALIGN_DW(off, PAGE_SIZE);

    it->entry = ext2_entry(&it->bki, vol, dir->no, VM_RD);

    //it->emap = kmap(PAGE_SIZE, dir->dev->underlying, lba, VM_RD);
    //it->entry = ADDR_OFF(it->emap, off - lba);


    // TODO -- Check that entry is correct !!
    it->idx = 0;
    return it;
}

/* Find the next inode on a directory context */
uint32_t ext2_nextdir(inode_t *dir, char *name, ext2_dir_iter_t *it)
{
    (void)dir;
    // TODO -- If we read above file size, leave
    for (;;) {

        uint32_t blk = it->entry->block[it->idx / it->vol->blocksize]; // Look at IDX (for blk and off)

        size_t off = it->vol->blocksize * blk + (it->idx % it->vol->blocksize);
        size_t lba = ALIGN_DW(off, PAGE_SIZE);
        if (it->lba != lba) {
            // bkunmap(&it->bkm);
            if (it->cmap)
                kunmap(it->cmap, PAGE_SIZE);
            it->cmap = kmap(PAGE_SIZE, it->vol->dev->underlying, lba, VM_RD);
            it->lba = lba;
        }

        ext2_dir_en_t *entry = ADDR_OFF(it->cmap, off - lba);
        it->idx += entry->rec_len;
        if (strcmp(entry->name, ".") == 0 || strcmp(entry->name, "..") == 0)
            continue;

        // Assert name is on the block ?
        if (entry->type == 0)
            return 0;

        if (name) {
            strncpy(name, entry->name, MIN(255, entry->name_len));
            name[entry->name_len] = '\0';
        }
        return entry->ino;
    }
}

/* Look for the next entry on a directoy context */
inode_t *ext2_readdir(inode_t *dir, char *name, ext2_dir_iter_t *it)
{
    uint32_t ino_no = ext2_nextdir(dir, name, it);
    if (ino_no == 0)
        return NULL;
    return ext2_inode(dir->drv_data, ino_no);
}

/* Close a directory context */
int ext2_closedir(inode_t *dir, ext2_dir_iter_t *it)
{
    //if (it->cmap)
    //    kunmap(it->cmap, PAGE_SIZE);
    //if (it->emap)
    //    kunmap(it->emap, PAGE_SIZE);
    // bkunmap(&it->bkm);
    bkunmap(&it->bki);


    kfree(it);
    return 0;
}

int ext2_dir_is_empty(ext2_volume_t *vol, ext2_ino_t *dir)
{
    // Open directory
    ext2_dir_iter_t it;
    memset(&it, 0, sizeof(ext2_dir_iter_t));
    it.vol = vol;
    it.entry = dir;
    it.idx = 0;

    // Look first entry
    uint32_t ino = ext2_nextdir(NULL, NULL, &it);

    // Clear
    bkunmap(&it.bki);
    return ino == 0 ? 0 : -1;
}
