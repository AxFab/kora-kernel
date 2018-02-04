/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015  <Fabien Bavent>
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
#include <kernel/vfs.h>
#include <kernel/sys/inode.h>
#include <kernel/sys/device.h>
#include <kora/bbtree.h>
#include <string.h>
#include <errno.h>

#define FILENAME_MAX 255

/* Identificators for volume descriptors */
#define ISO9660_STD_ID1    0x30444300
#define ISO9660_STD_ID2    0x00003130
#define ISO9660_VOLDESC_BOOT 0
#define ISO9660_VOLDESC_PRIM 1
#define ISO9660_VOLDESC_SUP  2
#define ISO9660_VOLDESC_VOL  3
#define ISO9660_VOLDESC_TERM 255

typedef struct ISO9660_entry ISO9660_entry_t;
typedef struct ISO9660_descriptor ISO9660_descriptor_t;

#define ISO9660_nextEntry(e)  ((ISO9660_entry_t*)&(((char*)(e))[(e)->lengthRecord]));

/* Structure of a Directory entry in Iso9660 FS */
PACK(struct ISO9660_entry {
    uint8_t lengthRecord;
    char extLengthRecord;
    int locExtendLE;
    int locExtendBE;
    int dataLengthLE;
    int dataLengthBE;
    char recordYear;
    char recordMonth;
    char recordDay;
    char recordHour;
    char recordMinute;
    char recordSecond;
    char recordTimelag;
    char fileFlag;
    char fileUnitSize;
    char gapSize;
    short volSeqNumLE;
    short volSeqNumBE;
    char lengthFileId;
    char fileId[1];
});


/* Structure of the Primary volume descriptor in Iso9660 FS */
PACK(struct ISO9660_descriptor {
    union {
        struct {
            unsigned char type;
            char magic[7];
        };
        int magicInt[2];
    };
    char sysname[32];
    char volname[32];
    char unused1[8];
    int volSpaceSizeLE;
    int volSpaceSizeBE;
    char unused2[32];
    short volSetSizeLE;
    short volSetSizeBE;
    short volSeqNumberLE;
    short volSeqNumberBE;
    short logicBlockSizeLE;
    short logicBlockSizeBE;
    int pathTableSizeLE;
    int pathTableSizeBE;
    int locOccLpathTable;
    int locOccOptLpathTable;
    int locOccMpathTable;
    int locOccOptMpathTable;
    ISO9660_entry_t rootDir;
    char volsetname[128];
    char publishId[128];
    char dataPrepId[128];
    char applicationId[128];
    char otherId[128];
    char dateYears[4];
    char dateMonths[2];
    char dateDays[2];
    char dateHours[2];
    char dateMinutes[2];
    char dateSeconds[2];
    char dateHundredthsSec[2];
    char timelag;
});


typedef struct ISO_inode ISO_inode_t;
typedef struct ISO_info ISO_info_t;

struct ISO_info {
    time_t created;
    char bootable;
    int lbaroot;
    int lgthroot;
    int sectorCount;
    int sectorSize;
    inode_t *dev;
    char name[128];
};

struct ISO_inode {
    inode_t ino;
    ISO_info_t *vol;
};



extern vfs_fs_ops_t ISO9660_fs_ops;
extern vfs_dir_ops_t ISO9660_dir_ops;
extern vfs_io_ops_t ISO9660_io_ops;

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

inode_t *ISO9660_lookup(ISO_inode_t *dir, const char *name)
{
    int lba = dir->ino.lba;
    uint8_t *address = kmap(8192, dir->vol->dev, lba * 2048, VMA_FG_RO_FILE);

    // kprintf ("iso9660] Search %s on dir at lba[%x]\n", name, sec);
    // kprintf ("iso9660] Read sector %d on %s \n", sec, dir->name_);

    /* Skip the first two entries */
    ISO9660_entry_t *entry = (ISO9660_entry_t *)address;
    entry = ISO9660_nextEntry(entry); /* Directory '.' */
    entry = ISO9660_nextEntry(entry); /* Directory '..' */

    char *filename = (char *)kalloc(FILENAME_MAX);
    while (entry->lengthRecord) {

        /* Copy and fix filename */
        memcpy(filename, entry->fileId, entry->lengthFileId);
        filename[(int)entry->lengthFileId] = '\0';
        if (filename[entry->lengthFileId - 2 ] == ';') {
            if (filename[entry->lengthFileId - 3] == '.') {
                filename[entry->lengthFileId - 3] = '\0';
            } else {
                filename[entry->lengthFileId - 2] = '\0';
            }
        }

        /* Compare filenames */
        if (strcmp(name, filename) == 0) {

            int mode = entry->fileFlag & 2 ? S_IFDIR : S_IFREG;
            ISO_inode_t *ino = (ISO_inode_t *)vfs_inode(entry->locExtendLE, mode,
                               sizeof(ISO_inode_t));
            ino->ino.length = entry->dataLengthLE;
            ino->ino.lba = entry->locExtendLE;
            ino->ino.block = 2048;
            ino->ino.fs_ops = &ISO9660_fs_ops;
            ino->ino.dir_ops = &ISO9660_dir_ops;
            ino->ino.io_ops = &ISO9660_io_ops;
            ino->vol = dir->vol;

            // ino->atime =

            kunmap(address, 8192);
            kfree(filename);
            errno = 0;
            return &ino->ino;
        }

        /* Move pointer to next entry, eventualy continue directory mapping. */
        entry = ISO9660_nextEntry(entry);
        bool remap = (size_t)entry >= (size_t)address + 8192;
        if (!remap) {
            remap = (size_t)entry + entry->lengthRecord > (size_t)address + 8192;
        }

        if (remap) {
            size_t off = (size_t)entry - (size_t)address - 4096;
            lba += 2;
            kunmap(address, 8192);
            address = kmap(8192, dir->vol->dev, lba * 2048, VMA_FG_RO_FILE);
            entry = (ISO9660_entry_t *)((size_t)address + off);
        }
    }

    kunmap(address, 8192);
    kfree(filename);
    errno = ENOENT;
    return NULL;
}

// int ISO9660_opendir(inode_t *dir, dir_context_t *ctx)
// {
//   ctx->offset = 0;
//   ctx->batch = 4096 / 32; // Max entry per page
//   errno = 0;
//   return -1;
// }

// int ISO9660_readdir(inode_t *dir, dir_context_t *ctx)
// {
//   size_t sec = dir->lba;
//   // struct ISO_info* volume = (struct ISO_info*)dir->dev_->data_;
//   void *address = kmap(4096, device->inode, dir->lba * 2048, VMA_FILE);

//   // kprintf ("iso9660] Search %s on dir at lba[%x]\n", name, sec);
//   // kprintf ("iso9660] Read sector %d on %s \n", sec, dir->name_);

//   //
//   ISO9660_entry_t *entry = (ISO9660_entry_t*)address;

//   // Seek (Skip the first two entries '.' and '..')
//   for (int i=0; i < ctx->offset + 2; ++i) {
//     entry = (ISO9660_entry_t*) & (((char*)entry)[entry->lengthRecord]);
//   }

//   // Loop over records
//   char *filename = (char*)kalloc(FILENAME_MAX);
//   int count = 0;
//   while (entry->lengthRecord) { // TODO leave at the end of the mapping
//     // Copy and fix file name
//     memcpy(filename, entry->fileId, entry->lengthFileId);
//     filename[(int)entry->lengthFileId] = '\0';
//     if (filename[entry->lengthFileId - 2 ] == ';') {
//       if (filename[entry->lengthFileId - 3] == '.') {
//         filename[entry->lengthFileId - 3] = '\0';
//       } else {
//         filename[entry->lengthFileId - 2] = '\0';
//       }
//     }

//     // Register directory entry
//     ctx->entries[count]->lba = entry->locExtendLE;
//     ctx->entries[count]->length = entry->dataLengthLE;
//     ctx->entries[count]->mode = entry->fileFlag & 2 ? S_IFDIR | 0555 : S_IFREG | 0444;
//     strncpy(ctx->entries[count]->name, filename, FILENAME_MAX);
//     // ino->atime =

//     count++;
//     entry = (ISO9660_entry_t*) & (((char*)entry)[entry->lengthRecord]);
//   }

//   ctx->count = count;
//   ctx->eod = entry->lengthRecord == 0;

//   kunmap(address, 4096);
//   kfree(filename);
//   errno = 0;
//   return -1;
// }

// int ISO9660_releasedir(inode_t *dir, dir_context_t *ctx)
// {
//   errno = 0;
//   return 0;
// }


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

inode_t *ISO9660_mount(inode_t *dev)
{
    int i;
    int lba = 16;
    struct ISO_info *volume = NULL;
    if (dev == NULL || dev->block != 2048) {
        errno = ENODEV;
        return NULL;
    }

    uint8_t *address = (uint8_t *)kmap(8192, dev, lba * 2048, VMA_FG_RO_FILE);
    int addressLba = 16;
    int addressCnt = 4;
    for (lba = 16; ; ++lba) {

        if ((lba - addressLba) >= addressCnt) {
            kunmap(address, addressCnt * 2048);
            addressLba = lba;
            address = (uint8_t *)kmap(addressCnt * 2048, dev, addressLba * 2048,
                                      VMA_FG_RO_FILE);
        }

        ISO9660_descriptor_t *descriptor = (ISO9660_descriptor_t *)&address[(lba -
                                           addressLba) * 2048];
        if ((descriptor->magicInt[0] & 0xFFFFFF00) != ISO9660_STD_ID1 ||
                (descriptor->magicInt[1] & 0x0000FFFF) != ISO9660_STD_ID2 ||
                (descriptor->type != ISO9660_VOLDESC_PRIM && !volume)) {
            if (volume) {
                kfree (volume);
            }

            kprintf(-1, " iso9660 -- Not a volume descriptor at lba %d\n", lba);
            kunmap(address, addressCnt * 2048);
            errno = EBADF;
            return NULL;
        }

        if (descriptor->type == ISO9660_VOLDESC_PRIM) {
            volume = (struct ISO_info *)kalloc(sizeof(struct ISO_info));
            volume->bootable = 0;
            volume->created = 0;
            volume->sectorSize = descriptor->logicBlockSizeLE;
            volume->sectorCount = descriptor->volSpaceSizeLE;
            volume->lbaroot = descriptor->rootDir.locExtendLE;
            volume->lgthroot = descriptor->rootDir.dataLengthLE;

            // for (i = 127; i >= 0 && descriptor->applicationId[i] == ' '; --i) {
            //   descriptor->applicationId[i] = '\0'; // TODO -- Horrible !
            // }

            for (i = 31; i >= 0 && descriptor->volname[i] == ' '; --i) {
                descriptor->volname [i] = '\0'; // TODO -- Horrible !
            }

            strcpy (volume->name, descriptor->volname);

        } else if (descriptor->type == ISO9660_VOLDESC_BOOT) {
            volume->bootable = !0;
        } else if (descriptor->type == ISO9660_VOLDESC_TERM) {
            break;
        } else {
            if (volume) {
                kfree (volume);
            }
            kprintf(-1, " iso9660 -- Bad volume descriptor id %d\n", descriptor->type);
            kunmap(address, addressCnt * 2048);
            errno = EBADF;
            return NULL;
        }
    }
    kunmap(address, addressCnt * 2048);


    ISO_inode_t *ino = (ISO_inode_t *)vfs_mountpt(volume->lbaroot, volume->name,
                       "iso9660", sizeof(ISO_inode_t));
    ino->ino.length = volume->lgthroot;
    ino->ino.lba = volume->lbaroot;
    ino->ino.block = 2048;
    ino->ino.fs_ops = &ISO9660_fs_ops;
    ino->ino.dir_ops = &ISO9660_dir_ops;
    ino->ino.io_ops = &ISO9660_io_ops;
    ino->vol = volume;
    ino->vol->dev = vfs_open(dev);

    // kprintf ("ISO9660] Mount device %s at %s.\n", device->name_, name);
    // TODO create_device(name, dev, &stat, volume);
    return &ino->ino;
}

int ISO9660_unmount(ISO_inode_t *ino)
{
    kfree(ino->vol);
    return 0;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int ISO9660_read(ISO_inode_t *ino, void *buffer, size_t length, off_t offset)
{
    int lba = ino->ino.lba;
    return vfs_read(ino->vol->dev, buffer, length, lba * 2048 + offset);
}

int ISO9660_not_allowed()
{
    errno = EROFS;
    return -1;
}

int ISO9660_setup()
{
    vfs_register("iso9660", &ISO9660_fs_ops);
    errno = 0;
    return 0;
}

int ISO9660_teardown()
{
    vfs_unregister("iso9660");
    errno = 0;
    return 0;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

vfs_fs_ops_t ISO9660_fs_ops = {
    .mount = (fs_mount)ISO9660_mount,
    .unmount = (fs_unmount)ISO9660_unmount,
};

vfs_dir_ops_t ISO9660_dir_ops = {

    .lookup = (fs_lookup)ISO9660_lookup,


    // .link = ISO9660_link,
    // .unlink = ISO9660_unlink,
    // .rmdir = ISO9660_rmdir,
};

vfs_io_ops_t ISO9660_io_ops = {
    .read = (fs_read)ISO9660_read,
    // .write = ISO9660_write,
    // .truncate = ISO9660_truncate,
};
