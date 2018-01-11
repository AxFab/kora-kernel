/*
 *      This file is part of the SmokeOS project.
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
#include <skc/bbtree.h>
#include <string.h>
#include <errno.h>


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
      char type;
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
  struct ISO_dirEntry rootDir;
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


struct ISO_info {
  time_t created;
  char bootable;
  char *name;
  int lbaroot;
  int lgthroot;
  int sectorCount;
  int sectorSize;
};



extern vfs_fs_ops_t ISO9660_fs_ops;
extern vfs_dir_ops_t ISO9660_dir_ops;
extern vfs_io_ops_t ISO9660_io_ops;

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int ISO9660_mount(inode_t* ino, device_t device)
{
  int i;
  int inDesc = 1;
  int lba = 16;
  char cdName[128];
  uint8_t *address;
  uint32_t *addressInt;
  struct ISO_descFirst *firstDesc;
  struct ISO_info *volume = NULL;
  SMK_stat_t stat;
  kMemArea_t *area;
  time_t now = time(NULL);

  if (device == NULL || device->block != 2048) {
    errno = ENODEV;
    return -1;
  }

  void *address = kmap(4096, device->inode, lba * 2048, VMA_FILE);
  for (lba = 16; ; ++lba) {

    // TODO -- Check need for remap
    ISO9660_descriptor_t *descriptor = (ISO9660_descriptor_t *)address;

    if ((descriptor->magicInt[0] & 0xFFFFFF00) != ISO9660_STD_ID1 ||
        (descriptor->magicInt[1] & 0x0000FFFF) != ISO9660_STD_ID2 ||
        (descriptor->type != ISO9660_VOLDESC_PRIM && !volume)) {
      if (volume) {
        kfree (volume);
      }

      kunmap(address, 4096);
      errno = EBADF;
      return -1;
    }

    if (descriptor->type == ISO9660_VOLDESC_PRIM) {
      volume = KALLOC(struct ISO_info);
      volume->bootable = 0;
      volume->created = 0;
      volume->sectorSize = descriptor->logicBlockSizeLE;
      volume->sectorCount = descriptor->volSpaceSizeLE;
      volume->lbaroot = descriptor->rootDir.locExtendLE;
      volume->lgthroot = descriptor->rootDir.dataLengthLE;

      // kprintf ("ROOT { %x - %x }\n", descriptor->rootDir.locExtendLE, descriptor->rootDir.dataLengthLE);
      // kprintf ("VOLUME NAME '%s'\n", descriptor->volname);

      for (i = 127; i >= 0 && descriptor->applicationId[i] == ' '; --i) {
        descriptor->applicationId[i] = '\0';
      }

      for (i = 31; i >= 0 && descriptor->volname[i] == ' '; --i) {
          descriptor->volname [i] = '\0';
      }

      strcpy (cdName, descriptor->volname);

      // kprintf ("iso9660] This disc is named '%s' \n", descriptor->applicationId);

      //err = Fsys_ChangeVolumeName (prim->applicationId);
    } else if (descriptor->type == ISO9660_VOLDESC_BOOT) {
      // kprintf ("iso9660] Bootable descriptor\n");
      volume->bootable = !0;
    } else if (descriptor->type == ISO9660_VOLDESC_TERM) {
      // kprintf ("iso9660] Terminal descriptor\n");
      break;
    } else {
      // kprintf ("iso9660] Bad volume descriptor id %d\n", buf[0]);
      if (volume) {
        kfree (volume);
      }

      kunmap(address, 4096);
      errno = EBADF;
      return -1;
    }
  }
  kunmap(address, 4096);

  memset(&stat, 0, sizeof(stat));
  ino.atime_ = now;
  ino.ctime_ = now;
  ino.mtime_ = now;
  ino.mode = S_IFDIR | 0555;
  ino.lba = volume->lbaroot;
  ino.length = volume->lgthroot;

  // kprintf ("ISO9660] Mount device %s at %s.\n", device->name_, name);
  // TODO create_device(name, dev, &stat, volume);
  return 0;
}

int ISO9660_unmount(inode_t* ino)
{
  // ...
  return 0;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int ISO9660_lookup(inode_t* dir, inode_t* ino, const char *name)
{
  size_t sec = dir->lba;
  // struct ISO_info* volume = (struct ISO_info*)dir->dev_->data_;
  void *address = kmap(4096, device->inode, dir->lba * 2048, VMA_FILE);

  // kprintf ("iso9660] Search %s on dir at lba[%x]\n", name, sec);
  // kprintf ("iso9660] Read sector %d on %s \n", sec, dir->name_);

  // Skip the first two entries '.' and '..'
  ISO9660_entry_t *entry = (ISO9660_entry_t*)address;
  entry = (ISO9660_entry_t*) & (((char*)entry)[entry->lengthRecord]);
  entry = (ISO9660_entry_t*) & (((char*)entry)[entry->lengthRecord]);

  // Loop over records
  char *filename = (char*)kalloc(FILENAME_MAX);
  while (entry->lengthRecord) {
    // Copy and fix file name
    memcpy(filename, entry->fileId, entry->lengthFileId);
    filename[(int)entry->lengthFileId] = '\0';
    if (filename[entry->lengthFileId - 2 ] == ';') {
      if (filename[entry->lengthFileId - 3] == '.') {
        filename[entry->lengthFileId - 3] = '\0';
      } else {
        filename[entry->lengthFileId - 2] = '\0';
      }
    }

    // Compare filenames
    if (strcmpi(name, filename) == 0) {
      ino->lba = entry->locExtendLE;
      ino->length = entry->dataLengthLE;
      ino->mode = entry->fileFlag & 2 ? S_IFDIR | 0555 : S_IFREG | 0444;
      // ino->atime =
      kunmap(address, 4096);
      kfree(filename);
      errno = 0;
      return 0;
    }

    entry = (ISO9660_entry_t*) & (((char*)entry)[entry->lengthRecord]);
  }

  kunmap(address, 4096);
  kfree(filename);
  errno = ENOENT;
  return -1;
}

int ISO9660_opendir(inode_t *dir, dir_context_t *ctx)
{
  ctx->offset = 0;
  ctx->batch = 4096 / 32; // Max entry per page
  errno = 0;
  return -1;
}

int ISO9660_readdir(inode_t *dir, dir_context_t *ctx)
{
  size_t sec = dir->lba;
  // struct ISO_info* volume = (struct ISO_info*)dir->dev_->data_;
  void *address = kmap(4096, device->inode, dir->lba * 2048, VMA_FILE);

  // kprintf ("iso9660] Search %s on dir at lba[%x]\n", name, sec);
  // kprintf ("iso9660] Read sector %d on %s \n", sec, dir->name_);

  //
  ISO9660_entry_t *entry = (ISO9660_entry_t*)address;

  // Seek (Skip the first two entries '.' and '..')
  for (int i=0; i < ctx->offset + 2; ++i) {
    entry = (ISO9660_entry_t*) & (((char*)entry)[entry->lengthRecord]);
  }

  // Loop over records
  char *filename = (char*)kalloc(FILENAME_MAX);
  int count = 0;
  while (entry->lengthRecord) { // TODO leave at the end of the mapping
    // Copy and fix file name
    memcpy(filename, entry->fileId, entry->lengthFileId);
    filename[(int)entry->lengthFileId] = '\0';
    if (filename[entry->lengthFileId - 2 ] == ';') {
      if (filename[entry->lengthFileId - 3] == '.') {
        filename[entry->lengthFileId - 3] = '\0';
      } else {
        filename[entry->lengthFileId - 2] = '\0';
      }
    }

    // Register directory entry
    ctx->entries[count]->lba = entry->locExtendLE;
    ctx->entries[count]->length = entry->dataLengthLE;
    ctx->entries[count]->mode = entry->fileFlag & 2 ? S_IFDIR | 0555 : S_IFREG | 0444;
    strncpy(ctx->entries[count]->name, filename, FILENAME_MAX);
    // ino->atime =

    count++;
    entry = (ISO9660_entry_t*) & (((char*)entry)[entry->lengthRecord]);
  }

  ctx->count = count;
  ctx->eod = entry->lengthRecord == 0;

  kunmap(address, 4096);
  kfree(filename);
  errno = 0;
  return -1;
}

int ISO9660_releasedir(inode_t *dir, dir_context_t *ctx)
{
  errno = 0;
  return 0;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

// int ISO9660_read(inode_t* ino, const char *buf, size_t size, off_t offset)
// {
//   return -1;
// }

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int ISO9660_write()
{
  errno = EROFS;
  return -1;
}

vfs_fs_ops_t ISO9660_fs_ops = {
  .mount = ISO9660_mount,
};

vfs_dir_ops_t ISO9660_dir_ops = {

  .lookup = ISO9660_lookup,


  // .link = ISO9660_link,
  .unlink = ISO9660_unlink,
  .rmdir = ISO9660_rmdir,
};

vfs_io_ops_t ISO9660_io_ops = {
  .read = NULL,
  // .read = ISO9660_read,
  // .write = ISO9660_write,
  // .truncate = ISO9660_truncate,
};
