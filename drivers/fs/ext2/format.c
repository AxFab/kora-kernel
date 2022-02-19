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
#include <kernel/stdc.h>
#include <kernel/vfs.h>
#include <kernel/mods.h>
#include <string.h>
#include <errno.h>
#include "ext2.h"

static uint32_t uint_log2(uint32_t value) {
    unsigned log = 0;
    while (value > 1) {
        log++;
        value >>= 1;
    }
    return log;
}

static int ext2_has_super(uint32_t value)
{
    // 0, 1 and follow all powers of 3, 5 or 7 up to uint32 limit
    static const uint32_t supers[] = {
        0, 1, 3, 5, 7, 9, 25, 27, 49, 81, 125, 243, 343, 625, 729,
        2187, 2401, 3125, 6561
    };
    // assert(value != 0);
    const uint32_t* sp = supers + (sizeof(supers) / sizeof(uint32_t));
    for (;;) {
        sp--;
        if (value == *sp)
            return 1;
        if (value > *sp)
            return 0;
    }
}

static void ext2_format_allocate_grp(inode_t *dev, size_t blkno, uint32_t block_size, uint32_t start, uint32_t end)
{
    struct bkmap bm;
    uint8_t* ptr = bkmap(&bm, blkno, block_size, 0, dev, VM_WR);

    memset(ptr, 0, block_size);
    // Mark start first as buzy
    unsigned i = start / 8;
    memset(ptr, 0xff, i);
    ptr[i] = (1 << (start & 7)) - 1;
    // Mark end last as buzy
    i = end / 8;
    ptr[block_size - i - 1] |= 0x7f00 >> (end & 7);
    memset(ptr + block_size - i, 0xff, i);
    bkunmap(&bm);
}


int ext2_format(inode_t* dev, const char* options)
{
    if (dev->length <= 0) // TODO -- Or can't seek!
        return -1;
    
    uint32_t bytes_per_inode = 16384;
    if (dev->length < 3 * _Mib_)
        bytes_per_inode = 8 * _Kib_;
    else if (dev->length < 512 * _Mib_)
        bytes_per_inode = 4 * _Kib_;

    uint32_t block_size = 1024;
    uint32_t inode_size = sizeof(ext2_ino_t);
    if (dev->length > 512 * _Mib_) {
        block_size = 4096;
        inode_size = 256;
    }
    if (EXT2_MAX_BLOCK_SIZE > 4096) {
        while ((dev->length / 4 * _Gib_) > block_size && block_size < EXT2_MAX_BLOCK_SIZE)
            block_size *= 2;
    }

    if (bytes_per_inode < block_size)
        return -1;

    uint32_t blocks_per_group = 8 * block_size;
    uint32_t block_log_size = uint_log2(block_size);
    uint32_t nbr_blocks = dev->length >> block_log_size;
    uint32_t nreserved = (uint64_t)nbr_blocks * 5 / 100;
    if (nbr_blocks < 60)
        return -1;

    int dc = (block_size == 1024 ? 1 : 0);
    uint32_t grp_desc_per_block = block_size / sizeof(ext2_grp_t);
    uint32_t ngroup, grp_desc_blocks, inodes_per_group, inode_table_blocks, lost_and_found_block;
    for (;;) {
        ngroup = ALIGN_UP(nbr_blocks - dc, blocks_per_group) / blocks_per_group;
        grp_desc_blocks = ALIGN_UP(ngroup, grp_desc_per_block) / grp_desc_per_block;

        // check group alignement or discard block
        uint32_t max_inodes = dev->length / bytes_per_inode;
        if (max_inodes < EXT2_GOOD_OLD_FIRST_INO + 1)
            max_inodes = EXT2_GOOD_OLD_FIRST_INO + 1;
        inodes_per_group = (max_inodes + ngroup - 1) / ngroup;
        if (inodes_per_group < 16)
            inodes_per_group = 16;
        if (inodes_per_group > blocks_per_group)
            inodes_per_group = blocks_per_group;
        // Adjust to fill inode table
        uint32_t group_sz = inodes_per_group * inode_size;
        group_sz = ((group_sz + block_size - 1) / block_size) * block_size;
        inodes_per_group = (group_sz / inode_size) & ~7;
        inode_table_blocks = (inodes_per_group * inode_size + block_size - 1) / block_size;
                    
        lost_and_found_block = MIN(EXT2_NDIR_BLOCKS, 16 >> (block_log_size - 10));

        uint32_t overhead = (ext2_has_super(ngroup - 1) ? (1 + grp_desc_blocks) : 0) + 2 + inode_table_blocks; // TODO -- explain !
        uint32_t remainder = (nbr_blocks - dc) % blocks_per_group;
        if (remainder && (remainder < overhead + 50)) {
            nbr_blocks -= remainder;
            continue;
        }
        
        break;
    }

    // int unused_block = (dev->length >> block_log_size) - nbr_blocks;


    kprintf(-1, "File system label = %s\n", "");
    kprintf(-1, "Block size = %d\n", (1024 << (block_log_size - 10)));
    kprintf(-1, "Fragment size = %d\n", (1024 << (block_log_size - 10)));
    kprintf(-1, "%d inodes, %d blocks\n", inodes_per_group * ngroup, nbr_blocks);
    kprintf(-1, "%d blocks reserved for super user\n", nreserved);
    kprintf(-1, "First data block = %d\n", dc);
    kprintf(-1, "Maximum filesystem blocks = %d\n", grp_desc_blocks * (block_size / sizeof(ext2_grp_t)) * blocks_per_group);
    kprintf(-1, "%d block groups\n", ngroup);
    kprintf(-1, "%d blocks per group\n", blocks_per_group);
    kprintf(-1, "%d frags per group\n", blocks_per_group);
    kprintf(-1, "%d inodes per group\n", inodes_per_group);

    unsigned i, n;
    uint32_t pos = dc;
    for (i = 1; i < ngroup; ++i) {
        pos += blocks_per_group;
        if (ext2_has_super(i))
            kprintf(-1, "Superblock backup on blocks:%u\n", pos);
    }

    uint32_t timestamp = (uint32_t)(xtime_read(XTIME_CLOCK) / 1000000UL);

    // Prepare super block !
    ext2_sb_t* sb = kalloc(1024);
    // memset(sb, 0, sizeof(ext2_sb_t));
    sb->magic = EXT2_SUPER_MAGIC;
    sb->rev_level = EXT2_DYNAMIC_REV;
    strncpy(sb->volume_name, "", 16);
    sb->inode_size = inode_size;
    // if (inode_size != sizeof(ext2_inode_t)) TODO
    sb->first_ino = EXT2_GOOD_OLD_FIRST_INO;
    sb->log_block_size = block_log_size - 10;
    sb->log_frag_size = block_log_size - 10;
    sb->first_data_block = dc;
    sb->blocks_per_group = blocks_per_group;
    sb->frags_per_group = blocks_per_group;
    sb->blocks_count = nbr_blocks;
    sb->rsvd_blocks_count = nreserved;
    sb->inodes_per_group = inodes_per_group;
    sb->inodes_count = inodes_per_group * ngroup;
    sb->free_inodes_count = inodes_per_group * ngroup - EXT2_GOOD_OLD_FIRST_INO;
    sb->mtime = timestamp;
    sb->wtime = timestamp;
    sb->lastcheck = timestamp;
    sb->state = 1;
    sb->creator_os = 793;
    sb->checkinterval = 24 * 3600 * 180; // 180 days
    sb->errors = 1;
    sb->feature_compat = 8; // TODO -- Default
    sb->feature_incompat = 2;
    sb->feature_ro_compat = 1;
    sb->max_mnt_count = 20 + rand8() % 20; // jitter hack    
    for (i = 0; i < 16; ++i)
        sb->uuid[i] = rand8();


    uint32_t grp_full_size = grp_desc_blocks * block_size;
    ext2_grp_t* gd = kalloc(grp_full_size);
    uint32_t free_blocks_count = 0;
    for (i = 0, pos = dc, n = nbr_blocks; i < ngroup; ++i, pos += blocks_per_group, n -= blocks_per_group) {

        uint32_t overhead = pos + (ext2_has_super(i) ? (1 + grp_desc_blocks) : 0);
        uint32_t free_blocks, free_inodes;
        gd[i].block_bitmap = overhead + 0;
        gd[i].inode_bitmap = overhead + 1;
        gd[i].inode_table = overhead + 2;
        kprintf(-1, "Group %d :: %d block bitmap, %d inodes bitmap, %d inodes tables, %d data blocks\n", i, overhead, overhead + 1, overhead + 2, sb->blocks_per_group - 3);
        overhead = overhead - pos + 2 + inode_table_blocks;
        free_inodes = inodes_per_group;
        if (i == 0) {
            overhead += 1 + lost_and_found_block;
            gd[i].used_dirs_count = 2;
            gd[i].free_inodes_count -= EXT2_GOOD_OLD_FIRST_INO;
            free_inodes -= EXT2_GOOD_OLD_FIRST_INO;
        }
        free_blocks = (n < blocks_per_group ? n : blocks_per_group) - overhead;
        gd[i].free_inodes_count = free_inodes;
        gd[i].free_blocks_count = free_blocks;

        // Blocks bitmap
        ext2_format_allocate_grp(dev, gd[i].block_bitmap, block_size, overhead, blocks_per_group - free_blocks - overhead);
        // Inodes bitmap
        ext2_format_allocate_grp(dev, gd[i].inode_bitmap, block_size, inodes_per_group - free_inodes, blocks_per_group - inodes_per_group);

        free_blocks_count += free_blocks;
    }
    uint32_t last_block = dc + 1 + grp_desc_blocks + sb->blocks_per_group * ngroup;
    kprintf(-1, "Last block %d / %d\n", pos, last_block);

    // Super blocks and descriptors
    sb->free_blocks_count = free_blocks_count;
    struct bkmap bm;
    uint8_t* ptr;
    for (i = 0, pos = dc; i < ngroup; ++i, pos += blocks_per_group) {
        if (!ext2_has_super(i))
            continue;
        ptr = bkmap(&bm, pos, block_size, 0, dev, VM_WR);
        memcpy(ptr, sb, 1024);
        bkunmap(&bm);

        ptr = bkmap(&bm, pos + 1, block_size, grp_full_size, dev, VM_WR);
        memcpy(ptr, gd, grp_full_size);
        bkunmap(&bm);
    }

    // Boot sector
    ptr = bkmap(&bm, 0, block_size, 0, dev, VM_WR);
    memset(ptr, 0, 1024);
    bkunmap(&bm);

    // Inodes tables
    for (i = 0; i < ngroup; ++i) {
        for (n = 0; n < inode_table_blocks; ++n) {
            ptr = bkmap(&bm, gd[i].inode_table + n, block_size, 0, dev, VM_RW);
            memset(ptr, 0, block_size);
            bkunmap(&bm);
        }
    }

    // Root directory
    ext2_ino_t* ino = kalloc(sizeof(ext2_ino_t));
    ino->mode = EXT2_S_IFDIR | 0755;
    ino->mtime = timestamp;
    ino->atime = timestamp;
    ino->ctime = timestamp;
    ino->size = block_size;
    ino->blocks = block_size / 512;
    ino->links = 3; // /.  /..  /lost+found/..
    ino->block[0] = gd[0].inode_table + inode_table_blocks;
    ptr = bkmap(&bm, gd[0].inode_table, block_size, 0, dev, VM_RW);
    memcpy(&ptr[(EXT2_ROOT_INO - 1) * inode_size], ino, inode_size);
    bkunmap(&bm);

    // lost+found directory
    ino->links = 2;
    ino->size = lost_and_found_block * block_size;
    ino->blocks = lost_and_found_block * block_size / 512;
    n = ino->block[0] + 1;
    for (i = 0; i < lost_and_found_block; ++i)
        ino->block[i] = n + i;
    ptr = bkmap(&bm, gd[0].inode_table, block_size, 0, dev, VM_RW);
    memcpy(&ptr[(EXT2_GOOD_OLD_FIRST_INO - 1) * inode_size], ino, inode_size);
    bkunmap(&bm);


    // lost+found content 2nd+ block
    ext2_dir_hack_t* dir = kalloc(block_size);
    dir->rec_len1 = block_size;
    for (i = 1; i < lost_and_found_block; ++i) {
        ptr = bkmap(&bm, gd[0].inode_table + inode_table_blocks + 1 + i, block_size, 0, dev, VM_RW);
        memcpy(ptr, dir, block_size);
        bkunmap(&bm);
    }

    // lost+found content 1st block
    dir->ino1 = EXT2_GOOD_OLD_FIRST_INO;
    dir->rec_len1 = 12;
    dir->name_len1 = 1;
    dir->file_type1 = EXT2_FT_DIR;
    dir->name1[0] = '.';

    dir->ino2 = EXT2_ROOT_INO;
    dir->rec_len2 = block_size - 12;
    dir->name_len2 = 2;
    dir->file_type2 = EXT2_FT_DIR;
    dir->name2[0] = '.';
    dir->name2[1] = '.';
    ptr = bkmap(&bm, gd[0].inode_table + inode_table_blocks + 1, block_size, 0, dev, VM_RW);
    memcpy(ptr, dir, block_size);
    bkunmap(&bm);

    // root content
    dir->ino1 = EXT2_ROOT_INO;
    dir->rec_len2 = 12;
    dir->ino3 = EXT2_GOOD_OLD_FIRST_INO;
    dir->rec_len3 = block_size - 24;
    dir->name_len3 = 10;
    dir->file_type3 = EXT2_FT_DIR;
    strcpy(dir->name3, "lost+found");
    ptr = bkmap(&bm, gd[0].inode_table + inode_table_blocks + 0, block_size, 0, dev, VM_RW);
    memcpy(ptr, dir, block_size);
    bkunmap(&bm);

    kfree(sb);
    kfree(gd);
    kfree(ino);
    kfree(dir);
    return 0;
}
