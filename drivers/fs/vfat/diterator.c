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
#include "vfat.h"
#include <assert.h>

void fat_create_iterator(inode_t *dir, fat_iterator_t *ctx)
{
    assert(dir != NULL && dir->type == FL_DIR && dir->lba != 0);
    fat_volume_t *volume = dir->drv_data;
    ctx->cluster = NULL;
    ctx->index = 0;
    ctx->dev = dir->dev->underlying;
    ctx->sec_size = volume->BytsPerSec;
    if (dir->no == 1) {
        int dL = (volume->RootEntry * volume->BytsPerSec) / PAGE_SIZE;
        int dO = (volume->RootEntry * volume->BytsPerSec) % PAGE_SIZE;
        int eL = ALIGN_UP((volume->RootEntry + volume->RootDirSectors) * volume->BytsPerSec, PAGE_SIZE) / PAGE_SIZE;
        int sL = eL - dL;
        ctx->lba = volume->RootEntry;
        ctx->map_size = sL * PAGE_SIZE;
        ctx->icount = volume->RootDirSectors * 16;
    } else {
        int lba = FAT_CLUSTER_TO_LBA(volume, dir->lba);
        ctx->lba = lba;
        int fL = ((volume->FirstDataSector) * volume->BytsPerSec) / PAGE_SIZE;
        //int dL = (lba * volume->BytsPerSec) / PAGE_SIZE;
        //int dO = (lba * volume->BytsPerSec) % PAGE_SIZE;
        int eL = ALIGN_UP((volume->FirstDataSector + volume->SecPerClus) * volume->BytsPerSec, PAGE_SIZE) / PAGE_SIZE;
        int sL = eL - fL;
        ctx->map_size = sL * PAGE_SIZE;
        ctx->icount = volume->SecPerClus * 16;
    }
}

void fat_clear_iterator(fat_iterator_t *ctx)
{
    if (ctx->cluster == NULL)
        return;
    kunmap(ctx->cluster, ctx->map_size);
    ctx->cluster = NULL;
}

int fat_iterator_next(fat_iterator_t *ctx, fat_entry_t **ptr)
{
    for (;;) {
        if (ctx->cluster == NULL) {
            int page = (ctx->lba * ctx->sec_size) / PAGE_SIZE;
            int off = (ctx->lba * ctx->sec_size) % PAGE_SIZE;
            ctx->cluster = kmap(ctx->map_size, ctx->dev, page * PAGE_SIZE, VM_RD);
            ctx->tables = (fat_entry_t *)(ctx->cluster + off);
            ctx->index = 0;
        }

        while (ctx->index < ctx->icount) {
            fat_entry_t *entry = &ctx->tables[++ctx->index];

            if (entry->DIR_Attr == 0)
                return 0;
            if (entry->DIR_Attr == ATTR_VOLUME_ID || entry->DIR_Attr == ATTR_DELETED)
                continue;
            if ((entry->DIR_Attr & ATTR_LONG_NAME_MASK) == ATTR_LONG_NAME) {
                // TODO - Prepare long name
                continue;
            }
            if (memcmp(entry->DIR_Name, FAT_DIRNAME_CURRENT, 11) == 0)
                continue;
            if (memcmp(entry->DIR_Name, FAT_DIRNAME_PARENT, 11) == 0)
                continue;
            *ptr = entry;
            return ctx->lba * (ctx->sec_size / 32) + ctx->index - 1;
        }

        // TODO -- Got next cluster...
    }
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

inode_t* fat_lookup(inode_t* dir, const char* name, void* acl)
{
    inode_t* ino;
    fat_iterator_t ctx;
    fat_create_iterator(dir, &ctx);
    fat_volume_t* volume = dir->drv_data;

    for (;;) {
        fat_entry_t* entry;
        int no = fat_iterator_next(&ctx, &entry);
        if (no == 0)
            break;

        char shortname[14];
        fatfs_read_shortname(entry, shortname);
        if (strcmp(name, shortname) != 0) // TODO - long name
            continue;

        ino = fatfs_inode(no, entry, dir->dev, volume);
        fat_clear_iterator(&ctx);
        errno = 0;
        return ino;
    }

    fat_clear_iterator(&ctx);
    errno = ENOENT;
    return NULL;
}

//inode_t* fat_create(inode_t* dir, const char* name, void* acl, int mode)
//{
//
//}

inode_t *fat_open(inode_t *dir, const char *name, ftype_t type, void *acl, int flags)
{
    inode_t *ino;
    fat_iterator_t ctx;
    fat_create_iterator(dir, &ctx);
    fat_volume_t *volume = dir->drv_data;

    for (;;) {
        fat_entry_t *entry;
        int no = fat_iterator_next(&ctx, &entry);
        if (no == 0)
            break;

        char shortname[14];
        fatfs_read_shortname(entry, shortname);
        if (strcmp(name, shortname) != 0) // TODO - long name
            continue;

        // The file exist
        if (!(flags & IO_OPEN)) {
            errno = EEXIST;
            return NULL;
        }

        ino = fatfs_inode(no, entry, dir->dev, volume);
        fat_clear_iterator(&ctx);
        errno = 0;
        return ino;
    }

    if (!(flags & IO_CREAT)) {
        fat_clear_iterator(&ctx);
        errno = ENOENT;
        return NULL;
    }

    //// Create a new entry at the end
    //entry = it->entry;
    //if (entry == NULL) {
    //    // TODO - alloc cluster
    //}
    //int alloc_lba = type == FL_DIR ? fatfs_mkdir(info, dir) : 0;

    //fatfs_short_entry(entry, alloc_lba, type);
    //fatfs_write_shortname(entry, name);
    //memset(&entry[1], 0, sizeof(*entry)); // TODO - not behind cluster

    //ino = fatfs_inode(it->lba * entries_per_cluster + it->idx, entry, dir->dev, info);
    //fatfs_diterator_close(it);
    //// bio_sync(info->io_head);
    //errno = 0;
    //return ino;
    fat_clear_iterator(&ctx);
    return NULL;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void *fat_opendir(inode_t *dir)
{
    fat_iterator_t *ctx = kalloc(sizeof(fat_iterator_t));
    fat_create_iterator(dir, ctx);
    return ctx;
}

inode_t *fat_readdir(inode_t *dir, char *name, void *ptr)
{
    fat_iterator_t *ctx = ptr;
    struct FAT_ShortEntry *entry;
    int no = fat_iterator_next(ctx, &entry);
    if (no == 0)
        return NULL;

    fat_volume_t *volume = dir->drv_data;
    fatfs_read_shortname(entry, name);
    return fatfs_inode(no, entry, dir->dev, volume);
}

int fat_closedir(inode_t *dir, void *ptr)
{
    fat_iterator_t *ctx = ptr;
    fat_clear_iterator(ctx);
    kfree(ctx);
    return 0;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int fat_unlink(inode_t *dir, const char *name)
{
    fat_iterator_t ctx;
    fat_create_iterator(dir, &ctx); // TODO - write access
    fat_volume_t *volume = dir->drv_data;

    for (;;) {
        fat_entry_t *entry;
        int no = fat_iterator_next(&ctx, &entry);
        if (no == 0)
            break;

        char shortname[14];
        fatfs_read_shortname(entry, shortname);
        if (strcmp(name, shortname) != 0) // TODO - long name
            continue;

        if (entry->DIR_Attr & ATTR_DIRECTORY) {
            // TODO - Check directory is empty
            // fat_iterator_t ctx2;
            // inode_t *sdir = fatfs_inode(no, entry, dir->dev, volume);
            // fat_create_iterator(sdir, &ctx2);
            // fat_iterator_next(&ctx2) == 0
            // fat_clear_iterator(&ctx2);
            // vfs_close_inode(sdir);
            fat_clear_iterator(&ctx);
            errno = ENOTEMPTY;
            return -1;
        }

        // TODO - Remove allocated cluster
        entry->DIR_Attr = ATTR_DELETED;
        fat_clear_iterator(&ctx);
    }

    fat_clear_iterator(&ctx);
    errno = ENOENT;
    return -1;
}
