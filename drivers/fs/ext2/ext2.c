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


static inline int math_power(int val, int exp)
{
    int res = val;
    --exp;
    while (exp-- > 0)
        res *= val;
    return res;
}



extern ino_ops_t ext2_dir_ops;
extern ino_ops_t ext2_reg_ops;

ext2_ino_t *ext2_entry(struct bkmap *bk, ext2_volume_t *vol, unsigned no, int flg)
{
    uint32_t group = (no - 1) / vol->sb->inodes_per_group;
    uint32_t index = (no - 1) % vol->sb->inodes_per_group;
    uint32_t blk = vol->grp[group].inode_table;
    int ino_per_block = vol->blocksize / sizeof(ext2_ino_t);
    blk += index / ino_per_block;
    index = index % ino_per_block;
    ext2_ino_t *ptr = bkmap(bk, blk, vol->blocksize, 0, vol->blkdev, flg);
    return &ptr[index];
}

inode_t *ext2_inode_from(ext2_volume_t *vol, ext2_ino_t *entry, unsigned no)
{
    inode_t *ino;
    if ((entry->mode & 0xf000) == EXT2_S_IFDIR)
        ino = vfs_inode(no, FL_DIR, vol->dev, &ext2_dir_ops);
    else if ((entry->mode & 0xf000) == EXT2_S_IFREG)
        ino = vfs_inode(no, FL_REG, vol->dev, &ext2_reg_ops);
    else
        return NULL;

    ino->length = entry->size;
    ino->btime = entry->ctime * _PwNano_;
    ino->ctime = entry->ctime * _PwNano_;
    ino->mtime = entry->mtime * _PwNano_;
    ino->atime = entry->atime * _PwNano_;
    ino->drv_data = vol;
    return ino;
}

inode_t *ext2_inode(ext2_volume_t *vol, uint32_t no)
{
    struct bkmap bk;
    ext2_ino_t *en = ext2_entry(&bk, vol, no, VM_RD);
    inode_t *ino = ext2_inode_from(vol, en, no);
    bkunmap(&bk);
    return ino;
}

inode_t *ext2_creat(ext2_volume_t *vol, uint32_t no, ftype_t type, xtime_t time)
{
    uint32_t group = (no - 1) / vol->sb->inodes_per_group;
    uint32_t index = (no - 1) % vol->sb->inodes_per_group;
    // uint32_t block = (index * vol->sb->inode_size) / (1024 << vol->sb->log_block_size);
    uint32_t blk = vol->grp[group].inode_table;

    size_t off = (1024 << vol->sb->log_block_size) * blk + index * sizeof(ext2_ino_t);
    size_t lba = ALIGN_DW(off, PAGE_SIZE);
    void *ptr = kmap(PAGE_SIZE, vol->blkdev, lba, VM_WR);
    ext2_ino_t *entry = ADDR_OFF(ptr, off - lba);

    entry->mode = EXT2_S_IFDIR | 0755;
    entry->uid = 0;
    entry->size = 0;
    entry->ctime = time / _PwNano_;
    entry->mtime = time / _PwNano_;
    entry->atime = time / _PwNano_;
    entry->dtime = 0;
    entry->gid = 0;
    entry->links = 0;
    entry->blocks = 0;
    entry->flags = 0;
    entry->osd1 = 0;
    memset(entry->osd2, 0, sizeof(entry->osd2));
    memset(entry->block, 0, sizeof(entry->block));
    entry->generation = 0;
    entry->file_acl = 0;
    entry->dir_acl = 0;
    entry->faddr = 0;

    inode_t *ino = NULL;
    if (type == FL_DIR)
        ino = vfs_inode(no, type, vol->dev, &ext2_dir_ops);
    else if (type == FL_REG)
        ino = vfs_inode(no, type, vol->dev, &ext2_reg_ops);
    else {
        kunmap(ptr, PAGE_SIZE);
        return NULL;
    }

    ino->length = entry->size;
    ino->btime = entry->ctime * _PwNano_;
    ino->ctime = entry->ctime * _PwNano_;
    ino->mtime = entry->mtime * _PwNano_;
    ino->atime = entry->atime * _PwNano_;
    ino->drv_data = vol;
    kunmap(ptr, PAGE_SIZE);
    return ino;
}


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

uint32_t ext2_alloc_block(ext2_volume_t *vol)
{
    int i, j, k;

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
    int i = block / vol->sb->blocks_per_group;
    int j = (block - i * vol->sb->blocks_per_group) / 8;
    int k = block % 8;
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
            buf[j] == 0;
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
    int ip = blk / blkr;
    int ix = blk % blkr;
    if (ip < blkr) {
        p = bkmap(&k1, dir->block[13], vol->blocksize, 0, vol->blkdev, VM_RD);
        pp = bkmap(&k2, p[ip], vol->blocksize, 0, vol->blkdev, VM_RD);
        lba = pp[ix];
        bkunmap(&k1);
        bkunmap(&k2);
        return lba;
    }

    int iy = ip / blk;
    int iz = ip % blk;
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

    return -1;
}



static void ext2_truncate_direct(ext2_volume_t *vol, uint32_t *table, int len, int bcount, int offset)
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

static uint32_t ext2_truncate_indirect(ext2_volume_t *vol, uint32_t block, int bcount, int offset, int depth)
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

uint32_t ext2_alloc_inode(ext2_volume_t *vol)
{
    int i, j, k;

    for (i = 0; i < vol->groupCount; ++i) {
        if (vol->grp[i].free_inodes_count != 0)
            break;
    }

    if (i >= vol->groupCount)
        return 0;


    struct bkmap bm;
    uint8_t *bitmap = bkmap(&bm, vol->grp[i].inode_bitmap, vol->blocksize, 0, vol->blkdev, VM_WR);

    for (j = 0; j < vol->blocksize; ++j) {
        if (bitmap[j] == 0xff)
            continue;
        k = 0;
        while (bitmap[j] & (1 << k))
            k++;
        bitmap[j] |= (1 << k);
        vol->grp[i].free_inodes_count--;
        bkunmap(&bm);
        return i * vol->sb->inodes_per_group + j * 8 + k + 1;
    }
    bkunmap(&bm);
    return 0;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

ext2_dir_iter_t *ext2_opendir(inode_t *dir)
{
    assert(dir != NULL);
    assert(dir->type == FL_DIR);

    ext2_dir_iter_t *it = (ext2_dir_iter_t *)kalloc(sizeof(ext2_dir_iter_t));
    ext2_volume_t *vol = (ext2_volume_t *)dir->drv_data;
    it->vol = vol;

    uint32_t group = (dir->no - 1) / vol->sb->inodes_per_group;
    uint32_t index = (dir->no - 1) % vol->sb->inodes_per_group;
    // uint32_t block = (index * vol->sb->inode_size) / (1024 << vol->sb->log_block_size);
    uint32_t blk = vol->grp[group].inode_table;

    size_t off = (1024 << vol->sb->log_block_size) * blk + index * sizeof(ext2_ino_t);
    size_t lba = ALIGN_DW(off, PAGE_SIZE);
    it->emap = kmap(PAGE_SIZE, dir->dev->underlying, lba, VM_RD);
    it->entry = ADDR_OFF(it->emap, off - lba);
    it->idx = 0;
    return it;
}

uint32_t ext2_nextdir(inode_t *dir, char *name, ext2_dir_iter_t *it)
{
    for (;;) {

        uint32_t blk = it->entry->block[it->idx / it->vol->blocksize]; // Look at IDX (for blk and off)

        size_t off = it->vol->blocksize * blk + (it->idx % it->vol->blocksize);
        size_t lba = ALIGN_DW(off, PAGE_SIZE);
        if (it->lba != lba) {
            if (it->cmap)
                kunmap(it->cmap, PAGE_SIZE);
            it->cmap = kmap(PAGE_SIZE, dir->dev->underlying, lba, VM_RD);
            it->lba = lba;
        }

        ext2_dir_en_t *entry = ADDR_OFF(it->cmap, off - lba);
        it->idx += entry->rec_len;
        if (strcmp(entry->name, ".") == 0 || strcmp(entry->name, "..") == 0)
            continue;

        // Assert name is on the block ?
        if (entry->type == 0)
            return 0;

        strncpy(name, entry->name, MIN(255, entry->name_len));
        name[entry->name_len] = '\0';
        return entry->ino;
    }
}


inode_t *ext2_readdir(inode_t *dir, char *name, ext2_dir_iter_t *it)
{
    uint32_t ino_no = ext2_nextdir(dir, name, it);
    if (ino_no == 0)
        return NULL;
    return ext2_inode(dir->drv_data, ino_no);
}

int ext2_closedir(inode_t *dir, ext2_dir_iter_t *it)
{
    ext2_volume_t *vol = (ext2_volume_t *)dir->drv_data;
    if (it->cmap)
        kunmap(it->cmap, PAGE_SIZE);
    if (it->emap)
        kunmap(it->emap, PAGE_SIZE);

    // bio_clean(vol->io, it->blk);
    kfree(it);
    return 0;
}



/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int ext2_search_inode(inode_t *dir, const char *name, char *buf)
{
    ext2_volume_t *vol = (ext2_volume_t *)dir->drv_data;
    ext2_dir_iter_t *it = ext2_opendir(dir);

    int ino_no;
    while ((ino_no = ext2_nextdir(dir, buf, it)) != 0) {
        if (strcmp(name, buf) != 0)
            continue;

        ext2_closedir(dir, it);
        return ino_no;
    }

    ext2_closedir(dir, it);
    return ino_no;
}

int ext2_add_link(ext2_volume_t *vol, ext2_ino_t *dir, unsigned no, const char *name)
{
    int i;
    int lg = strnlen(name, 255);
    int sz = ALIGN_UP(sizeof(ext2_dir_en_t) + lg, 4);
    int pages = dir->blocks / (vol->blocksize / 512);
    for (i = 0; i < pages; ++i) {
        int idx = 0;
        int blk = ext2_get_block(vol, dir, i);
        struct bkmap bk;
        ext2_dir_en_t *en = bkmap(&bk, blk, vol->blocksize, 0, vol->blkdev, VM_WR);
        while (en->ino != 0) {
            if (en->rec_len == 0) {
                bkunmap(&bk);
                errno = EIO;
                return -1;
            }
            if (idx + en->rec_len >= vol->blocksize) {
                if (idx + sizeof(ext2_dir_en_t) + ALIGN_UP(en->name_len, 4) + sz <= vol->blocksize) {
                    // Write here
                    en->rec_len = sizeof(ext2_dir_en_t) + ALIGN_UP(en->name_len, 4);

                    idx += en->rec_len;
                    en = ADDR_OFF(en, en->rec_len);
                    en->ino = no;
                    en->rec_len = vol->blocksize - idx;
                    en->name_len = lg;
                    en->type = 2;
                    strcpy(en->name, name);
                    bkunmap(&bk);
                    errno = 0;
                    return 0;
                }
                break;
            }
            idx += en->rec_len;
            en = ADDR_OFF(en, en->rec_len);
        }
        bkunmap(&bk);
    }

    // No page found !! Add a new page !?
    errno = EIO;
    return -1;
}



inode_t *ext2_lookup(inode_t *dir, const char *name, void *acl)
{
    ext2_volume_t *vol = (ext2_volume_t *)dir->drv_data;
    char *filename = kalloc(256);
    int ino_no = ext2_search_inode(dir, name, filename);
    kfree(filename);
    if (ino_no == 0) {
        errno = ENOENT;
        return NULL;
    }

    inode_t *ino = ext2_inode(dir->drv_data, ino_no);
    errno = 0;
    return ino;
}

inode_t *ext2_create(inode_t *dir, const char *name, int mode, void *acl)
{
    ext2_volume_t *vol = (ext2_volume_t *)dir->drv_data;
    char *filename = kalloc(256);
    int ino_no = ext2_search_inode(dir, name, filename);
    kfree(filename);

    if (ino_no != 0) {
        errno = EEXIST;
        return NULL;
    }

    // Create a file !!
    // uint32_t blkno = ext2_alloc_block(vol);
    uint32_t no = ext2_alloc_inode(vol);

    xtime_t now = xtime_read(XTIME_CLOCK);

    // Update parent inode
    struct bkmap bk_dir;
    ext2_ino_t *ino_dir = ext2_entry(&bk_dir, vol, dir->no, VM_WR);
    ino_dir->links++;
    ino_dir->ctime = now / _PwMicro_;
    ino_dir->mtime = now / _PwMicro_;

    // Create a new inode
    struct bkmap bk_new;
    ext2_ino_t *ino_new = ext2_entry(&bk_new, vol, no, VM_WR);
    ino_new->ctime = now / _PwMicro_;
    ino_new->atime = now / _PwMicro_;
    ino_new->mtime = now / _PwMicro_;
    ino_new->links = 2;
    ino_new->mode = 0755 | EXT2_S_IFREG;
    // ino_new->block[0] = 0;
    ino_new->size = 0; // vol->blocksize;
    ino_new->blocks = 0; // vol->blocksize / 512;

    inode_t *ino = ext2_inode_from(vol, ino_new, no);
    assert(ino != NULL);

    // Add the new entry
    ext2_add_link(vol, ino_dir, no, name);

    bkunmap(&bk_dir);
    bkunmap(&bk_new);
    return ino;
}

inode_t *ext2_mkdir(inode_t *dir, const char *name, int mode, void *acl)
{
    ext2_volume_t *vol = (ext2_volume_t *)dir->drv_data;
    char *filename = kalloc(256);
    int ino_no = ext2_search_inode(dir, name, filename);
    kfree(filename);

    if (ino_no != 0) {
        errno = EEXIST;
        return NULL;
    }

    // Create a new repository
    uint32_t blkno = ext2_alloc_block(vol);
    uint32_t no = ext2_alloc_inode(vol);

    // Add . and ..
    struct bkmap bkd;
    char *data = bkmap(&bkd, blkno, vol->blocksize, 0, vol->blkdev, VM_RW);
    memset(data, 0, vol->blocksize);
    ext2_dir_en_t *en = data;
    en->ino = no;
    en->rec_len = sizeof(ext2_dir_en_t) + 4;
    en->name_len = 1;
    en->type = 2; // Directory (2)
    strcpy(en->name, ".");
    en = ADDR_OFF(data, en->rec_len);
    en->ino = dir->no;
    en->rec_len = sizeof(ext2_dir_en_t) + 4;
    en->name_len = 2;
    en->type = 2;
    strcpy(en->name, "..");
    bkunmap(&bkd);

    xtime_t now = xtime_read(XTIME_CLOCK);

    // Update parent inode
    struct bkmap bk_dir;
    ext2_ino_t *ino_dir = ext2_entry(&bk_dir, vol, dir->no, VM_WR);
    ino_dir->links++;
    ino_dir->ctime = now / _PwMicro_;
    ino_dir->mtime = now / _PwMicro_;

    // Create a new inode
    struct bkmap bk_new;
    ext2_ino_t *ino_new = ext2_entry(&bk_new, vol, no, VM_WR);
    ino_new->ctime = now / _PwMicro_;
    ino_new->atime = now / _PwMicro_;
    ino_new->mtime = now / _PwMicro_;
    ino_new->links = 2;
    ino_new->mode = 0755 | EXT2_S_IFDIR;
    ino_new->block[0] = blkno;
    ino_new->size = vol->blocksize;
    ino_new->blocks = vol->blocksize / 512;

    inode_t *ino = ext2_inode_from(vol, ino_new, no);
    assert(ino != NULL);

    // Add the new entry
    ext2_add_link(vol, ino_dir, no, name);

    bkunmap(&bk_dir);
    bkunmap(&bk_new);
    return ino;
}



inode_t *ext2_open(inode_t *dir, const char *name, ftype_t type, void *acl, int flags)
{
    ext2_volume_t *vol = (ext2_volume_t *)dir->drv_data;
    ext2_dir_iter_t *it = ext2_opendir(dir);

    char filename[256]; // TODO - Avoid large stack allocation !!!
    int ino_no;
    while ((ino_no = ext2_nextdir(dir, filename, it)) != 0) {
        if (strcmp(name, filename) != 0)
            continue;

        // The file exist
        if (!(flags & IO_OPEN)) {
            errno = EEXIST;
            return NULL;
        }

        inode_t *ino = ext2_inode(dir->drv_data, ino_no);
        ext2_closedir(dir, it);
        errno = 0;
        return ino;
    }

    if (!(flags & IO_CREAT)) {
        ext2_closedir(dir, it);
        errno = ENOENT;
        return NULL;
    }

    // Create it
    int last = 0;
    int blk_no = last / vol->blocksize;
    int off = last % vol->blocksize;
    int lba = it->entry->block[blk_no];
    if (it->lba != lba) {
        // TODO
        ext2_closedir(dir, it);
        errno = EIO;
        return NULL;
    }

    ext2_dir_en_t *new_en;
    ext2_dir_en_t *entry = (ext2_dir_en_t *)NULL; // &it->cur_block[off];
    uint32_t no = ext2_alloc_inode(dir->drv_data);
    inode_t *ino = ext2_creat(dir->drv_data, no, FL_REG, xtime_read(XTIME_CLOCK));
    // TODO -- REgister !
    assert(false);
    if (entry->rec_len > entry->name_len + 4 + strlen(name)) {
        int sz = entry->name_len + 2;
        new_en = (ext2_dir_en_t *)NULL; // &it->cur_block[off + sz];
        entry->rec_len = sz;
        new_en->rec_len = entry->rec_len - sz;
        new_en->name_len = (uint8_t)strlen(name);
        new_en->ino = ino->no;
        memcpy(new_en->name, name, new_en->name_len);
    } else {
        // TODO
    }
    ext2_closedir(dir, it);
    errno = 0;
    return ino;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


int ext2_read(inode_t *ino, void *buffer, size_t length, xoff_t offset, int flags)
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
        buffer = ((char *)buffer) + cap;
    }

    bkunmap(&bk);
    return 0;
}

int ext2_write(inode_t *ino, void *buffer, size_t length, xoff_t offset, int flags)
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
        buffer = ((char *)buffer) + cap;
    }

    bkunmap(&bk);
    return 0;
}


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int ext2_truncate(inode_t *ino, xoff_t offset)
{
    struct bkmap bk;
    ext2_volume_t *vol = (ext2_volume_t *)ino->drv_data;
    ext2_ino_t *en = ext2_entry(&bk, vol, ino->no, VM_WR);

    int block_count = ALIGN_UP(offset, vol->blocksize) / vol->blocksize;
    int i = 0;
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

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


ino_ops_t ext2_dir_ops = {
    .lookup = ext2_lookup,
    .create = ext2_create,
    // .open = ext2_open,
    .opendir = (void *)ext2_opendir,
    .readdir = (void *)ext2_readdir,
    .closedir = (void *)ext2_closedir,
    .mkdir = ext2_mkdir,
};

ino_ops_t ext2_reg_ops = {
    .read = ext2_read,
    .write = ext2_write,
    .close = NULL,
    .truncate = ext2_truncate,
};


inode_t *ext2_mount(inode_t *dev, const char *options)
{
    ext2_volume_t *vol = kalloc(sizeof(ext2_volume_t));

    uint8_t *ptr = kmap(PAGE_SIZE, dev, 0, VM_RD);
    ext2_sb_t *sb = (ext2_sb_t *)&ptr[1024];
    vol->sb = sb;

    if (sb->magic != 0xEF53 || sb->blocks_per_group == 0) {
        kunmap(ptr, PAGE_SIZE);
        errno = EBADF;
        return NULL;
    }

    kprintf(-1, "File system label = %s\n", sb->volume_name);
    kprintf(-1, "Block size = %d\n", (1024 << sb->log_block_size));
    kprintf(-1, "Fragment size = %d\n", (1024 << sb->log_frag_size));
    kprintf(-1, "%d inodes, %d blocks\n", sb->inodes_count, sb->blocks_count);
    kprintf(-1, "%d blocks reserved for super user\n", sb->rsvd_blocks_count);
    kprintf(-1, "First data block = %d\n", sb->first_data_block);
    kprintf(-1, "Maximum filesystem blocks = %d\n", 0);
    kprintf(-1, "%d block groups\n", sb->blocks_count / sb->blocks_per_group);
    kprintf(-1, "%d blocks per group\n", sb->blocks_per_group);
    kprintf(-1, "%d frags per group\n", sb->frags_per_group);
    kprintf(-1, "%d inodes per group\n", sb->inodes_per_group);
    kprintf(-1, "Superblock backup on blocks: %d\n", 0);

    size_t inosz = sizeof(ext2_ino_t);
    size_t table_ino = inosz * sb->inodes_per_group;

    // vol->dev = vfs_open_inode(dev);
    vol->blocksize = 1024 << sb->log_block_size;
    assert(POW2(vol->blocksize));

    vol->grp = (ext2_grp_t *)&ptr[MAX(2048, vol->blocksize)];
    // vol->io = bio_create(dev, VMA_FILE_RW, 1024 << sb->log_block_size, 0);

    vol->groupCount = sb->blocks_count / sb->blocks_per_group;
    vol->groupSize = vol->groupCount * sizeof(ext2_grp_t);

    int i;
    size_t grp_info = vol->groupCount * sizeof(ext2_grp_t);
    for (i = 0; i < vol->groupCount; ++i) {
        int bb = vol->grp[i].block_bitmap;
        int ib = vol->grp[i].inode_bitmap;
        int it = vol->grp[i].inode_table;
        kprintf(-1, "Group %d :: %d blocks bitmap, %d inodes bitmap, %d inodes table\n", i, bb, ib, it);
    }

    vol->blkdev = vfs_open_inode(dev);
    inode_t *ino = ext2_inode(vol, 2);
    if (ino == NULL) {
        vfs_close_inode(dev);
        kunmap(ptr, PAGE_SIZE);
        kfree(vol);
        errno = EBADF;
        return NULL;
    }

    vol->dev = ino->dev;
    ino->dev->devname = sb->volume_name;
    ino->dev->devclass = "ext2";
    ino->dev->underlying = vfs_open_inode(dev);
    errno = 0;
    return ino;
}


void ext2_setup()
{
    vfs_addfs("ext2", ext2_mount);
}


void ext2_teardown()
{
    vfs_rmfs("ext2");
}

EXPORT_MODULE(ext2, ext2_setup, ext2_teardown);
