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




extern ino_ops_t ext2_dir_ops;
extern ino_ops_t ext2_reg_ops;

ext2_ino_t* ext2_entry(struct bkmap* bk, ext2_volume_t* vol, unsigned no, int flg)
{
    uint32_t group = (no - 1) / vol->sb->inodes_per_group;
    uint32_t index = (no - 1) % vol->sb->inodes_per_group;
    uint32_t blk = vol->grp[group].inode_table;
    int ino_per_block = vol->blocksize / sizeof(ext2_ino_t);
    blk += index / ino_per_block;
    index = index % ino_per_block;
    ext2_ino_t* ptr = bkmap(bk, blk, vol->blocksize, 0, vol->blkdev, flg);
    return &ptr[index];
}

inode_t* ext2_inode_from(ext2_volume_t* vol, ext2_ino_t* entry, unsigned no)
{
    inode_t* ino;
    if ((entry->mode & 0xf000) == EXT2_S_IFDIR)
        ino = vfs_inode(no, FL_DIR, vol->dev, &ext2_dir_ops);
    else if ((entry->mode & 0xf000) == EXT2_S_IFREG)
        ino = vfs_inode(no, FL_REG, vol->dev, &ext2_reg_ops);
    else
        return NULL;

    ino->length = entry->size;
    ino->mode = entry->mode & 0xFFF;
    ino->btime = (uint64_t)entry->ctime * _PwMicro_;
    ino->ctime = (uint64_t)entry->ctime * _PwMicro_;
    ino->mtime = (uint64_t)entry->mtime * _PwMicro_;
    ino->atime = (uint64_t)entry->atime * _PwMicro_;
    ino->links = entry->links;
    ino->drv_data = vol;
    return ino;
}

inode_t* ext2_inode(ext2_volume_t* vol, uint32_t no)
{
    struct bkmap bk;
    ext2_ino_t* en = ext2_entry(&bk, vol, no, VM_RD);
    inode_t* ino = ext2_inode_from(vol, en, no);
    bkunmap(&bk);
    return ino;
}

inode_t* ext2_creat(ext2_volume_t* vol, uint32_t no, ftype_t type, xtime_t time)
{
    uint32_t group = (no - 1) / vol->sb->inodes_per_group;
    uint32_t index = (no - 1) % vol->sb->inodes_per_group;
    // uint32_t block = (index * vol->sb->inode_size) / (1024 << vol->sb->log_block_size);
    uint32_t blk = vol->grp[group].inode_table;

    size_t off = (1024 << vol->sb->log_block_size) * blk + index * sizeof(ext2_ino_t);
    size_t lba = ALIGN_DW(off, PAGE_SIZE);
    void* ptr = kmap(PAGE_SIZE, vol->blkdev, lba, VM_WR);
    ext2_ino_t* entry = ADDR_OFF(ptr, off - lba);

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

    inode_t* ino = NULL;
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


uint32_t ext2_alloc_inode(ext2_volume_t* vol)
{
    unsigned i, j, k;

    for (i = 0; i < vol->groupCount; ++i) {
        if (vol->grp[i].free_inodes_count != 0)
            break;
    }

    if (i >= vol->groupCount)
        return 0;


    struct bkmap bm;
    uint8_t* bitmap = bkmap(&bm, vol->grp[i].inode_bitmap, vol->blocksize, 0, vol->blkdev, VM_WR);

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

static void ext2_make_empty_dir(ext2_volume_t* vol, uint32_t blkno, int no, int dir_no)
{
    // Add . and ..
    struct bkmap bkd;
    char* data = bkmap(&bkd, blkno, vol->blocksize, 0, vol->blkdev, VM_RW);
    memset(data, 0, vol->blocksize);
    ext2_dir_hack_t* dir = (ext2_dir_hack_t*)data;

    // Create '.' entry
    dir->ino1 = no;
    dir->rec_len1 = 12;
    dir->name_len1 = 1;
    dir->file_type1 = EXT2_FT_DIR;
    dir->name1[0] = '.';

    // Create '..' entry
    dir->ino2 = dir_no;
    dir->rec_len2 = vol->blocksize - 12;
    dir->name_len2 = 2;
    dir->file_type2 = EXT2_FT_DIR;
    dir->name2[0] = '.';
    dir->name2[1] = '.';

    bkunmap(&bkd);
}

//static inode_t *ext2_make_newinode(ext2_volume_t *vol, int no, xtime_t now, int mode, uint32_t blkno)
//{
//    struct bkmap bkn;
//    ext2_ino_t *ino_new = ext2_entry(&bkn, vol, no, VM_WR);
//    ino_new->ctime = now / _PwMicro_;
//    ino_new->atime = now / _PwMicro_;
//    ino_new->mtime = now / _PwMicro_;
//    ino_new->links = mode & EXT2_S_IFDIR ? 2 : 1;
//    ino_new->mode = mode;
//    ino_new->block[0] = blkno;
//    ino_new->size = vol->blocksize;
//    ino_new->blocks = vol->blocksize / 512;
//
//    inode_t *ino = ext2_inode_from(vol, ino_new, no);
//    assert(ino != NULL);
//
//    bkunmap(&bkn);
//    return ino;
//}


int ext2_search_inode(inode_t* dir, const char* name, char* buf)
{
    ext2_dir_iter_t* it = ext2_opendir(dir);

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


int ext2_add_link(ext2_volume_t* vol, ext2_ino_t* dir, unsigned no, const char* name)
{
    unsigned i;
    unsigned lg = strnlen(name, 255);
    unsigned sz = ALIGN_UP(sizeof(ext2_dir_en_t) + lg, 4);
    unsigned pages = dir->blocks / (vol->blocksize / 512);
    for (i = 0; i < pages; ++i) {
        unsigned idx = 0;
        unsigned blk = ext2_get_block(vol, dir, i);
        struct bkmap bk;
        ext2_dir_en_t* en = bkmap(&bk, blk, vol->blocksize, 0, vol->blkdev, VM_WR);
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

int ext2_rm_link(ext2_volume_t *vol, ext2_ino_t *dir, unsigned no)
{
    unsigned i;
    unsigned pages = dir->blocks / (vol->blocksize / 512);
    for (i = 0; i < pages; ++i) {
        unsigned idx = 0;
        unsigned blk = ext2_get_block(vol, dir, i);
        struct bkmap bk;
        ext2_dir_en_t *prev = NULL;
        ext2_dir_en_t *next = NULL;
        ext2_dir_en_t *en = bkmap(&bk, blk, vol->blocksize, 0, vol->blkdev, VM_WR);
        ext2_dir_en_t *start = en;
        while (en->ino != 0) {
            if (idx + en->rec_len >= vol->blocksize)
                next = NULL;
            else
                next = ADDR_OFF(en, en->rec_len);

            if (en->rec_len == 0) {
                bkunmap(&bk);
                errno = EIO;
                return -1;
            }
            if (en->ino == no) {
                next = ADDR_OFF(en, en->rec_len);
                if ((size_t)next >= (size_t)start + vol->blocksize) {
                    prev->rec_len += en->rec_len;
                } else {
                    unsigned sz = en->rec_len;
                    unsigned lf = (size_t)start + vol->blocksize - (size_t)next;
                    if (lf > 0)
                        memmove(en, next, lf);
                    while (en->rec_len > 0 && idx + en->rec_len + sz < lf)
                        en = ADDR_OFF(en, en->rec_len);
                    if (en->rec_len == 0) {
                        bkunmap(&bk);
                        errno = EIO;
                        return -1;
                    }
                    en->rec_len += sz;
                    unsigned check = (size_t)start + vol->blocksize - (size_t)en;
                    if (en->rec_len != check) {
                        bkunmap(&bk);
                        errno = EIO;
                        return -1;
                    }
                }

                bkunmap(&bk);
                return 0;
            }

            // Nothing more on this block
            if (next == NULL)
                break; 

            idx += en->rec_len;
            prev = en;
            en = next;
        }
        bkunmap(&bk);
    }

    // No page found !! Add a new page !?
    errno = EIO;
    return -1;
}


inode_t* ext2_lookup(inode_t* dir, const char* name, void* acl)
{
    char* filename = kalloc(256);
    int ino_no = ext2_search_inode(dir, name, filename);
    kfree(filename);
    if (ino_no == 0) {
        errno = ENOENT;
        return NULL;
    }

    inode_t* ino = ext2_inode(dir->drv_data, ino_no);
    errno = 0;
    return ino;
}

inode_t* ext2_create(inode_t* dir, const char* name, void* acl, int mode)
{
    ext2_volume_t* vol = (ext2_volume_t*)dir->drv_data;
    char* filename = kalloc(256);
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
    ext2_ino_t* ino_dir = ext2_entry(&bk_dir, vol, dir->no, VM_WR);
    ino_dir->links++;
    ino_dir->ctime = now / _PwMicro_;
    ino_dir->mtime = now / _PwMicro_;
    dir->ctime = ino_dir->ctime * _PwMicro_;
    dir->ctime = ino_dir->mtime * _PwMicro_;
    dir->links = ino_dir->links;

    // Create a new inode
     // inode_t *ino = ext2_make_newinode(vol, no, now, EXT2_S_IFREG | 0644, 0);

    struct bkmap bk_new;
    ext2_ino_t* ino_new = ext2_entry(&bk_new, vol, no, VM_WR);
    memset(ino_new, 0, sizeof(ext2_ino_t));
    ino_new->ctime = now / _PwMicro_;
    ino_new->atime = now / _PwMicro_;
    ino_new->mtime = now / _PwMicro_;
    ino_new->links = 1;
    ino_new->mode = (mode & 0xFFF) | EXT2_S_IFREG;
    
    // ino_new->block[0] = 0;
    ino_new->size = 0; // vol->blocksize;
    ino_new->blocks = 0; // vol->blocksize / 512;

    inode_t* ino = ext2_inode_from(vol, ino_new, no);
    assert(ino != NULL);

    // Add the new entry
    int ret = ext2_add_link(vol, ino_dir, no, name);
    if (ret != 0) {
        // TODO -- Rollback !
    }

    bkunmap(&bk_dir);
    bkunmap(&bk_new);
    return ino;
}


inode_t* ext2_mkdir(inode_t* dir, const char* name, int mode, void* acl)
{
    ext2_volume_t* vol = (ext2_volume_t*)dir->drv_data;
    char* filename = kalloc(256);
    int ino_no = ext2_search_inode(dir, name, filename);
    kfree(filename);

    if (ino_no != 0) {
        errno = EEXIST;
        return NULL;
    }

    // Create a new repository
    uint32_t blkno = ext2_alloc_block(vol);
    uint32_t no = ext2_alloc_inode(vol);

    ext2_make_empty_dir(vol, blkno, no, dir->no);

    xtime_t now = xtime_read(XTIME_CLOCK);

    // Update parent inode
    struct bkmap bk_dir;
    ext2_ino_t* ino_dir = ext2_entry(&bk_dir, vol, dir->no, VM_WR);
    ino_dir->links++;
    ino_dir->ctime = now / _PwMicro_;
    ino_dir->mtime = now / _PwMicro_;
    dir->ctime = ino_dir->ctime * _PwMicro_;
    dir->ctime = ino_dir->mtime * _PwMicro_;
    dir->links = ino_dir->links;


    // Create a new inode
    // inode_t* ino = ext2_make_newinode(vol, no, now, EXT2_S_IFDIR | 0755, blkno);
    struct bkmap bk_new;
    ext2_ino_t* ino_new = ext2_entry(&bk_new, vol, no, VM_WR);
    ino_new->ctime = now / _PwMicro_;
    ino_new->atime = now / _PwMicro_;
    ino_new->mtime = now / _PwMicro_;
    ino_new->links = 2;
    ino_new->mode = (mode & 0xFFF) | EXT2_S_IFDIR;
    ino_new->block[0] = blkno;
    ino_new->size = vol->blocksize;
    ino_new->blocks = vol->blocksize / 512;

    inode_t* ino = ext2_inode_from(vol, ino_new, no);
    assert(ino != NULL);

    // Add the new entry
    ext2_add_link(vol, ino_dir, no, name);

    bkunmap(&bk_dir);
    bkunmap(&bk_new);
    return ino;
}

int ext2_rmdir(inode_t *dir, const char *name)
{
    ext2_volume_t *vol = (ext2_volume_t *)dir->drv_data;
    char *filename = kalloc(256);
    int ino_no = ext2_search_inode(dir, name, filename);
    kfree(filename);
    if (ino_no == 0) {
        errno = ENOENT;
        return -1;
    }

    struct bkmap bk_new;
    ext2_ino_t *ino_new = ext2_entry(&bk_new, vol, ino_no, VM_WR);
    if ((ino_new->mode & EXT2_S_IFMT) != EXT2_S_IFDIR) {
        bkunmap(&bk_new);
        errno = ENOTDIR;
        return -1;
    }

    // TODO - Check directory is null
    if (ext2_dir_is_empty(vol, ino_new) != 0) {
        bkunmap(&bk_new);
        errno = ENOTEMPTY;
        return -1;
    }

    // Unlink the inode
    struct bkmap bk_dir;
    ext2_ino_t *ino_dir = ext2_entry(&bk_dir, vol, dir->no, VM_WR);
    int link = ext2_rm_link(vol, ino_dir, ino_no);
    
    // Update parent
    xtime_t now = xtime_read(XTIME_CLOCK);
    ino_dir->ctime = now / _PwMicro_;
    ino_dir->mtime = now / _PwMicro_;
    ino_dir->links--;
    dir->ctime = ino_dir->ctime * _PwMicro_;
    dir->ctime = ino_dir->mtime * _PwMicro_;
    dir->links = ino_dir->links;

    if (ino_new->links == 2) {
        // Copy blocks
        uint32_t blocks[15];
        memcpy(blocks, ino_new->block, sizeof(ino_new->block));

        // Free inodes

        // Free blocks
    } else {
        inode_t *ino = ext2_inode(vol, ino_no);
        ino_new->links--;
        ino_new->ctime = now / _PwMicro_;
        ino_new->mtime = now / _PwMicro_;
        ino->ctime = ino_new->ctime * _PwMicro_;
        ino->ctime = ino_new->mtime * _PwMicro_;
        ino->links = ino_new->links;
    }

    bkunmap(&bk_dir);
    bkunmap(&bk_new);
    return 0;
}

int ext2_unlink(inode_t *dir, const char *name)
{
    ext2_volume_t *vol = (ext2_volume_t *)dir->drv_data;
    char *filename = kalloc(256);
    int ino_no = ext2_search_inode(dir, name, filename);
    kfree(filename);
    if (ino_no == 0) {
        errno = ENOENT;
        return -1;
    }

    struct bkmap bk_new;
    ext2_ino_t *ino_new = ext2_entry(&bk_new, vol, ino_no, VM_WR);
    if ((ino_new->mode & EXT2_S_IFMT) == EXT2_S_IFDIR) {
        bkunmap(&bk_new);
        errno = EPERM;
        return -1;
    }

    // Unlink the inode
    struct bkmap bk_dir;
    ext2_ino_t *ino_dir = ext2_entry(&bk_dir, vol, dir->no, VM_WR);
    int link = ext2_rm_link(vol, ino_dir, ino_no);

    // Update parent
    xtime_t now = xtime_read(XTIME_CLOCK);
    ino_dir->ctime = now / _PwMicro_;
    ino_dir->mtime = now / _PwMicro_;
    dir->ctime = ino_dir->ctime * _PwMicro_;
    dir->ctime = ino_dir->mtime * _PwMicro_;
    dir->links = ino_dir->links;
    ino_new->links--;

    if (ino_new->links == 0) {
        // Copy blocks
        uint32_t blocks[15];
        memcpy(blocks, ino_new->block, sizeof(ino_new->block));

        // Free inodes

        // Free blocks
    } else {
        inode_t *ino = ext2_inode(vol, ino_no);
        ino_new->ctime = now / _PwMicro_;
        ino_new->mtime = now / _PwMicro_;
        ino->ctime = ino_new->ctime * _PwMicro_;
        ino->ctime = ino_new->mtime * _PwMicro_;
        ino->links = ino_new->links;
    }

    bkunmap(&bk_dir);
    bkunmap(&bk_new);
    return 0;
}

inode_t* ext2_open(inode_t* dir, const char* name, ftype_t type, void* acl, int flags)
{
    ext2_volume_t* vol = (ext2_volume_t*)dir->drv_data;
    ext2_dir_iter_t* it = ext2_opendir(dir);

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

        inode_t* ino = ext2_inode(dir->drv_data, ino_no);
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
    unsigned last = 0;
    unsigned blk_no = last / vol->blocksize;
    // int off = last % vol->blocksize;
    unsigned lba = it->entry->block[blk_no];
    if (it->lba != lba) {
        // TODO
        ext2_closedir(dir, it);
        errno = EIO;
        return NULL;
    }

    ext2_dir_en_t* new_en;
    ext2_dir_en_t* entry = (ext2_dir_en_t*)NULL; // &it->cur_block[off];
    uint32_t no = ext2_alloc_inode(dir->drv_data);
    inode_t* ino = ext2_creat(dir->drv_data, no, FL_REG, xtime_read(XTIME_CLOCK));
    // TODO -- REgister !
    assert(false);
    if (entry->rec_len > entry->name_len + 4 + strlen(name)) {
        int sz = entry->name_len + 2;
        new_en = (ext2_dir_en_t*)NULL; // &it->cur_block[off + sz];
        entry->rec_len = sz;
        new_en->rec_len = entry->rec_len - sz;
        new_en->name_len = (uint8_t)strlen(name);
        new_en->ino = ino->no;
        memcpy(new_en->name, name, new_en->name_len);
    }
    else {
        // TODO
    }
    ext2_closedir(dir, it);
    errno = 0;
    return ino;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


ino_ops_t ext2_dir_ops = {
    .lookup = ext2_lookup,
    .create = ext2_create,
    .unlink = ext2_unlink,
    // .open = ext2_open,
    .opendir = (void*)ext2_opendir,
    .readdir = (void*)ext2_readdir,
    .closedir = (void*)ext2_closedir,
    .mkdir = ext2_mkdir,
    .rmdir = ext2_rmdir,
};

ino_ops_t ext2_reg_ops = {
    .read = ext2_read,
    .write = ext2_write,
    .close = NULL,
    .truncate = ext2_truncate,
};


inode_t* ext2_mount(inode_t* dev, const char* options)
{
    ext2_volume_t* vol = kalloc(sizeof(ext2_volume_t));

    uint8_t* ptr = kmap(PAGE_SIZE, dev, 0, VM_RD);
    ext2_sb_t* sb = (ext2_sb_t*)&ptr[1024];
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

    vol->blocksize = 1024 << sb->log_block_size;
    assert(POW2(vol->blocksize));

    vol->grp = (ext2_grp_t*)&ptr[MAX(2048, vol->blocksize)];
    vol->groupCount = sb->blocks_count / sb->blocks_per_group;
    vol->groupSize = vol->groupCount * sizeof(ext2_grp_t);

    unsigned i;
    for (i = 0; i < vol->groupCount; ++i) {
        int bb = vol->grp[i].block_bitmap;
        int ib = vol->grp[i].inode_bitmap;
        int it = vol->grp[i].inode_table;
        kprintf(-1, "Group %d :: %d blocks bitmap, %d inodes bitmap, %d inodes table\n", i, bb, ib, it);
    }

    vol->blkdev = vfs_open_inode(dev);
    inode_t* ino = ext2_inode(vol, 2);
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


int ext2_format(inode_t* dev, const char* options);

void ext2_setup()
{
    vfs_addfs("ext2", ext2_mount, ext2_format);
}


void ext2_teardown()
{
    vfs_rmfs("ext2");
}

EXPORT_MODULE(ext2, ext2_setup, ext2_teardown);
