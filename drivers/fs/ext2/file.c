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


/* Copy into the buffer data from mapped area of the underlying block device */
int ext2_read(inode_t *ino, char *buffer, size_t length, xoff_t offset, int flags)
{
    struct bkmap bk;
    ext2_volume_t *vol = (ext2_volume_t *)ino->drv_data;
    ext2_ino_t *en = ext2_entry(&bk, vol, ino->no, VM_RD);

    while (length > 0) {
        size_t cap = MIN(vol->blocksize, length);
        uint32_t blk = ext2_get_block(vol, en, offset / vol->blocksize);
        int off = offset % vol->blocksize;

        if (blk && offset < ino->length) {
            struct bkmap bm;
            char *data = bkmap(&bm, blk, vol->blocksize, 0, ino->dev->underlying, VM_RD);
            cap = MIN(cap, ino->length - offset);
            cap = MIN(cap, vol->blocksize - off);
            memcpy(buffer, &data[off], cap);
            bkunmap(&bm);
        } else
            memset(buffer, 0, cap);

        length -= cap;
        offset += cap;
        buffer = buffer + cap;
    }

    bkunmap(&bk);
    return 0;
}

/* Copy the buffer data into mapped area of the underlying block device */
int ext2_write(inode_t *ino, const char *buffer, size_t length, xoff_t offset, int flags)
{
    struct bkmap bk;
    ext2_volume_t *vol = (ext2_volume_t *)ino->drv_data;
    ext2_ino_t *en = ext2_entry(&bk, vol, ino->no, VM_RD);

    while (length > 0) {
        size_t cap = MIN(vol->blocksize, length);
        uint32_t blk = ext2_get_block(vol, en, offset / vol->blocksize);
        int off = offset % vol->blocksize;

        if (blk && offset < ino->length) {
            struct bkmap bm;
            char *data = bkmap(&bm, blk, vol->blocksize, 0, ino->dev->underlying, VM_WR);
            cap = MIN(cap, ino->length - offset);
            cap = MIN(cap, vol->blocksize - off);
            memcpy(&data[off], buffer, cap);
            bkunmap(&bm);
        } else {
            bkunmap(&bk);
            errno = EINVAL;
            return -1;
        }

        length -= cap;
        offset += cap;
        buffer = buffer + cap;
    }

    bkunmap(&bk);
    return 0;
}

/* Change the size of a file inode and allocate or release blocks */
int ext2_truncate(inode_t *ino, xoff_t offset)
{
    struct bkmap bk;
    ext2_volume_t *vol = (ext2_volume_t *)ino->drv_data;
    ext2_ino_t *en = ext2_entry(&bk, vol, ino->no, VM_WR);

    int block_count = ALIGN_UP(offset, vol->blocksize) / vol->blocksize;
    /* direct block number */
    ext2_truncate_direct(vol, en->block, 12, block_count, 0);

    int intPerBlock = vol->blocksize / 4;
    /* indirect block number */
    en->block[12] = ext2_truncate_indirect(vol, en->block[12], block_count, 12, 1);

    /* bi-indirect block number */
    en->block[13] = ext2_truncate_indirect(vol, en->block[13], block_count, 12 + intPerBlock, 2);

    /* tri-indirect block number */
    en->block[14] = ext2_truncate_indirect(vol, en->block[14], block_count, 12 + intPerBlock + intPerBlock * intPerBlock, 3);

    en->size = offset;
    en->blocks = block_count;
    ino->length = offset;
    bkunmap(&bk);
    return 0;
}
