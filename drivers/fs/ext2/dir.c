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
#include <errno.h>

/* Open a directory context for browsing files */
ext2_dir_iter_t *ext2_opendir(inode_t *dir)
{
    assert(dir != NULL);
    assert(dir->type == FL_DIR);

    ext2_volume_t *vol = (ext2_volume_t *)dir->drv_data;
    ext2_dir_iter_t *iter = kalloc(sizeof(ext2_dir_iter_t));

    iter->entry = ext2_entry(&iter->bk, vol, dir->no, VM_RD);
    ext2_iterator_open(vol, iter->entry, &iter->it, false, VM_RD);
    return iter;
}

/* Look for the next entry on a directoy context */
inode_t *ext2_readdir(inode_t *dir, char *name, ext2_dir_iter_t *iter)
{
    ext2_volume_t *vol = (ext2_volume_t *)dir->drv_data;
    for (;;) {
        int ret = ext2_iterator_next(vol, &iter->it);
        if (ret != 0) {
            return NULL; // TODO -- EIO and not ENOEMPTY
        }
        
        if (iter->it.entry == NULL) {
            errno = ENOENT;
            return NULL;
        }

        if (iter->it.entry->type != 2 || iter->it.entry->ino == 0 || strcmp(iter->it.entry->name, ".") == 0 || strcmp(iter->it.entry->name, "..") == 0)
            continue;

        if (name) {
            memcpy(name, iter->it.entry->name, iter->it.entry->name_len);
            name[iter->it.entry->name_len] = '\0';
        }
        return ext2_inode(vol, iter->it.entry->ino);
    }
}

/* Close a directory context */
int ext2_closedir(inode_t *dir, ext2_dir_iter_t *iter)
{
    ext2_volume_t *vol = (ext2_volume_t *)dir->drv_data;
    ext2_iterator_close(vol, &iter->it);
    bkunmap(&iter->bk);
    kfree(iter);
    return 0;
}

int ext2_dir_is_empty(ext2_volume_t *vol, ext2_ino_t *dir)
{
    int ret;
    ext2_diterator_t iter;
    ext2_iterator_open(vol, dir, &iter, false, VM_RD);
    for (;;) {
        ret = ext2_iterator_next(vol, &iter);
        if (ret != 0) {
            ext2_iterator_close(vol, &iter);
            return -1; // TODO -- EIO and not ENOEMPTY
        }

        if (iter.entry == NULL)
            break;
        if (iter.entry->type == 2 && strcmp(iter.entry->name, ".") != 0 && strcmp(iter.entry->name, "..") != 0) {
            ret = -1;
            break;
        }
    }
    ext2_iterator_close(vol, &iter);
    return ret;
}





void ext2_iterator_open(ext2_volume_t *vol, ext2_ino_t *dir, ext2_diterator_t *iter, bool create, int flags)
{
    memset(iter, 0, sizeof(ext2_diterator_t));
    iter->lba = -1;
    iter->blocks = dir->blocks / (vol->blocksize / 512);
    iter->dir = dir;
    iter->create = create;
    iter->flags = flags;
}

void ext2_iterator_close(ext2_volume_t *vol, ext2_diterator_t *iter)
{
    if (iter->lba >= 0)
        bkunmap(&iter->km);
}



int ext2_iterator_next(ext2_volume_t *vol, ext2_diterator_t *iter)
{
    iter->previous = iter->entry;
    for (unsigned lba = iter->off / vol->blocksize; ; ++lba) {
        if (lba >= iter->blocks) {
            if (!iter->create)
                break;
            iter->dir->size += vol->blocksize;
            iter->dir->blocks = ALIGN_UP(iter->dir->size, vol->blocksize) / 512;
        }

        // Map block
        if (iter->lba != (int)lba) {
            iter->previous = NULL;
            if (iter->lba >= 0)
                bkunmap(&iter->km);
            unsigned blkno = ext2_get_block(vol, iter->dir, lba, false);
            if (blkno == 0 && !iter->create) {
                iter->off += vol->blocksize;
                continue;
            }
            if (blkno == 0) {
                blkno = ext2_get_block(vol, iter->dir, lba, true);
                iter->ptr = bkmap(&iter->km, blkno, vol->blocksize, 0, vol->blkdev, iter->flags);
                iter->lba = lba;
                ext2_dir_en_t *entry = (ext2_dir_en_t *)iter->ptr;
                memset(entry, 0, sizeof(ext2_dir_en_t));
                entry->rec_len = vol->blocksize;

            } else {
                iter->ptr = bkmap(&iter->km, blkno, vol->blocksize, 0, vol->blkdev, iter->flags);
                iter->lba = lba;
            }
        }

        if (iter->ptr == NULL) {
            errno = EIO;
            return -1;
        }

        // Loop over the block entries
        while (lba == iter->off / vol->blocksize) {
            ext2_dir_en_t *entry = ADDR_OFF(iter->ptr, iter->off - lba * vol->blocksize);
            if (entry->rec_len == 0 || iter->off - lba * vol->blocksize + entry->rec_len > vol->blocksize) {
                errno = EIO;
                return -1;
            }

            iter->off += entry->rec_len;
            iter->entry = entry;
            return 0;
        }
    }


    // End of directory
    iter->entry = NULL;
    return 0;
}

ext2_dir_en_t *ext_iterator_find(ext2_volume_t *vol, ext2_diterator_t *iter, const char *name)
{
    unsigned name_len = strnlen(name, 255);
    for (;;) {
        int ret = ext2_iterator_next(vol, iter);
        if (ret != 0) {
            ext2_iterator_close(vol, iter);
            return NULL;
        }

        if (iter->entry == NULL) {
            ext2_iterator_close(vol, iter);
            errno = ENOENT;
            return NULL;
        }

        ext2_dir_en_t *entry = iter->entry;
        if (entry->type != 2 || entry->name_len != name_len || memcmp(name, entry->name, name_len) != 0)
            continue;

        return entry;
    }
}

