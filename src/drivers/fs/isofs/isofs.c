/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2018  <Fabien Bavent>
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
#include "isofs.h"
#include <kernel/files.h>

#define ISOFS_SECTOR_SIZE  2048

int isofs_close(inode_t *ino);
ISO_dirctx_t *isofs_opendir(inode_t *dir);
int isofs_closedir(inode_t *dir, ISO_dirctx_t *ctx);
inode_t *isofs_open(inode_t *dir, CSTR name, ftype_t type, acl_t *acl, int flags);
inode_t *isofs_readdir(inode_t *dir, char *name, ISO_dirctx_t *ctx);
int isofs_read(inode_t *ino, void *buffer, size_t length, off_t offset);
page_t isofs_fetch(inode_t *ino, off_t off);
void isofs_release(inode_t *ino, off_t off, page_t pg);



fs_ops_t isofs_ops = {
    .open = isofs_open,
};

ino_ops_t iso_reg_ops = {
    .close = isofs_close,
    .fetch = isofs_fetch,
    .release = isofs_release,
};

ino_ops_t iso_dir_ops = {
    .close = isofs_close,
    .opendir = (void *)isofs_opendir,
    .readdir = (void *)isofs_readdir,
    .closedir = (void *)isofs_closedir,
};



/* Copy the filename read from a directory entry */
static void isofs_filename(ISOFS_entry_t *entry, char *name)
{
    memcpy(name, entry->fileId, entry->lengthFileId);
    name[(int)entry->lengthFileId] = '\0';
    if (name[entry->lengthFileId - 2 ] == ';') {
        if (name[entry->lengthFileId - 3] == '.')
            name[entry->lengthFileId - 3] = '\0';

        else
            name[entry->lengthFileId - 2] = '\0';
    }
}

/* */
static inode_t *isofs_inode(volume_t *volume, ISOFS_entry_t *entry)
{
    ftype_t type = entry->fileFlag & 2 ? FL_DIR : FL_REG;
    inode_t *ino = vfs_inode(entry->locExtendLE, type, volume);
    ino->length = entry->dataLengthLE;
    ino->lba = entry->locExtendLE;
    struct tm tm;
    tm.tm_year = entry->recordYear;
    tm.tm_mon = entry->recordMonth;
    tm.tm_mday = entry->recordDay;
    tm.tm_hour = entry->recordHour;
    tm.tm_min = entry->recordMinute;
    tm.tm_sec = entry->recordSecond;
    ino->ctime = mktime(&tm) * _PwNano_;
    ino->btime = ino->ctime;
    ino->atime = ino->ctime;
    ino->mtime = ino->ctime;
    if (type == FL_REG) {
        ino->info = map_create(ino, isofs_read, NULL);
        ino->ops = &iso_reg_ops;
    } else
        ino->ops = &iso_dir_ops;
    return ino;
}


int isofs_close(inode_t *ino)
{
    if (ino->type == FL_REG)
        map_destroy(ino->info);
    else if (ino->type == FL_VOL) {
        kfree(ino->und.vol->volname);
        kfree(ino->und.vol);
    }
    return 0;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

ISO_dirctx_t *isofs_opendir(inode_t *dir)
{
    ISO_dirctx_t *ctx = (ISO_dirctx_t *)kalloc(sizeof(ISO_dirctx_t));
    errno = 0;
    return ctx;
}

int isofs_closedir(inode_t *dir, ISO_dirctx_t *ctx)
{
    struct ISO_info *info = (struct ISO_info *)dir->und.vol->info;
    if (ctx->base != NULL)
        bio_clean(info->io, ctx->lba);
    // kunmap(ctx->base, 8192);
    kfree(ctx);
    errno = 0;
    return 0;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


inode_t *isofs_open(inode_t *dir, CSTR name, ftype_t type, acl_t *acl, int flags)
{
    if (!(flags & VFS_OPEN)) {
        errno = EROFS;
        return NULL;
    }

    int lba = dir->lba;
    struct ISO_info *info = (struct ISO_info *)dir->und.vol->info;
    uint8_t *address = bio_access(info->io, lba);
    // kmap(8192, dir->und.vol->dev, lba * ISOFS_SECTOR_SIZE, VMA_FILE_RO);

    /* Skip the first two entries */
    ISOFS_entry_t *entry = (ISOFS_entry_t *)address;
    entry = ISOFS_nextEntry(entry); /* Directory '.' */
    entry = ISOFS_nextEntry(entry); /* Directory '..' */

    char *filename = (char *)kalloc(FILENAME_MAX);
    while (entry->lengthRecord) {

        /* Copy and fix filename */
        isofs_filename(entry, filename);

        /* Compare filenames */
        if (strcmp(name, filename) == 0) {
            inode_t *ino = isofs_inode(dir->und.vol, entry);
            bio_clean(info->io, lba);
            // kunmap(address, 8192);
            kfree(filename);
            errno = 0;
            return ino;
        }

        /* Move pointer to next entry, eventualy continue directory mapping. */
        entry = ISOFS_nextEntry(entry);
        bool remap = (size_t)entry >= (size_t)address + PAGE_SIZE;
        if (!remap)
            remap = (size_t)entry + entry->lengthRecord > (size_t)address + 2 * ISOFS_SECTOR_SIZE;

        if (remap) {
            size_t off = (size_t)entry - (size_t)address - ISOFS_SECTOR_SIZE;
            bio_clean(info->io, lba);
            lba += 2;
            // kunmap(address, 8192);
            address = bio_access(info->io, lba);
            // address = kmap(8192, dir->und.vol->dev, lba * ISOFS_SECTOR_SIZE, VMA_FILE_RO);
            entry = (ISOFS_entry_t *)((size_t)address + off);
        }
    }

    bio_clean(info->io, lba);
    // kunmap(address, 8192);
    kfree(filename);

    if (flags & VFS_CREAT) {
        errno = EROFS;
        return NULL;
    }

    errno = ENOENT;
    return NULL;
}

inode_t *isofs_readdir(inode_t *dir, char *name, ISO_dirctx_t *ctx)
{
    struct ISO_info *info = (struct ISO_info *)dir->und.vol->info;
    if (ctx->lba == 0)
        ctx->lba = dir->lba;
    if (ctx->base == NULL)
        ctx->base = bio_access(info->io, ctx->lba);
    // ctx->base = kmap(8192, dir->und.vol->dev, ctx->lba * ISOFS_SECTOR_SIZE, VMA_FILE_RO); // TODO - NOT ALIGNED !!

    /* Skip the first two entries */
    ISOFS_entry_t *entry = (ISOFS_entry_t *)(&ctx->base[ctx->off]);
    while (entry->lengthRecord) {
        isofs_filename(entry, name);
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0 || name[0] < 0x20) {
            entry = ISOFS_nextEntry(entry);
            continue;
        }
        inode_t *ino = isofs_inode(dir->und.vol, entry);

        /* Move pointer to next entry, eventualy continue directory mapping. */
        entry = ISOFS_nextEntry(entry);
        bool remap = (size_t)entry >= (size_t)ctx->base + 2 * ISOFS_SECTOR_SIZE;
        if (!remap)
            remap = (size_t)entry + entry->lengthRecord > (size_t)ctx->base + 2 * ISOFS_SECTOR_SIZE;

        ctx->off = (size_t)entry - (size_t)ctx->base;
        if (remap) {
            ctx->off -= ISOFS_SECTOR_SIZE;
            bio_clean(info->io, ctx->lba);
            // kunmap(ctx->base, 8192);
            ctx->lba ++;
            ctx->base = NULL;
        }

        errno = 0;
        return ino;
    }

    errno = 0;
    return NULL;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int isofs_read(inode_t *ino, void *buffer, size_t length, off_t offset)
{
    return vfs_read(ino->und.vol->dev, buffer, length, ino->lba * ISOFS_SECTOR_SIZE + offset, 0);
}

page_t isofs_fetch(inode_t *ino, off_t off)
{
    return map_fetch(ino->info, off);
}

void isofs_release(inode_t *ino, off_t off, page_t pg)
{
    map_release(ino->info, off, pg);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

inode_t *isofs_mount(inode_t *dev)
{
    int i;
    int lba = 16;
    struct ISO_info *info = NULL;
    if (dev == NULL) {
        errno = ENODEV;
        return NULL;
    }

    uint8_t *address = (uint8_t *)kmap(8192, dev, lba * ISOFS_SECTOR_SIZE, VMA_FILE_RO);
    int addressLba = 16;
    int addressCnt = 4;
    for (lba = 16; ; ++lba) {

        if ((lba - addressLba) >= addressCnt) {
            kunmap(address, addressCnt * ISOFS_SECTOR_SIZE);
            addressLba = lba;
            address = (uint8_t *)kmap(addressCnt * ISOFS_SECTOR_SIZE, dev, addressLba * ISOFS_SECTOR_SIZE, VMA_FILE_RO);
        }

        ISOFS_descriptor_t *descriptor = (ISOFS_descriptor_t *)&address[(lba - addressLba) * ISOFS_SECTOR_SIZE];
        if ((descriptor->magicInt[0] & 0xFFFFFF00) != ISOFS_STD_ID1 ||
            (descriptor->magicInt[1] & 0x0000FFFF) != ISOFS_STD_ID2 ||
            (descriptor->type != ISOFS_VOLDESC_PRIM && !info)) {
            if (info)
                kfree(info);

            kprintf(KLOG_DBG, " isofs -- Not a volume descriptor at lba %d\n", lba);
            kunmap(address, addressCnt * ISOFS_SECTOR_SIZE);
            errno = EBADF;
            return NULL;
        }

        if (descriptor->type == ISOFS_VOLDESC_PRIM) {
            info = (struct ISO_info *)kalloc(sizeof(struct ISO_info));
            info->bootable = 0;
            info->created = 0;
            info->sectorSize = descriptor->logicBlockSizeLE;
            info->sectorCount = descriptor->volSpaceSizeLE;
            info->lbaroot = descriptor->rootDir.locExtendLE;
            info->lgthroot = descriptor->rootDir.dataLengthLE;
            info->io = bio_create2(dev, VMA_FILE_RO, 2048, 0, 1);
            // descriptor->applicationId

            memcpy(info->name, descriptor->volname, 32);
            info->name[32] = '\0';
            i = 31;
            while (i >= 0 && info->name[i] == ' ')
                info->name[i--] = '\0';

        } else if (descriptor->type == ISOFS_VOLDESC_BOOT)
            info->bootable = !0;

        else if (descriptor->type == ISOFS_VOLDESC_TERM)
            break;

        else {
            if (info)
                kfree(info);
            kprintf(KLOG_DBG, " isofs -- Bad volume descriptor id %d\n", descriptor->type);
            kunmap(address, addressCnt * ISOFS_SECTOR_SIZE);
            errno = EBADF;
            return NULL;
        }
    }
    kunmap(address, addressCnt * ISOFS_SECTOR_SIZE);


    inode_t *ino = vfs_inode(info->lbaroot, FL_VOL, NULL);
    ino->length = info->lgthroot;
    ino->lba = info->lbaroot;
    ino->und.vol->flags = VFS_RDONLY;
    ino->und.vol->volfs = "isofs";
    ino->und.vol->volname = strdup(info->name);
    ino->und.vol->dev = vfs_open(dev);
    ino->und.vol->info = info;
    ino->und.vol->ops = &isofs_ops;
    ino->ops = &iso_dir_ops;
    return ino;
}

int isofs_umount(inode_t *ino)
{
    return 0;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


void isofs_setup()
{
    register_fs("isofs", (fs_mount)isofs_mount);
}

void isofs_teardown()
{
    unregister_fs("isofs");
}

MODULE(isofs, isofs_setup, isofs_teardown);

