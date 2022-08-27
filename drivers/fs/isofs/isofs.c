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
#include "isofs.h"
#include <kernel/mods.h>
#include <time.h>

inode_t *isofs_mount(inode_t *dev, const char *options);
// int isofs_umount(inode_t* ino);

inode_t *isofs_lookup(inode_t *dir, const char *name, void *acl);
int isofs_read(inode_t *ino, void *buffer, size_t length, xoff_t offset);

xoff_t *isofs_opendir(inode_t *dir);
inode_t *isofs_readdir(inode_t *dir, char *name, xoff_t *ctx);
int isofs_closedir(inode_t *dir, xoff_t *ctx);

void isofs_close(inode_t *ino);

ino_ops_t iso_reg_ops = {
    .read = (void *)isofs_read,
    .close = isofs_close,
};

ino_ops_t iso_dir_ops = {
    .lookup = isofs_lookup,
    .opendir = (void *)isofs_opendir,
    .readdir = (void *)isofs_readdir,
    .closedir = (void *)isofs_closedir,
    .close = isofs_close,
};


/* Copy the filename read from a directory entry */
static void isofs_filename(ISOFS_entry_t *entry, char *name)
{
    memcpy(name, entry->fileId, entry->lengthFileId);
    name[(int)entry->lengthFileId] = '\0';
    if (entry->lengthFileId - 2 > 0 && entry->lengthFileId - 2 < 256 && name[entry->lengthFileId - 2 ] == ';') {
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

typedef struct ISO_info iso_volume_t;

/* */
static inode_t *isofs_inode(device_t *volume, iso_volume_t *vol, ISOFS_entry_t *entry)
{
    ftype_t type = entry->fileFlag & 2 ? FL_DIR : FL_REG;
    ino_ops_t *ops = type == FL_REG ? &iso_reg_ops : &iso_dir_ops;
    inode_t *ino = vfs_inode(entry->locExtendLE, type, volume, ops);
    if (ino->rcu == 1)
        atomic_inc(&vol->rcu);
    ino->length = entry->dataLengthLE;
    ino->lba = entry->locExtendLE;
    ino->mode = entry->fileFlag & 2 ? 0755 : 0644;
    struct tm tm;
    memset(&tm, 0, sizeof(struct tm));
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
    ino->drv_data = vol;
    return ino;
}

void isofs_close(inode_t *ino)
{
    iso_volume_t *vol = ino->drv_data;
    if (atomic_xadd(&vol->rcu, -1) == 1) {
        vfs_close_inode(ino->dev->underlying);
        kfree(vol);
    }
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


xoff_t *isofs_opendir(inode_t *dir)
{
    xoff_t *poffset = kalloc(sizeof(xoff_t));
    errno = 0;
    return poffset;
}

int isofs_closedir(inode_t *dir, xoff_t *poffset)
{
    kfree(poffset);
    errno = 0;
    return 0;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


inode_t *isofs_lookup(inode_t *dir, const char *name, void *acl)
{
    char *filename = kalloc(256);
    xoff_t offset = 0;
    struct bkmap bk;
    void *ptr = NULL;

    while (offset < dir->length) {
        if (ptr)
            bkunmap(&bk);
        ptr = bkmap(&bk, dir->lba + offset / ISOFS_SECTOR_SIZE, ISOFS_SECTOR_SIZE, 0, dir->dev->underlying, VM_RD);
        ISOFS_entry_t *entry = ADDR_OFF(ptr, offset % ISOFS_SECTOR_SIZE);
        if (entry->lengthRecord == 0) {
            offset = ALIGN_UP(offset + 1, ISOFS_SECTOR_SIZE);
            continue;
        }

        isofs_filename(entry, filename);
        if (strcmp(name, filename) == 0) {
            inode_t *ino = isofs_inode(dir->dev, dir->drv_data, entry);
            bkunmap(&bk);
            kfree(filename);
            errno = 0;
            return ino;
        }

        offset += entry->lengthRecord;
    }

    kfree(filename);
    if (ptr)
        bkunmap(&bk);
    errno = ENOENT;
    return NULL;
}

inode_t *isofs_readdir(inode_t *dir, char *name, xoff_t *poffset)
{
    // struct ISO_info *info = (struct ISO_info *)dir->drv_data;
    xoff_t offset = *poffset;
    struct bkmap bk;
    void *ptr = NULL;
    while (offset < dir->length) {
        if (ptr)
            bkunmap(&bk);
        ptr = bkmap(&bk, dir->lba + offset / ISOFS_SECTOR_SIZE, ISOFS_SECTOR_SIZE, 0, dir->dev->underlying, VM_RD);
        ISOFS_entry_t *entry = ADDR_OFF(ptr, offset % ISOFS_SECTOR_SIZE);
        if (entry->lengthRecord == 0) {
            offset = ALIGN_UP(offset + 1, ISOFS_SECTOR_SIZE);
            continue;
        }

        isofs_filename(entry, name);
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0 || name[0] < 0x20) {
            offset += entry->lengthRecord;
            continue;
        }

        offset += entry->lengthRecord;
        inode_t *ino = isofs_inode(dir->dev, dir->drv_data, entry);
        bkunmap(&bk);
        errno = 0;
        *poffset = offset;
        return ino;
    }

    if (ptr)
        bkunmap(&bk);
    errno = ENOENT;
    *poffset = offset;
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
    struct ISO_info *info = NULL;
    if (dev == NULL || (dev->type != FL_BLK && dev->type != FL_REG)) {
        errno = ENODEV;
        return NULL;
    }

    // Read volume descriptors
    for (int lba = 16; ; lba++) {
        struct bkmap bk;
        ISOFS_descriptor_t *descriptor = bkmap(&bk, lba, ISOFS_SECTOR_SIZE, 0, dev, VM_RD);
        if ((descriptor->magicInt[0] & 0xFFFFFF00) != ISOFS_STD_ID1 ||
            (descriptor->magicInt[1] & 0x0000FFFF) != ISOFS_STD_ID2 ||
            (descriptor->type != ISOFS_VOLDESC_PRIM && !info)) {
            if (info)
                kfree(info);

            kprintf(-1, "isofs] Not a volume descriptor at lba %d\n", lba);
            bkunmap(&bk);
            errno = EBADF;
            return NULL;
        }

        // Primary descriptor
        if (descriptor->type == ISOFS_VOLDESC_PRIM) {
            if (info != NULL) {
                kfree(info);
                bkunmap(&bk);
                errno = EBADF;
                return NULL;
            }
            info = (struct ISO_info *)kalloc(sizeof(struct ISO_info));
            info->bootable = false;
            info->created = 0;
            info->sectorSize = descriptor->logicBlockSizeLE;
            info->sectorCount = descriptor->volSpaceSizeLE;
            info->lbaroot = descriptor->rootDir.locExtendLE;
            info->lgthroot = descriptor->rootDir.dataLengthLE;
            memcpy(info->name, descriptor->volname, 32);
            info->name[32] = '\0';
            int i = 31;
            while (i >= 0 && info->name[i] == ' ')
                info->name[i--] = '\0';

        // Boot descriptor
        } else if (descriptor->type == ISOFS_VOLDESC_BOOT) {
            if (info)
                info->bootable = true;

        // Terminal descriptor
        } else if (descriptor->type == ISOFS_VOLDESC_TERM) {
            bkunmap(&bk);
            break;

        // Unknown descriptor
        } else {
            kprintf(-1, "isofs] Unknown volume descriptor id %d\n", descriptor->type);
        }

        bkunmap(&bk);
    }

    inode_t *ino = vfs_inode(info->lbaroot, FL_DIR, NULL, &iso_dir_ops);
    ino->length = info->lgthroot;
    ino->lba = info->lbaroot;
    ino->mode = 0755;
    ino->dev->flags = FD_RDONLY;
    ino->dev->devclass = kstrdup("isofs");
    ino->dev->devname = kstrdup(info->name);
    ino->dev->underlying = vfs_open_inode(dev);
    ino->drv_data = info;
    info->rcu = 1;
    return ino;
}

//int isofs_umount(inode_t *ino)
//{
//    return 0;
//}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


void isofs_setup()
{
    vfs_addfs("isofs", isofs_mount, NULL);
    vfs_addfs("iso", isofs_mount, NULL);
}

void isofs_teardown()
{
    vfs_rmfs("isofs");
    vfs_rmfs("iso");
}

EXPORT_MODULE(isofs, isofs_setup, isofs_teardown);
