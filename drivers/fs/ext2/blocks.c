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


static inline int math_power(int val, int exp)
{
    int res = val;
    --exp;
    while (exp-- > 0)
        res *= val;
    return res;
}

uint32_t ext2_alloc_block(ext2_volume_t *vol)
{
    unsigned i, j, k;

    for (i = 0; i < vol->groupCount; ++i) {
        if (vol->grp[i].free_blocks_count != 0)
            break;
    }

    if (i >= vol->groupCount)
        return 0;

    struct bkmap bm;
    uint8_t *bitmap = bkmap(&bm, vol->grp[i].block_bitmap, vol->blocksize, 0, vol->blkdev, VM_WR);

    for (j = 0; j < vol->blocksize; ++j) {
        if (bitmap[j] == 0xff)
            continue;
        k = 0;
        while (bitmap[j] & (1 << k))
            k++;
        bitmap[j] |= (1 << k);
        vol->grp[i].free_blocks_count--;
        bkunmap(&bm);
        return i * vol->sb->blocks_per_group + j * 8 + k;
    }
    bkunmap(&bm);
    return 0;
}

int ext2_release_block(ext2_volume_t *vol, uint32_t block)
{
    unsigned i = block / vol->sb->blocks_per_group;
    unsigned j = (block - i * vol->sb->blocks_per_group) / 8;
    unsigned k = block % 8;
    assert(block == i * vol->sb->blocks_per_group + j * 8 + k);

    if (i >= vol->groupCount || j >= vol->blocksize)
        return -1;

    struct bkmap bm;
    uint8_t *bitmap = bkmap(&bm, vol->grp[i].block_bitmap, vol->blocksize, 0, vol->blkdev, VM_WR);
    bitmap[j] &= ~(1 << k);
    vol->grp[i].free_blocks_count++;
    bkunmap(&bm);
    return 0;
}


int ext2_getblock_direct(ext2_volume_t *vol, uint32_t *table, int len, int offset, uint32_t *buf, int cnt, int off)
{
    // TODO -- Offset !
    int j = 0, i = off / vol->blocksize;
    assert(i < len);
    for (; i < len && j < cnt; ++i, ++j)
        buf[j] = table[i];
    return j;
}

int ext2_getblock_indirect(ext2_volume_t *vol, uint32_t block, int offset, uint32_t *buf, int cnt, int off, int depth)
{
    // TODO -- Offset !
    int subCount = math_power(vol->blocksize / 4, depth);
    int count = subCount * (vol->blocksize / 4);
    int j = 0, i = off / vol->blocksize;
    assert(i < count);
    --depth;

    if (block == 0) {
        for (j = 0; i < count && j < cnt; ++i, ++j)
            buf[j] = 0;
        return j;
    }

    struct bkmap bk;
    uint32_t *table = bkmap(&bk, block, vol->blocksize, 0, vol->blkdev, VM_RW);

    if (depth == 0) {
        int ret = ext2_getblock_direct(vol, table, vol->blocksize / 4, offset, buf, cnt, off); //, depth);
        bkunmap(&bk);
        return ret;
    }


    bkunmap(&bk);
    return 0;
}

uint32_t ext2_get_block(ext2_volume_t *vol, ext2_ino_t *dir, uint32_t blk)
{
    if (blk < 12)
        return dir->block[blk];
    struct bkmap k1;
    struct bkmap k2;
    struct bkmap k3;
    uint32_t *p;
    uint32_t *pp;
    uint32_t *ppp;
    uint32_t lba;
    size_t blkr = vol->blocksize / sizeof(uint32_t);
    if (blk < 12 + blkr) {
        p = bkmap(&k1, dir->block[12], vol->blocksize, 0, vol->blkdev, VM_RD);
        lba = p[blk - 12];
        bkunmap(&k1);
        return lba;
    }

    blk -= 12  + blkr;
    unsigned ip = blk / blkr;
    unsigned ix = blk % blkr;
    if (ip < blkr) {
        p = bkmap(&k1, dir->block[13], vol->blocksize, 0, vol->blkdev, VM_RD);
        pp = bkmap(&k2, p[ip], vol->blocksize, 0, vol->blkdev, VM_RD);
        lba = pp[ix];
        bkunmap(&k1);
        bkunmap(&k2);
        return lba;
    }

    unsigned iy = ip / blk;
    unsigned iz = ip % blk;
    if (ip < blkr) {
        p = bkmap(&k1, dir->block[14], vol->blocksize, 0, vol->blkdev, VM_RD);
        pp = bkmap(&k2, p[iy], vol->blocksize, 0, vol->blkdev, VM_RD);
        ppp = bkmap(&k3, pp[iz], vol->blocksize, 0, vol->blkdev, VM_RD);
        lba = ppp[ix];
        bkunmap(&k1);
        bkunmap(&k2);
        bkunmap(&k3);
        return lba;
    }

    return 0;
}


void ext2_truncate_direct(ext2_volume_t *vol, uint32_t *table, int len, int bcount, int offset)
{
    for (int i = 0; i < len; ++i) {
        if (table[i] == 0 && i + offset < bcount) {
            /* Need a new block */
            table[i] = ext2_alloc_block(vol);
            /* Map and reset to zero */
            struct bkmap bk;
            char *ptr = bkmap(&bk, table[i], vol->blocksize, 0, vol->blkdev, VM_RW);
            memset(ptr, 0, vol->blocksize);
            bkunmap(&bk);
        } else if (table[i] != 0 && i + offset >= bcount) {
            /* Need to release block */
            ext2_release_block(vol, table[i]);
            table[i] = 0;
        }
    }
}

uint32_t ext2_truncate_indirect(ext2_volume_t *vol, uint32_t block, int bcount, int offset, int depth)
{
    depth--;
    /* indirect block number */
    if (block == 0 && offset < bcount)
        block = ext2_alloc_block(vol);
    if (block != 0) {
        struct bkmap bk;
        uint32_t *table = bkmap(&bk, block, vol->blocksize, 0, vol->blkdev, VM_RW);
        if (depth == 0)
            ext2_truncate_direct(vol, table, vol->blocksize / 4, bcount, offset);
        else {
            int subCount = math_power(vol->blocksize / 4, depth);
            for (int i = 0, n = vol->blocksize / 4; i < n; ++i)
                table[i] = ext2_truncate_indirect(vol, table[i], bcount, offset + i * subCount, depth);
        }
        bkunmap(&bk);
    }
    if (block != 0 && offset >= bcount) {
        ext2_release_block(vol, block);
        block = 0;
    }
    return block;
}
