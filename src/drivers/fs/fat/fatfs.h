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
 *
 *      File system driver FAT12, FAT16, FAT32 and exFAT.
 */
#ifndef _SRC_FATFS_H
#define _SRC_FATFS_H 1

#include <kernel/device.h>
#include <kernel/core.h>
#include <kora/mcrs.h>
#include <errno.h>
#include <string.h>

/* Identificators for volume descriptors */
#define FAT12 12
#define FAT16 16
#define FAT32 32

#define ATTR_READ_ONLY      0x01
#define ATTR_HIDDEN       0x02
#define ATTR_SYSTEM       0x04
#define ATTR_VOLUME_ID      0x08
#define ATTR_DIRECTORY      0x10
#define ATTR_ARCHIVE      0x20
#define ATTR_LONG_NAME      0x0F
#define ATTR_LONG_NAME_MASK   0x3F
#define ATTR_DELETED        0xE5


#define CLUSTER_OF(v,s)   ((((s) - (v)->FirstDataSector) / (v)->SecPerClus) + 2)
#define SECTOR_OF(v,s)    ((((s) - 2) * (v)->SecPerClus) + (v)->FirstDataSector)
#define FSECTOR_FROM(v,s) ((((((((s) - (v)->FirstDataSector) / (v)->SecPerClus) + 2)) - 2) * (v)->SecPerClus) + (v)->FirstDataSector)

#define FAT_TYPE(cl)  ((cl) < 4085 ? FAT12 : ((cl) < 65525 ? FAT16 : FAT32))

PACK(struct BPB_Struct {
    unsigned char BS_jmpBoot [3];
    char      BS_OEMName [8];
    unsigned short  BPB_BytsPerSec;
    unsigned char BPB_SecPerClus;
    unsigned short  BPB_ResvdSecCnt;
    unsigned char BPB_NumFATs;
    unsigned short  BPB_RootEntCnt;
    unsigned short  BPB_TotSec16;
    unsigned char BPB_Media;
    unsigned short  BPB_FATSz16;
    unsigned short  BPB_SecPerTrk;
    unsigned short  BPB_NumHeads;
    unsigned int  BPB_HiddSec;
    unsigned int  BPB_TotSec32;

    unsigned char BS_DrvNum;
    unsigned char BS_Reserved1;
    unsigned char BS_BootSig;
    unsigned int  BS_VolID;
    char      BS_VolLab [11];
    char      BS_FilSysType [8];
});

PACK(struct BPB_Struct32 {
    unsigned char BS_jmpBoot [3];
    char      BS_OEMName [8];
    unsigned short  BPB_BytsPerSec;
    unsigned char BPB_SecPerClus;
    unsigned short  BPB_ResvdSecCnt;
    unsigned char BPB_NumFATs;
    unsigned short  BPB_RootEntCnt;
    unsigned short  BPB_TotSec16;
    unsigned char BPB_Media;
    unsigned short  BPB_FATSz16;
    unsigned short  BPB_SecPerTrk;
    unsigned short  BPB_NumHeads;
    unsigned int  BPB_HiddSec;
    unsigned int  BPB_TotSec32;

    unsigned int  BPB_FATSz32;
    unsigned short  BPB_ExtFlags;
    unsigned short  BPB_FSVer;
    unsigned int  BPB_RootClus;
    unsigned short  BPB_FSInfo;
    unsigned short  BPB_BkBootSec;
    char      BPB_Reserved [12];

    unsigned char BS_DrvNum;
    unsigned char BS_Reserved1;
    unsigned char BS_BootSig;
    unsigned int  BS_VolID;
    char      BS_VolLab [11];
    char      BS_FilSysType [8];
});

PACK(struct FAT_ShortEntry {
    unsigned char DIR_Name[11];
    unsigned char DIR_Attr;
    unsigned char DIR_NTRes;
    unsigned char DIR_CrtTimeTenth;
    unsigned short  DIR_CrtTime;
    unsigned short  DIR_CrtDate;
    unsigned short  DIR_LstAccDate;
    unsigned short  DIR_FstClusHi;
    unsigned short  DIR_WrtTime;
    unsigned short  DIR_WrtDate;
    unsigned short  DIR_FstClusLo;
    unsigned int  DIR_FileSize;
});

PACK(struct FAT_LongNameEntry {
    unsigned char LDIR_Ord;
    unsigned short  LDIR_Name1[5];
    unsigned char LDIR_Attr;
    unsigned char LDIR_Type;
    unsigned char LDIR_Cheksum;
    unsigned short  LDIR_Name2[6];
    unsigned short  LDIR_FstClusLO;
    unsigned short  LDIR_Name3[2];
});



/** Structure for a volume in FAT FS */
struct FAT_volume {
    char    name[48];
    char    label[8];
    long long totalSize;
    long long usedSpace;
    long long freeSpace;
    unsigned int RootDirSectors;
    unsigned int FATSz;
    unsigned int FirstDataSector;
    unsigned int TotSec;
    unsigned int DataSec;
    unsigned int CountofClusters;
    unsigned int ResvdSecCnt;
    unsigned int BytsPerSec;
    int FATType;
    unsigned int RootEntry;
    int SecPerClus;
    bio_t *io_head;
    bio_t *io_data_rw;
    bio_t *io_data_ro;
};

/* FAT FS SRC
  alloc.c
    int alloc_cluster(info, int prev, size_t size);
    int compute_space(info)
  dir.c
     mkdir(info, dir, ino, name);
     unlink(info, dir, name);
     opendir
     readdir
     closedir
     ...
  data.c
     read
     write
     chmod
     utimes

  volume.c
     read_info();
  setup.c
    setup
    teardown
    format
    mount
    umount
  fat.h

*/

typedef struct FAT_inode FAT_inode_t;
typedef struct FAT_diterator  FAT_diterator_t;


struct FAT_inode {
    inode_t ino;
    struct FAT_volume *vol;
};



void fatfs_settime(unsigned short *date, unsigned short *time, time64_t value);
time64_t fatfs_gettime(unsigned short *date, unsigned short *time);
void fatfs_read_shortname(struct FAT_ShortEntry *entry, char *shortname);
void fatfs_write_shortname(struct FAT_ShortEntry *entry, const char *shortname);
FAT_inode_t *fatfs_inode(int no, struct FAT_ShortEntry *entry, struct FAT_volume *info);
void fatfs_short_entry(struct FAT_ShortEntry *entry, unsigned cluster, int mode);
int fatfs_mkdir(struct FAT_volume *info, FAT_inode_t *dir);

struct FAT_volume *fatfs_init(void *ptr);
void fatfs_reserve_cluster_16(struct FAT_volume *info, int cluster, int previous);
unsigned fatfs_alloc_cluster_16(struct FAT_volume *info, int previous);

FAT_inode_t *fatfs_open(FAT_inode_t *dir, CSTR name, int mode, acl_t *acl, int flags);
int fatfs_unlink(FAT_inode_t *dir, CSTR name);

FAT_diterator_t *fatfs_opendir(FAT_inode_t *dir);
FAT_inode_t *fatfs_readdir(FAT_inode_t *dir, char *name, FAT_diterator_t *it);
int fatfs_closedir(FAT_inode_t *dir, FAT_diterator_t *it);

#define FILENAME_MAX  256

#endif /* _SRC_FATFS_H */
