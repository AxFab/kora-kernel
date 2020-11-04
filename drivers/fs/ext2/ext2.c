/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2019  <Fabien Bavent>
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

inode_t *ext2_inode(inode_t *underlying, device_t *dev, ext2_volume_t *vol, uint32_t no)
{
    uint32_t group = (no - 1) / vol->sb->inodes_per_group;
    uint32_t index = (no - 1) % vol->sb->inodes_per_group;
    // uint32_t block = (index * vol->sb->inode_size) / (1024 << vol->sb->log_block_size);
    uint32_t blk = vol->grp[group].inode_table;

    size_t off = (1024 << vol->sb->log_block_size) * blk + index * sizeof(ext2_ino_t);
    size_t lba = ALIGN_DW(off, PAGE_SIZE);
    void *ptr = kmap(PAGE_SIZE, underlying, lba, VM_RD);
    ext2_ino_t* entry = ADDR_OFF(ptr, off - lba);
    // entry += index;

    // int mode = entry->mode & 07777;
    inode_t *ino;
    if ((entry->mode & 0xf000) == EXT2_S_IFDIR)
        ino = vfs_inode(no, FL_DIR, dev, &ext2_dir_ops);
    else if ((entry->mode & 0xf000) == EXT2_S_IFREG)
        ino = vfs_inode(no, FL_REG, dev, &ext2_reg_ops);
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

int ext2_format(inode_t *dev)
{
    // uint8_t *ptr = kmap(PAGE_SIZE, dev, 0, VMA_FILE_RW);
    // ext2_sb_t *sb = (ext2_sb_t *)&ptr[1024];
    return -1;
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
    size_t blksize = (1024 << it->vol->sb->log_block_size);
    for (;;) {

        uint32_t blk = it->entry->block[it->idx / blksize]; // Look at IDX (for blk and off)

        size_t off = (1024 << it->vol->sb->log_block_size) * blk + (it->idx % blksize);
        size_t lba = ALIGN_DW(off, PAGE_SIZE);
        if (it->lba != lba) {
            if (it->cmap)
                kunmap(it->cmap, PAGE_SIZE);
            it->cmap = kmap(PAGE_SIZE, dir->dev->underlying, lba, VM_RD);
            it->lba = lba;
        }

        ext2_dir_en_t* entry = ADDR_OFF(it->cmap, off - lba);
        it->idx += entry->size;
        if (strcmp(entry->name, ".") == 0 || strcmp(entry->name, "..") == 0) {
            continue;
        }

        // Assert name is on the block ?
        if (entry->type == 0)
            return 0;

        strncpy(name, entry->name, MIN(255, entry->length));
        name[entry->length] = '\0';
        return entry->ino;
    }
}


inode_t *ext2_readdir(inode_t *dir, char *name, ext2_dir_iter_t *it)
{
    uint32_t ino_no = ext2_nextdir(dir, name, it);
    if (ino_no == 0)
        return NULL;
    return ext2_inode(dir->dev->underlying, dir->dev, dir->drv_data, ino_no);
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


inode_t *ext2_creat(device_t *volume, ftype_t type, void *acl)
{
    uint32_t no = 0;
    // TODO
    if (no == 0)
        return NULL;
    return NULL; // vfs_inode(no, type, volume);
}


inode_t *ext2_open(inode_t *dir, const char * name, ftype_t type, void *acl, int flags)
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

        inode_t *ino = ext2_inode(dir->dev->underlying, dir->dev, dir->drv_data, ino_no);
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
    int blk_no = last / (1024 << vol->sb->log_block_size);
    int off = last % (1024 << vol->sb->log_block_size);
    int lba = it->entry->block[blk_no];
    if (it->lba != lba) {
        // TODO
        ext2_closedir(dir, it);
        errno = EIO;
        return NULL;
    }

    ext2_dir_en_t *new_en;
    ext2_dir_en_t* entry = (ext2_dir_en_t*)NULL;// &it->cur_block[off];
    inode_t *ino = ext2_creat(dir->dev, type, acl);
    if (entry->size > entry->length + 4 + strlen(name)) {
        int sz = entry->length + 2;
        new_en = (ext2_dir_en_t*)NULL; // &it->cur_block[off + sz];
        entry->size = sz;
        new_en->size = entry->size - sz;
        new_en->length = (uint8_t)strlen(name);
        new_en->ino = ino->no;
        memcpy(new_en->name, name, new_en->length);
    } else {
        // TODO
    }
    ext2_closedir(dir, it);
    errno = 0;
    return ino;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


int ext2_read(inode_t* ino, void* buffer, size_t length, xoff_t offset, int flags)
{

    return 0;
}

int ext2_write(inode_t* ino, void* buffer, size_t length, xoff_t offset, int flags)
{
    assert(false);
    return 0;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */



ino_ops_t ext2_dir_ops = {
    .open = ext2_open,
    .opendir = (void *)ext2_opendir,
    .readdir = (void *)ext2_readdir,
    .closedir = (void *)ext2_closedir,
};

ino_ops_t ext2_reg_ops = {
    .read = ext2_read,
    .write = ext2_write,
    .close = NULL,
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

    // vol->dev = vfs_open_inode(dev);
    vol->grp = (ext2_grp_t *)&ptr[MAX(2048, 1024 << sb->log_block_size)];
    // vol->io = bio_create(dev, VMA_FILE_RW, 1024 << sb->log_block_size, 0);

    int i, n;
    for (i = 0, n = sb->blocks_count / sb->blocks_per_group; i < n; ++i) {
        int bb = vol->grp[i].block_bitmap;
        int ib = vol->grp[i].inode_bitmap;
        int it = vol->grp[i].inode_table;
        int dt = sb->blocks_per_group - bb - ib - it;
        // kprintf(-1, "Group %d block bitmap, %d inode bitmap, %d inodes, %d data\n", ib, bb, it, dt);
    }

    inode_t *ino = ext2_inode(dev, NULL, vol, 2);
    if (ino == NULL) {
        kunmap(ptr, PAGE_SIZE);
        errno = EBADF;
        return NULL;
    }

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

