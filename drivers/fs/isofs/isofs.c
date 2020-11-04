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
#include "isofs.h"
#include <kernel/mods.h>

inode_t *isofs_mount(inode_t *dev, const char *options);
// int isofs_umount(inode_t* ino);

inode_t *isofs_open(inode_t *dir, const char *name, ftype_t type, void *acl, int flags);
int isofs_read(inode_t *ino, void *buffer, size_t length, xoff_t offset);

ISO_dirctx_t *isofs_opendir(inode_t *dir);
inode_t *isofs_readdir(inode_t *dir, char *name, ISO_dirctx_t *ctx);
int isofs_closedir(inode_t *dir, ISO_dirctx_t *ctx);


ino_ops_t iso_reg_ops = {
    .read = isofs_read,
};

ino_ops_t iso_dir_ops = {
    .open = isofs_open,
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

    // Freaky wierd filename read, retro-enginered, not described on specs.
    ISOFS_entry_extra_t *ex = (void *)ALIGN_UP((size_t)entry->fileId + entry->lengthFileId, 2);
    if (ex->pm == 0x5850) {
        int mlen = entry->lengthRecord - sizeof(ISOFS_entry_t) + 1 - entry->lengthFileId - sizeof(ISOFS_entry_extra_t) + 1;
        if (mlen > 0) {
            if ((entry->lengthFileId % 2) == 0)
                mlen--;
            memcpy(name, ex->filename, mlen);
            name[mlen] = '\0';
        }
    }
}

/* */
static inode_t *isofs_inode(device_t *volume, ISOFS_entry_t *entry)
{
    ftype_t type = entry->fileFlag & 2 ? FL_DIR : FL_REG;
    ino_ops_t *ops = type == FL_REG ? &iso_reg_ops : &iso_dir_ops;
    inode_t *ino = vfs_inode(entry->locExtendLE, type, volume, ops);
    ino->length = entry->dataLengthLE;
    ino->lba = entry->locExtendLE;
    struct tm tm;
    tm.tm_year = entry->recordYear;
    tm.tm_mon = entry->recordMonth;
    tm.tm_mday = entry->recordDay;
    tm.tm_hour = entry->recordHour;
    tm.tm_min = entry->recordMinute;
    tm.tm_sec = entry->recordSecond;
    ino->ctime = SEC_TO_USEC(mktime(&tm));
    ino->btime = ino->ctime;
    ino->atime = ino->ctime;
    ino->mtime = ino->ctime;
    return ino;
}


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

static void iso_unmap(ISO_dirctx_t *ctx)
{
    if (ctx->map_ptr != NULL) {
        kunmap(ctx->map_ptr, PAGE_SIZE);
        ctx->map_ptr = NULL;
    }
}

static void iso_remap(device_t *dev, ISO_dirctx_t *ctx, int lba)
{
    int mlba = ALIGN_DW(lba * ISOFS_SECTOR_SIZE, PAGE_SIZE) / ISOFS_SECTOR_SIZE;
    if (mlba != ctx->map_lba) {
        iso_unmap(ctx);
        ctx->map_lba = mlba;
    }
    if (ctx->map_ptr == NULL)
        ctx->map_ptr = kmap(PAGE_SIZE, dev->underlying, (xoff_t)ctx->map_lba * ISOFS_SECTOR_SIZE, VM_RD);
    ctx->lba = lba;
    ctx->base = ctx->map_ptr + (ctx->lba - ctx->map_lba) * ISOFS_SECTOR_SIZE;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


ISO_dirctx_t *isofs_opendir(inode_t *dir)
{
    ISO_dirctx_t *ctx = (ISO_dirctx_t *)kalloc(sizeof(ISO_dirctx_t));
    ctx->lba = dir->lba;
    errno = 0;
    return ctx;
}

int isofs_closedir(inode_t *dir, ISO_dirctx_t *ctx)
{
    // struct ISO_info *info = (struct ISO_info *)dir->drv_data;
    iso_unmap(ctx);
    kfree(ctx);
    errno = 0;
    return 0;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


inode_t *isofs_open(inode_t *dir, const char *name, ftype_t type, void *acl, int flags)
{
    if (!(flags & IO_OPEN)) {
        errno = EROFS;
        return NULL;
    }

    char *filename = kalloc(256);
    size_t maxlba = dir->lba + dir->length / ISOFS_SECTOR_SIZE;
    // struct ISO_info *info = (struct ISO_info *)dir->drv_data;

    ISO_dirctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.lba = dir->lba;
    while (ctx.lba < maxlba) {
        iso_remap(dir->dev, &ctx, ctx.lba);
        ISOFS_entry_t *entry = (ISOFS_entry_t *)(&ctx.base[ctx.off]);
        while (entry->lengthRecord) {
            isofs_filename(entry, filename);

            if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0 || filename[0] < 0x20) {
                entry = ISOFS_nextEntry(entry);
                ctx.off = (size_t)entry - (size_t)ctx.base;
                continue;
            }

            /* Compare filenames */
            if (strcmp(name, filename) == 0) {
                inode_t *ino = isofs_inode(dir->dev, entry);
                iso_unmap(&ctx);
                kfree(filename);
                errno = 0;
                return ino;
            }

            /* Move pointer to next entry, eventualy continue directory mapping. */
            entry = ISOFS_nextEntry(entry);
            ctx.off = (size_t)entry - (size_t)ctx.base;
            if (ctx.off > ISOFS_SECTOR_SIZE)
                break;
        }

        ctx.off = 0;
        ctx.lba++;
    }

    kfree(filename);
    if (flags & IO_CREAT) {
        errno = EROFS;
        return NULL;
    }

    errno = ENOENT;
    return NULL;
}

inode_t *isofs_readdir(inode_t *dir, char *name, ISO_dirctx_t *ctx)
{
    // struct ISO_info *info = (struct ISO_info *)dir->drv_data;
    size_t maxlba = dir->lba + dir->length / ISOFS_SECTOR_SIZE;

    while (ctx->lba < maxlba) {
        iso_remap(dir->dev, ctx, ctx->lba);
        ISOFS_entry_t *entry = (ISOFS_entry_t *)(&ctx->base[ctx->off]);
        while (entry->lengthRecord) {
            isofs_filename(entry, name);

            if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0 || name[0] < 0x20) {
                entry = ISOFS_nextEntry(entry);
                ctx->off += (size_t)entry - (size_t)ctx->base;
                continue;
            }
            inode_t *ino = isofs_inode(dir->dev, entry);

            /* Move pointer to next entry, eventualy continue directory mapping. */
            entry = ISOFS_nextEntry(entry);
            ctx->off = (size_t)entry - (size_t)ctx->base;
            if (ctx->off > ISOFS_SECTOR_SIZE) {
                ctx->off = 0;
                ctx->lba++;
            }

            errno = 0;
            return ino;
        }

        ctx->off = 0;
        ctx->lba++;
    }

    errno = 0;
    return NULL;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int isofs_read(inode_t *ino, void *buffer, size_t length, xoff_t offset)
{
    int ret = vfs_read(ino->dev->underlying, buffer, length, ino->lba * ISOFS_SECTOR_SIZE + offset, 0);
    return (size_t)ret == length ? 0 : -1;
}


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

inode_t *isofs_mount(inode_t *dev, const char *options)
{
    int i;
    int lba = 16;
    struct ISO_info *info = NULL;
    if (dev == NULL) {
        errno = ENODEV;
        return NULL;
    }

    uint8_t *address = (uint8_t *)kmap(8192, dev, lba * ISOFS_SECTOR_SIZE, VM_RD);
    int addressLba = 16;
    int addressCnt = 4;
    for (lba = 16; ; ++lba) {

        if ((lba - addressLba) >= addressCnt) {
            kunmap(address, addressCnt * ISOFS_SECTOR_SIZE);
            addressLba = lba;
            address = (uint8_t *)kmap(addressCnt * ISOFS_SECTOR_SIZE, dev, addressLba * ISOFS_SECTOR_SIZE, VM_RD);
        }

        ISOFS_descriptor_t *descriptor = (ISOFS_descriptor_t *)&address[(lba - addressLba) * ISOFS_SECTOR_SIZE];
        if ((descriptor->magicInt[0] & 0xFFFFFF00) != ISOFS_STD_ID1 ||
            (descriptor->magicInt[1] & 0x0000FFFF) != ISOFS_STD_ID2 ||
            (descriptor->type != ISOFS_VOLDESC_PRIM && !info)) {
            if (info)
                kfree(info);

            kprintf(KL_DBG, " isofs -- Not a volume descriptor at lba %d\n", lba);
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
            kprintf(KL_DBG, " isofs -- Bad volume descriptor id %d\n", descriptor->type);
            kunmap(address, addressCnt * ISOFS_SECTOR_SIZE);
            errno = EBADF;
            return NULL;
        }
    }
    kunmap(address, addressCnt * ISOFS_SECTOR_SIZE);

    inode_t *ino = vfs_inode(info->lbaroot, FL_DIR, NULL, &iso_dir_ops);
    ino->length = info->lgthroot;
    ino->lba = info->lbaroot;
    ino->dev->flags = FD_RDONLY;
    ino->dev->devclass = "isofs";
    ino->dev->devname = strdup(info->name);
    ino->dev->underlying = vfs_open_inode(dev);
    ino->drv_data = info;
    return ino;
}

//int isofs_umount(inode_t *ino)
//{
//    return 0;
//}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


void isofs_setup()
{
    vfs_addfs("isofs", isofs_mount);
    vfs_addfs("iso", isofs_mount);
}

void isofs_teardown()
{
    vfs_rmfs("isofs");
    vfs_rmfs("iso");
}

EXPORT_MODULE(isofs, isofs_setup, isofs_teardown);

