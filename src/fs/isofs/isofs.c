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
#include "isofs.h"

/* Copy the filename read from a directory entry */
static void isofs_filename(ISOFS_entry_t *entry, char *name)
{
    memcpy(name, entry->fileId, entry->lengthFileId);
    name[(int)entry->lengthFileId] = '\0';
    if (name[entry->lengthFileId - 2 ] == ';') {
        if (name[entry->lengthFileId - 3] == '.') {
            name[entry->lengthFileId - 3] = '\0';
        } else {
            name[entry->lengthFileId - 2] = '\0';
        }
    }
}

/* */
static ISO_inode_t *isofs_inode(ISO_info_t *vol, ISOFS_entry_t *entry, acl_t *acl)
{

    int mode = entry->fileFlag & 2 ? S_IFDIR | 0755 : S_IFREG | 0644;
    ISO_inode_t *ino = (ISO_inode_t *)vfs_inode(entry->locExtendLE, mode,
                       acl, sizeof(ISO_inode_t));
    ino->ino.length = entry->dataLengthLE;
    ino->ino.lba = entry->locExtendLE;
    ino->vol = vol;
    return ino;
}


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

ISO_dirctx_t *isofs_opendir(ISO_inode_t *dir)
{
    ISO_dirctx_t *ctx = (ISO_dirctx_t*)kalloc(sizeof(ISO_dirctx_t));
    errno = 0;
    return ctx;
}

int isofs_closedir(ISO_inode_t *dir, ISO_dirctx_t *ctx)
{
    if (ctx->base != NULL)
        kunmap(ctx->base, 8192);
    kfree(ctx);
    errno = 0;
    return 0;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


ISO_inode_t *isofs_lookup(ISO_inode_t *dir, const char *name)
{
    int lba = dir->ino.lba;
    uint8_t *address = kmap(8192, dir->vol->dev, lba * 2048, VMA_FG_RO_FILE);

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
            ISO_inode_t *ino = isofs_inode(dir->vol, entry, NULL);
            kunmap(address, 8192);
            kfree(filename);
            errno = 0;
            return ino;
        }

        /* Move pointer to next entry, eventualy continue directory mapping. */
        entry = ISOFS_nextEntry(entry);
        bool remap = (size_t)entry >= (size_t)address + 8192;
        if (!remap) {
            remap = (size_t)entry + entry->lengthRecord > (size_t)address + 8192;
        }

        if (remap) {
            size_t off = (size_t)entry - (size_t)address - 4096;
            lba += 2;
            kunmap(address, 8192);
            address = kmap(8192, dir->vol->dev, lba * 2048, VMA_FG_RO_FILE);
            entry = (ISOFS_entry_t *)((size_t)address + off);
        }
    }

    kunmap(address, 8192);
    kfree(filename);
    errno = ENOENT;
    return NULL;
}

ISO_inode_t *isofs_readdir(ISO_inode_t *dir, char *name, ISO_dirctx_t *ctx)
{
    if (ctx->lba == 0)
        ctx->lba = dir->ino.lba;
    if (ctx->base == NULL)
        ctx->base = kmap(8192, dir->vol->dev, ctx->lba * 2048, VMA_FG_RO_FILE);

    /* Skip the first two entries */
    ISOFS_entry_t *entry = (ISOFS_entry_t *)(&ctx->base[ctx->off]);
    while (entry->lengthRecord) {
        isofs_filename(entry, name);
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0 || name[0] < 0x20) {
            entry = ISOFS_nextEntry(entry);
            continue;
        }
        ISO_inode_t *ino = isofs_inode(dir->vol, entry, NULL);

        /* Move pointer to next entry, eventualy continue directory mapping. */
        entry = ISOFS_nextEntry(entry);
        bool remap = (size_t)entry >= (size_t)ctx->base + 8192;
        if (!remap) {
            remap = (size_t)entry + entry->lengthRecord > (size_t)ctx->base + 8192;
        }

        ctx->off = (size_t)entry - (size_t)ctx->base;
        if (remap) {
            ctx->off -= 4096;
            ctx->lba += 2;
            kunmap(ctx->base, 8192);
            ctx->base = NULL;
        }

        errno = 0;
        return ino;
    }

    errno = 0;
    return NULL;
}

// int ISOFS_opendir(inode_t *dir, dir_context_t *ctx)
// {
//   ctx->offset = 0;
//   ctx->batch = 4096 / 32; // Max entry per page
//   errno = 0;
//   return -1;
// }

// int ISOFS_readdir(inode_t *dir, dir_context_t *ctx)
// {
//   size_t sec = dir->lba;
//   // struct ISO_info* volume = (struct ISO_info*)dir->dev_->data_;
//   void *address = kmap(4096, device->inode, dir->lba * 2048, VMA_FILE);

//   // kprintf ("isofs] Search %s on dir at lba[%x]\n", name, sec);
//   // kprintf ("isofs] Read sector %d on %s \n", sec, dir->name_);

//   //
//   ISOFS_entry_t *entry = (ISOFS_entry_t*)address;

//   // Seek (Skip the first two entries '.' and '..')
//   for (int i=0; i < ctx->offset + 2; ++i) {
//     entry = (ISOFS_entry_t*) & (((char*)entry)[entry->lengthRecord]);
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
//     entry = (ISOFS_entry_t*) & (((char*)entry)[entry->lengthRecord]);
//   }

//   ctx->count = count;
//   ctx->eod = entry->lengthRecord == 0;

//   kunmap(address, 4096);
//   kfree(filename);
//   errno = 0;
//   return -1;
// }

// int ISOFS_releasedir(inode_t *dir, dir_context_t *ctx)
// {
//   errno = 0;
//   return 0;
// }


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

inode_t *ISOFS_mount(inode_t *dev)
{
    int i;
    int lba = 16;
    struct ISO_info *volume = NULL;
    if (dev == NULL) {
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

        ISOFS_descriptor_t *descriptor = (ISOFS_descriptor_t *)&address[(lba -
                                           addressLba) * 2048];
        if ((descriptor->magicInt[0] & 0xFFFFFF00) != ISOFS_STD_ID1 ||
                (descriptor->magicInt[1] & 0x0000FFFF) != ISOFS_STD_ID2 ||
                (descriptor->type != ISOFS_VOLDESC_PRIM && !volume)) {
            if (volume) {
                kfree (volume);
            }

            kprintf(-1, " isofs -- Not a volume descriptor at lba %d\n", lba);
            kunmap(address, addressCnt * 2048);
            errno = EBADF;
            return NULL;
        }

        if (descriptor->type == ISOFS_VOLDESC_PRIM) {
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

        } else if (descriptor->type == ISOFS_VOLDESC_BOOT) {
            volume->bootable = !0;
        } else if (descriptor->type == ISOFS_VOLDESC_TERM) {
            break;
        } else {
            if (volume) {
                kfree (volume);
            }
            kprintf(-1, " isofs -- Bad volume descriptor id %d\n", descriptor->type);
            kunmap(address, addressCnt * 2048);
            errno = EBADF;
            return NULL;
        }
    }
    kunmap(address, addressCnt * 2048);


    ISO_inode_t *ino = (ISO_inode_t *)vfs_inode(volume->lbaroot, S_IFDIR | 0755, NULL, sizeof(ISO_inode_t));
    ino->ino.length = volume->lgthroot;
    ino->ino.lba = volume->lbaroot;
    ino->vol = volume;
    ino->vol->dev = vfs_open(dev);

    mountfs_t *fs = (mountfs_t*)kalloc(sizeof(mountfs_t));
    fs->lookup = (fs_lookup)isofs_lookup;
    fs->read = (fs_read)ISOFS_read;
    fs->umount = (fs_umount)ISOFS_umount;
    fs->opendir = (fs_opendir)isofs_opendir;
    fs->readdir = (fs_readdir)isofs_readdir;
    fs->closedir = (fs_closedir)isofs_closedir;
    fs->read_only = true;

    vfs_mountpt(volume->name, "isofs", fs, (inode_t*)ino);
    return &ino->ino;
}

int ISOFS_umount(ISO_inode_t *ino)
{
    vfs_close(ino->vol->dev);
    kfree(ino->vol);
    return 0;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int ISOFS_read(ISO_inode_t *ino, void *buffer, size_t length, off_t offset)
{
    int lba = ino->ino.lba;
    return vfs_read(ino->vol->dev, buffer, length, lba * 2048 + offset);
}

int ISOFS_not_allowed()
{
    errno = EROFS;
    return -1;
}

int ISOFS_setup()
{
    register_fs("isofs", (fs_mount)ISOFS_mount);
    errno = 0;
    return 0;
}

int ISOFS_teardown()
{
    unregister_fs("isofs");
    errno = 0;
    return 0;
}
