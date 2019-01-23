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
#include <kernel/core.h>
#include <kernel/device.h>
#include <string.h>
#include <errno.h>
#include "ext2.h"



extern ino_ops_t ext2_dir_ops;
extern ino_ops_t ext2_reg_ops;
extern fs_ops_t ext2_fs_ops;

inode_t *ext2_inode(volume_t *volume, uint32_t no)
{
    ext2_volume_t *vol = (ext2_volume_t *) volume->info;
    uint32_t group = (no - 1) / vol->sb->inodes_per_group;
    uint32_t index = (no - 1) % vol->sb->inodes_per_group;
    // uint32_t block = (index * vol->sb->inode_size) / (1024 << vol->sb->log_block_size);
    uint32_t blk = vol->grp[group].inode_table;

    ext2_ino_t *entry = bio_access(vol->io, blk);
    entry += index;

    // int mode = entry->mode & 07777;
    ftype_t type;
    if ((entry->mode & 0xf000) == EXT2_S_IFDIR)
        type = FL_DIR;
    else if ((entry->mode & 0xf000) == EXT2_S_IFREG)
        type = FL_REG;
    else {
        bio_clean(vol->io, blk);
        return NULL;
    }

    inode_t *ino = vfs_inode(no, type, volume);
    ino->length = entry->size;
    ino->btime = entry->ctime * _PwNano_;
    ino->ctime = entry->ctime * _PwNano_;
    ino->mtime = entry->mtime * _PwNano_;
    ino->atime = entry->atime * _PwNano_;
    if (type == FL_DIR)
        ino->ops = &ext2_dir_ops;
    else if (type == FL_DIR)
        ino->ops = &ext2_reg_ops;
    bio_clean(vol->io, blk);
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
    assert(VFS_ISDIR(dir));

    ext2_dir_iter_t *it = (ext2_dir_iter_t *)kalloc(sizeof(ext2_dir_iter_t));
    ext2_volume_t *vol = (ext2_volume_t *)dir->und.vol->info;
    it->vol = (ext2_volume_t *)dir->und.vol->info;

    uint32_t group = (dir->no - 1) / vol->sb->inodes_per_group;
    uint32_t index = (dir->no - 1) % vol->sb->inodes_per_group;
    // uint32_t block = (index * vol->sb->inode_size) / (1024 << vol->sb->log_block_size);
    it->blk = vol->grp[group].inode_table;
    it->entry = bio_access(vol->io, it->blk);
    it->entry += index;
    it->idx = 0;
    return it;
}

uint32_t ext2_nextdir(inode_t *dir, char *name, ext2_dir_iter_t *it)
{
    int blk_no = it->idx / (1024 << it->vol->sb->log_block_size);
    if (it->idx >= dir->length || it->entry->block[blk_no] == 0)
        return 0;

    int lba = it->entry->block[blk_no];
    int off = it->idx % (1024 << it->vol->sb->log_block_size);
    if (it->lba != lba) {
        it->cur_block = bio_access(it->vol->io, lba);
        it->lba = lba;
    }
    ext2_dir_en_t *entry = (ext2_dir_en_t *)&it->cur_block[off];
    it->last = it->idx;
    it->idx += entry->size;
    if (entry->type == 0)
        return 0;

    strncpy(name, entry->name, MIN(255, entry->length));
    name[entry->length] = '\0';
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
        return ext2_nextdir(dir, name, it); // TODO - No recursion allowed !
    return entry->ino;
}


inode_t *ext2_readdir(inode_t *dir, char *name, ext2_dir_iter_t *it)
{
    uint32_t ino_no = ext2_nextdir(dir, name, it);
    if (ino_no == 0)
        return NULL;
    return ext2_inode(dir->und.vol, ino_no);
}

int ext2_closedir(inode_t *dir, ext2_dir_iter_t *it)
{
    ext2_volume_t *vol = (ext2_volume_t *)dir->und.vol->info;
    bio_clean(vol->io, it->blk);
    kfree(it);
    return 0;
}



/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


inode_t *ext2_creat(volume_t *volume, ftype_t type, acl_t *acl)
{
    uint32_t no = 0;
    // TODO
    if (no == 0)
        return NULL;
    return vfs_inode(no, type, volume);
}


inode_t *ext2_open(inode_t *dir, CSTR name, ftype_t type, acl_t *acl, int flags)
{
    ext2_volume_t *vol = (ext2_volume_t *)dir->und.vol->info;
    ext2_dir_iter_t *it = ext2_opendir(dir);

    char filename[256]; // TODO - Avoid large stack allocation !!!
    int ino_no;
    while ((ino_no = ext2_nextdir(dir, filename, it)) != 0) {
        if (strcmp(name, filename) != 0)
            continue;

        // The file exist
        if (!(flags & VFS_OPEN)) {
            errno = EEXIST;
            return NULL;
        }

        inode_t *ino = ext2_inode(dir->und.vol, ino_no);
        ext2_closedir(dir, it);
        errno = 0;
        return ino;
    }

    if (!(flags & VFS_CREAT)) {
        ext2_closedir(dir, it);
        errno = ENOENT;
        return NULL;
    }

    // Create it
    int blk_no = it->last / (1024 << vol->sb->log_block_size);
    int off = it->last % (1024 << vol->sb->log_block_size);
    int lba = it->entry->block[blk_no];
    if (it->lba != lba) {
        // TODO
        ext2_closedir(dir, it);
        errno = EIO;
        return NULL;
    }

    ext2_dir_en_t *new_en;
    ext2_dir_en_t *entry = (ext2_dir_en_t *)&it->cur_block[off];
    inode_t *ino = ext2_creat(dir->und.vol, type, acl);
    if (entry->size > entry->length + 4 + strlen(name)) {
        int sz = entry->length + 2;
        new_en = (ext2_dir_en_t *)&it->cur_block[off + sz];
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



ino_ops_t ext2_dir_ops = {
    .opendir = (void *)ext2_opendir,
    .readdir = (void *)ext2_readdir,
    .closedir = (void *)ext2_closedir,
};

ino_ops_t ext2_reg_ops = {
};

fs_ops_t ext2_fs_ops = {
    .open = ext2_open,
};

inode_t *ext2_mount(inode_t *dev)
{
    ext2_volume_t *vol = kalloc(sizeof(ext2_volume_t));

    uint8_t *ptr = kmap(PAGE_SIZE, dev, 0, VMA_FILE_RO);
    ext2_sb_t *sb = (ext2_sb_t *)&ptr[1024];
    vol->sb = sb;

    kprintf(-1, "SB %d\n", sizeof(*sb));

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

    if (sb->magic != 0xEF53) {
        kunmap(ptr, PAGE_SIZE);
        errno = EBADF;
        return NULL;
    }

    vol->grp = (ext2_grp_t *)&ptr[MAX(2048, 1024 << sb->log_block_size)];
    vol->io = bio_create(dev, VMA_FILE_RW, 1024 << sb->log_block_size, 0);

    int i, n;
    for (i = 0, n = sb->blocks_count / sb->blocks_per_group; i < n; ++i) {
        int bb = vol->grp[i].block_bitmap;
        int ib = vol->grp[i].inode_bitmap;
        int it = vol->grp[i].inode_table;
        int dt = sb->blocks_per_group - bb - ib - it;
        kprintf(-1, "Group %d block bitmap, %d inode bitmap, %d inodes, %d data\n", ib, bb, it, dt);
    }

    inode_t *ino = ext2_inode(NULL, 2); // TODO volume !?
    if (ino == NULL) {
        kunmap(ptr, PAGE_SIZE);
        errno = EBADF;
        return NULL;
    }

    ino->und.vol->volname = sb->volume_name;
    ino->und.vol->volfs = "ext2";
    ino->und.vol->ops = &ext2_fs_ops;
    errno = 0;
    return ino;
}
