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
	errno = ENOSPC;
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

static uint32_t ext2_get_block_direct(ext2_volume_t *vol, uint32_t block, uint32_t no, bool alloc)
{
	if (block == 0)
		return 0;
	struct bkmap km;
	uint32_t *ptr = bkmap(&km, block, vol->blocksize, 0, vol->blkdev, VM_RD);
	if (ptr == NULL) {
		errno = EIO;
		return 0;
	}
	uint32_t lba = ptr[no];
	if (lba == 0 && alloc)
		lba = ptr[no] = ext2_alloc_block(vol);
	bkunmap(&km);
	return lba;
}

static uint32_t ext2_get_block_indirect(ext2_volume_t *vol, uint32_t block, uint32_t blkr, uint32_t no, int depth, bool alloc)
{
	if (block == 0)
		return 0;
	struct bkmap km;
	unsigned ip = no / blkr;
	unsigned ix = no % blkr;
	uint32_t *ptr = bkmap(&km, block, vol->blocksize, 0, vol->blkdev, VM_RD);
	if (ptr == NULL) {
		errno = EIO;
		return 0;
	}
	uint32_t lba;
	uint32_t blkno = ptr[ip];
	if (blkno == 0 && alloc)
		blkno = ptr[ip] = ext2_alloc_block(vol);

	if (depth == 0)
		lba = ext2_get_block_direct(vol, blkno, ix, alloc);
	else
		lba = ext2_get_block_indirect(vol, blkno, ix, blkr, depth - 1, alloc);
	bkunmap(&km);
	return lba;
}

uint32_t ext2_get_block(ext2_volume_t *vol, ext2_ino_t *dir, uint32_t blk, bool alloc)
{
	uint32_t blkno;
	if (blk < 12) {
		blkno = dir->block[blk];
		if (alloc && blkno == 0)
			blkno = dir->block[blk] = ext2_alloc_block(vol);
		return blkno;
	}

	size_t blkr = vol->blocksize / sizeof(uint32_t);
	if (blk < 12 + blkr) {
		blkno = dir->block[12];
		if (alloc && blkno == 0)
			blkno = dir->block[12] = ext2_alloc_block(vol);
		return ext2_get_block_direct(vol, blkno, blk - 12, alloc);
	}

	blk -= 12 + blkr;
	if (blk < blkr * blkr) {
		blkno = dir->block[13];
		if (alloc && blkno == 0)
			blkno = dir->block[13] = ext2_alloc_block(vol);
		return ext2_get_block_indirect(vol, dir->block[13], blkr, blk, 0, alloc);
	}

	blk -= blkr * blkr;
	if (blk < blkr * blkr * blkr) {
		blkno = dir->block[14];
		if (alloc && blkno)
			blkno = dir->block[14] = ext2_alloc_block(vol);
		return ext2_get_block_indirect(vol, dir->block[14], blkr, blk, 1, alloc);
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


static int ext2_free_blocks_direct(ext2_volume_t *vol, uint32_t block, size_t blkr)
{
	int ret = 0;
	struct bkmap km;
	uint32_t *ptr = bkmap(&km, block, vol->blocksize, 0, vol->blkdev, VM_RD);
	if (ptr == NULL)
		ret = -1;
	else {
		for (unsigned i = 0; i < blkr; ++i) {
			if (ptr[i] != 0)
				ret |= ext2_release_block(vol, ptr[i]);
			// ptr[i] = 0;
		}
	}
	ret |= ext2_release_block(vol, block);
	bkunmap(&km);
	return ret;
}

static int ext2_free_blocks_indirect(ext2_volume_t *vol, uint32_t block, size_t blkr, int depth)
{
	int ret = 0;
	struct bkmap km;
	uint32_t *ptr = bkmap(&km, block, vol->blocksize, 0, vol->blkdev, VM_RD);
	if (ptr == NULL)
		ret = -1;
	else {
		for (unsigned i = 0; i < blkr; ++i) {
			if (ptr[i] == 0)
				continue;
			if (depth == 0)
				ret |= ext2_free_blocks_direct(vol, ptr[i], blkr);
			else
				ret |= ext2_free_blocks_indirect(vol, ptr[i], blkr, depth - 1);
			// ptr[i] = 0;
		}
	}
	ret |= ext2_release_block(vol, block);
	bkunmap(&km);
	return ret;
}

int ext2_free_blocks(ext2_volume_t *vol, uint32_t *blocks)
{
	int ret = 0;
	for (unsigned i = 0; i < 12; ++i) {
		if (blocks[i] != 0)
			ret |= ext2_release_block(vol, blocks[i]);
	}

	size_t blkr = vol->blocksize / sizeof(uint32_t);

	if (blocks[12] != 0)
		ret |= ext2_free_blocks_direct(vol, blocks[12], blkr);

	if (blocks[13] != 0)
		ret |= ext2_free_blocks_indirect(vol, blocks[13], blkr, 0);

	if (blocks[14] != 0)
		ret |= ext2_free_blocks_indirect(vol, blocks[14], blkr, 1);
	return ret;
}
