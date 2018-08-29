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


#define CLUSTER_OF(v,s)   ((((s) - (v)->FirstDataSector) / (v)->SecPerClus) + 2)
#define SECTOR_OF(v,s)    ((((s) - 2) * (v)->SecPerClus) + (v)->FirstDataSector)
#define FSECTOR_FROM(v,s) ((((((((s) - (v)->FirstDataSector) / (v)->SecPerClus) + 2)) - 2) * (v)->SecPerClus) + (v)->FirstDataSector)

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
};

typedef struct FAT_inode FAT_inode_t;

struct FAT_inode {
    inode_t ino;
    struct FAT_volume *vol;
};

// /* ----------------------------------------------------------------------- */
// static char* FAT_build_short_name(struct FAT_ShortEntry *entrySh)
// {
//   int i;
//   char str [9] = { 0 };
//   memcpy (str, entrySh->DIR_Name, 8);
//   // for (i=1; i<9; ++i)
//   //   str[i] = tolower(str[i]);
//   for (i=7; i>0; --i) {
//     if (str[i] > 0x20)
//       break;
//     str[i] = 0;
//   }

//   return strdup (str);
// }

// /* ----------------------------------------------------------------------- */
// static char* FAT_build_long_name(struct FAT_LongNameEntry *entryLg)
// {
//   uint16_t unicode;
//   char part [72];
//   int i=0, j=0;

//   for (j=0; j<5; ++j) {
//     unicode = entryLg->LDIR_Name1[j];
//     if (unicode < 0x80)
//       part[i++] = unicode & 0x7F;
//     else
//       part[i++] = '-';
//   }

//   for (; j<11; ++j) {
//     unicode = entryLg->LDIR_Name2[j - 5];
//     if (unicode < 0x80)
//       part[i++] = unicode & 0x7F;
//     else
//       part[i++] = '-';
//   }

//   for (; j<13; ++j) {
//     unicode = entryLg->LDIR_Name3[j - 11];
//     if (unicode < 0x80)
//       part[i++] = unicode & 0x7F;
//     else
//       part[i++] = '-';
//   }

//   part[i] = 0;
//   return strdup(part);
// }

// /* ----------------------------------------------------------------------- */
// static char* FAT_name_concat(char* s1, char* s2)
// {
//   int lg = strlen(s1) + strlen(s2) + 1;
//   char* part = (char*)kalloc(lg);
//   strncpy(part, s1, lg);
//   strncat(part, s2, lg);
//   return part;
// }

// /* ----------------------------------------------------------------------- */
// static time_t FAT_read_time(unsigned short date, unsigned short time)
// {
//   struct tm datetime;
//   datetime.tm_mday = date & 0x1F;
//   datetime.tm_mon = (date >> 5) & 0x0F;
//   datetime.tm_year = (date >> 9) + 80; // since 1900
//   datetime.tm_sec = (time & 0x1F) << 1;
//   datetime.tm_min = (time >> 5) & 0x3F;
//   datetime.tm_hour = time >> 11;
//   return mktime(&datetime);
// }

// static struct FAT_LongNameEntry * FAT_prepare_entries (const char *name, int mode, SMK_stat_t *stat)
// {
//   struct FAT_LongNameEntry * entries;
//   // struct FAT_ShortEntry * shEntry;
//   const char *lname = name;
//   int i, j, eIdx = 0;
//   int *ptr;
//   int nlg = strlen (name); // @todo UTF-16
//   int enCount = nlg / 13;
//   if (nlg % 13)
//     enCount++;

//   entries = (struct FAT_LongNameEntry *)kalloc ((enCount + 1) * sizeof(struct FAT_LongNameEntry));
//   // shEntry = (struct FAT_ShortEntry *)(&entries[enCount]);

//   for (i=0,j=0; i < nlg; ++i, ++j) {
//     if (j >= 13)
//       eIdx++;

//     if (j < 5)
//       entries[eIdx].LDIR_Name1[j] = *lname; // @todo get from UTF-16 string!
//     else if (j < 11)
//       entries[eIdx].LDIR_Name2[j - 5] = *lname; // @todo get from UTF-16 string!
//     else
//       entries[eIdx].LDIR_Name3[j - 11] = *lname; // @todo get from UTF-16 string!
//     lname++;
//   }

//   for (eIdx=0; eIdx < enCount; ++eIdx) {
//     ptr = (int*)(&entries[eIdx]);
//     entries[eIdx].LDIR_Attr = ATTR_LONG_NAME;
//     entries[eIdx].LDIR_Ord = eIdx + 1;
//     entries[eIdx].LDIR_Cheksum = XOR_32_TO_8 (ptr[0] ^ ptr[1] ^ ptr[2] ^ ptr[3] ^ ptr[4] ^ ptr[5] ^ ptr[6] ^ ptr[7]);
//   }

//   // shEntry->DIR_Name[0] =
//   // shEntry->DIR_Attr =

//   return entries;
// }


// /* ----------------------------------------------------------------------- */
// // int FAT_readdir (const _pFileNode fn) {
// // }

// /* ----------------------------------------------------------------------- */
// int FAT_read(kInode_t *fp, void *buffer, size_t length, size_t offset)
// {
//   struct FAT_Volume* volume = (struct FAT_Volume*)fp->dev_->data_;
//   size_t clustSz = volume->SecPerClus * volume->BytsPerSec;
//   size_t lba = fp->stat_.lba_;
//   uint8_t *fat;
//   uint32_t fatEntry;
//   size_t fileOff = 0;
//   size_t fatCur = 0;
//   kMemArea_t *fatArea = NULL;
//   size_t cluster;
//   size_t fatSec;
//   size_t fatOff;

//   for (;;) {
//     if (fileOff + clustSz > offset && fileOff + clustSz < offset + length) {
//       size_t cap = length;
//       cap = MIN (cap, clustSz);
//       if (offset > fileOff)
//         cap = MIN (cap, clustSz - (offset - fileOff));
//       if (fileOff + clustSz > fp->stat_.length_)
//         cap = MIN (cap, fp->stat_.length_ - offset);
//       fs_block_read(fp->dev_->underlyingDev_, buffer, cap, lba * 512 + offset);

//       length -= cap;
//       buffer = ((char*)buffer) + cap;
//     }

//     if (length <= 0) {
//       if (fatArea)
//         area_unmap(kSYS.mspace_, fatArea);
//       return 0;
//     }

//     cluster = CLUSTER_OF(volume, lba);
//     fatSec = volume->ResvdSecCnt + (cluster / 128);
//     fatOff = cluster % 128;
//     if (fatArea == NULL || fatSec != fatCur) {
//       fatCur = fatSec;
//       fatArea = area_map_ino(kSYS.mspace_, fp->dev_->underlyingDev_->dev_->ino_, fatSec * 512, 512, 0);
//       fat = (uint8_t *)fatArea->address_;
//     }

//     fatEntry = ((uint32_t*)fat)[fatOff];
//     if (fatEntry < 0xFFFFFFF) {
//       fileOff += clustSz;
//       lba = SECTOR_OF(volume, fatEntry);
//       continue;
//     }

//     return EIO;
//   }
// }


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

// inode_t *FAT_lookup(FAT_inode_t *dir, const char *name)
// {
//     int idx;
//     const char *filename = NULL;

//     for (;;) {
//         uint32_t cluster = CLUSTER_OF(dir->vol, dir->ino.lba);
//         uint32_t offset = SECTOR_OF(dir->vol, cluster) * dir->vol->dev->block;
//         uint8_t *ptr = kmap(dir->vol->clustSz, dir->vol->dev, offset, VMA_FG_RO_FILE);

//         struct FAT_ShortEntry *entry = (struct FAT_ShortEntry *)ptr;
//         for (idx = 0; idx < 16; ++idx, ++entry) {

//             if (entry->DIR_Name[0] == 0xE5 || entry->DIR_Name[0] == 0x05 ||
//                     entry->DIR_Attr == ATTR_VOLUME_ID) {
//                 continue;
//             } else if (entry->DIR_Name[0] == 0) {
//                 kunmap(ptr, dir->vol->clustSz);
//                 error = ENOENT;
//                 return NULL;
//             } else if ((entry->DIR_Attr & ATTR_LONG_NAME) == ATTR_LONG_NAME) {
//                 // Build Long filename
//                 continue;
//             }

//             if (!filename) {
//                 // filename = FAT_filename_short(entry);
//             }

//             if (strcmpi(name, filename) != 0) {
//                 kfree(filename);
//                 filename = NULL;
//                 continue;
//             }

//             int lba = entry->DIR_FstClusLo | (entry->DIR_FstClusLo << 16);
//             int mode = entry->DIR_Attr & ATTR_DIRECTORY ? S_ISDIR : S_ISREG;
//             FAT_inode_t *ino = vfs_inode(lba, mode, sizeof(FAT_inode_t));
//             ino->length = entry->DIR_FileSize;
//             ino->lba = entry->DIR_FstClusLo | (entry->DIR_FstClusLo << 16);
//             return ino;
//         }

//         // TODO -- Go to next cluster! Using FAT table
//     }
// }



// /* ----------------------------------------------------------------------- */
// int FAT_create (const char *name, kInode_t *dir, int mode, size_t lg, SMK_stat_t *stat)
// {
//   struct FAT_Volume* volume = (struct FAT_Volume*)dir->dev_->data_;
//   size_t clustSz = volume->SecPerClus * volume->BytsPerSec;
//   size_t lba = dir->stat_.lba_;
//   uint8_t *fat;
//   uint32_t fatEntry;
//   size_t curCluster = 0;
//   size_t cluster;
//   size_t fatCur = 0;
//   size_t fatSec;
//   size_t fatOff;
//   int idx;
//   char* filename = NULL;
//   struct FAT_ShortEntry *entrySh;
//   struct FAT_LongNameEntry *entryLg;
//   kMemArea_t *fatArea = NULL;
//   kMemArea_t *dirArea = NULL;

//   // We need how many longentry !?
//   entryLg = FAT_prepare_entries(name, mode, stat);
//   __unused(entryLg);

//   for (;;) {
//     cluster = CLUSTER_OF(volume, lba);
//     if (dirArea == NULL || curCluster != cluster) {
//       if (dirArea != NULL)
//         area_unmap(kSYS.mspace_, dirArea);
//       dirArea = area_map_ino(kSYS.mspace_, dir->dev_->underlyingDev_->dev_->ino_, SECTOR_OF(volume, cluster) * 512, clustSz, 0);
//       fatSec = volume->ResvdSecCnt + (cluster / 128);
//       fatOff = cluster % 128;
//     }

//     entrySh = (struct FAT_ShortEntry *)dirArea->address_;
//     for (idx = 0; idx < 16 * volume->SecPerClus; entrySh++, idx++) {
//       if (entrySh->DIR_Name[0] == 0xE5 || entrySh->DIR_Name[0] == 0x05 || entrySh->DIR_Attr == ATTR_VOLUME_ID)
//         continue;

//       if (entrySh->DIR_Name[0] == 0) {
//         area_unmap(kSYS.mspace_, dirArea);
//         if (fatArea)
//           area_unmap(kSYS.mspace_, fatArea);
//         if (filename)
//           kfree(filename);
//         return ENOENT;
//       }

//       if ((entrySh->DIR_Attr & ATTR_LONG_NAME_MASK) == ATTR_LONG_NAME) {
//         if (filename) {
//           char* part = FAT_build_long_name((struct FAT_LongNameEntry *)entrySh);
//           char* tmp = FAT_name_concat(part, filename);
//           kfree(part);
//           kfree(filename);
//           filename = tmp;
//         } else {
//           filename = FAT_build_long_name((struct FAT_LongNameEntry *)entrySh);
//         }
//         continue;
//       }

//       if (!filename)
//         filename = FAT_build_short_name(entrySh);

//       if (strcmpi(name, filename) != 0) {
//         kfree(filename);
//         filename = NULL;
//         continue;
//       }

//       stat->ctime_ = FAT_read_time(entrySh->DIR_CrtDate, entrySh->DIR_CrtTime);
//       stat->atime_ = FAT_read_time(entrySh->DIR_LstAccDate, 0);
//       stat->mtime_ = FAT_read_time(entrySh->DIR_WrtDate, entrySh->DIR_WrtTime);
//       stat->mode_ = (entrySh->DIR_Attr & ATTR_DIRECTORY ? S_IFDIR: S_IFREG) | 0777;
//       stat->lba_ = SECTOR_OF(volume, entrySh->DIR_FstClusLo | (entrySh->DIR_FstClusHi << 16));
//       stat->length_ = entrySh->DIR_FileSize;

//       // kprintf ("FAT] %s (%c - %s)\n", filename, (entrySh->DIR_Attr & ATTR_DIRECTORY ? 'd': '-'), kpsize(stat->length_));
//       area_unmap(kSYS.mspace_, dirArea);
//       if (fatArea)
//         area_unmap(kSYS.mspace_, fatArea);
//       kfree(filename);
//       return 0;
//     }

//     if (fatArea == NULL || fatSec != fatCur) {
//       fatCur = fatSec;
//       fatArea = area_map_ino(kSYS.mspace_, dir->dev_->underlyingDev_->dev_->ino_, fatSec * 512, 512, 0);
//       fat = (uint8_t *)fatArea->address_;
//     }

//     fatEntry = ((uint32_t*)fat)[fatOff];
//     if (fatEntry < 0xFFFFFFF) {
//       lba = SECTOR_OF(volume, fatEntry);
//       continue;
//     }

//     area_unmap(kSYS.mspace_, dirArea);
//     if (fatArea)
//       area_unmap(kSYS.mspace_, fatArea);
//     if (filename)
//       kfree(filename);
//     return ENOENT;
//   }

// }

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

static int PW2_UP(int val)
{
	while(!POW2(val))
	   val++;
	return val;
}

int fatfs_format(inode_t *ino)
{
    uint8_t *ptr = (uint8_t *)kmap(PAGE_SIZE, ino, 0, VMA_FILE_RW);
    struct BPB_Struct *bpb = (struct BPB_Struct *)ptr;
    struct BPB_Struct32 *bpb32 = (struct BPB_Struct32 *)ptr;

    int fatType;
    int clustSz;
    int clustCnt;
    if (ino->length < 6 * _Mib_) {
        fatType = 12;
        clustSz = PW2_UP(ino->length / 4096);
    } else if (ino->length < 256 * _Mib_) {
        fatType = 16;
        clustSz = PW2_UP(ino->length / 65525);
    } else {
        fatType = 32;
        clustSz = PW2_UP(ino->length / 0xFFFFFFFF);
    }
    
    clustSz = MIN(MAX(clustSz, 512), 8192);
    clustCnt = ino->length / clustSz;
    int fatSz = clustCnt / (ino->blk->block * 8 / fatType);

    bpb->BS_jmpBoot[0] = 0xEB;
    bpb->BS_jmpBoot[2] = 0x90;
    memcpy(bpb->BS_OEMName, "MSWIN4.1", 8);
    bpb->BPB_BytsPerSec = ino->blk->block; // Usually 512
    bpb->BPB_SecPerClus = clustSz / ino->blk->block;
    bpb->BPB_ResvdSecCnt = fatType == 32 ? 32 : 1;
    bpb->BPB_NumFATs = 2;
    // unsigned short  BPB_RootEntCnt;
    bpb->BPB_Media = 0xF8;
    // bpb->BPB_SecPerTrk = ; // Geometry dist
    // bpb->BPB_NumHeads; // Geometry
    // bpb->BPB_HiddSec; // Geometry

    unsigned rootLba = 0;
    if (fatType != 32) {
        bpb->BPB_TotSec16 = ino->length / ino->blk->block;
        bpb->BPB_TotSec32 = 0;
        bpb->BS_DrvNum = 0x80;
        bpb->BS_Reserved1 = 0;
        bpb->BS_BootSig = 0x29;
        // bpb->BS_VolID = time();
        memcpy(bpb->BS_VolLab, "NO NAME    ", 11);
        memcpy(bpb->BS_FilSysType, fatType == 12 ? "FAT12   " : "FAT16   ", 8);
        bpb->BPB_FATSz16 = fatSz;
        rootLba = bpb->BPB_ResvdSecCnt + (bpb->BPB_NumFATs * bpb->BPB_FATSz16);
    } else {
        bpb->BPB_TotSec16 = 0;
        bpb->BPB_TotSec32 = ino->length / ino->blk->block;
        // bpb32->BPB_FATSz32 = ;
        // bpb32->BPB_ExtFlags = ;
        bpb32->BPB_FSVer = 0;
        bpb32->BPB_RootClus = 2;
        bpb32->BPB_FSInfo = 1;
        bpb32->BPB_BkBootSec = 6;
        memset(bpb32->BPB_Reserved, 0, 12);

        bpb32->BS_DrvNum = 0x80;
        bpb32->BS_Reserved1 = 0;
        bpb32->BS_BootSig = 0x29;
        // bpb32->BS_VolID = time();
        memcpy(bpb32->BS_VolLab, "NO NAME    ", 11);
        memcpy(bpb32->BS_FilSysType, fatType == 12 ? "FAT12   " : "FAT16   ", 8);
        // rootLba = ...
    }

    // TODO Create FAT
    // TODO Create Root Directory ( . / .. )
    kunmap(ptr, PAGE_SIZE);
    return -1;
}

int fatfs_umount(FAT_inode_t *ino)
{
    // vfs_close(ino->vol->dev);
    kfree(ino->vol);
    return 0;
}

inode_t *fatfs_mount(inode_t *dev)
{
    if (dev == NULL) {
        errno = ENODEV;
        return NULL;
    }

    uint8_t *ptr = (uint8_t *)kmap(PAGE_SIZE, dev, 0, VMA_FILE_RO);
    struct BPB_Struct *bpb = (struct BPB_Struct *)ptr;
    struct BPB_Struct32 *bpb32 = (struct BPB_Struct32 *)ptr;
    if (bpb->BS_jmpBoot[0] != 0xE9 && !(bpb->BS_jmpBoot[0] == 0xEB &&
                                        bpb->BS_jmpBoot[2] == 0x90)) {
        kunmap(ptr, PAGE_SIZE);
        errno = EBADF;
        return NULL;
    }

    struct FAT_volume *info = (struct FAT_volume *)kalloc(sizeof(
                                  struct FAT_volume));
    info->RootDirSectors = ((bpb->BPB_RootEntCnt * 32) + (bpb->BPB_BytsPerSec -
                            1)) / bpb->BPB_BytsPerSec;
    info->FATSz = (bpb->BPB_FATSz16 != 0 ? bpb->BPB_FATSz16 : bpb32->BPB_FATSz32);
    info->FirstDataSector = bpb->BPB_ResvdSecCnt + (bpb->BPB_NumFATs *
                            info->FATSz) + info->RootDirSectors;
    info->TotSec = (bpb->BPB_TotSec16 != 0 ? bpb->BPB_TotSec16 :
                    bpb->BPB_TotSec32);
    info->DataSec = info->TotSec - (bpb->BPB_ResvdSecCnt +
                                    (bpb->BPB_NumFATs * info->FATSz) + info->RootDirSectors);
    if (info->FATSz == 0 || bpb->BPB_SecPerClus == 0 ||
        info->TotSec <= info->DataSec) {
        kfree(info);
        kunmap(ptr, PAGE_SIZE);
        errno = EBADF;
        return NULL;
    }

    info->CountofClusters = info->DataSec / bpb->BPB_SecPerClus;
    info->FATType = (info->CountofClusters < 4085 ? FAT12 :
                     (info->CountofClusters < 65525 ? FAT16 : FAT32));
    if (info->FATType == FAT16) {
        info->RootEntry = bpb->BPB_ResvdSecCnt + (bpb->BPB_NumFATs *
                          bpb->BPB_FATSz16);
    } else {
        info->RootEntry = ((bpb32->BPB_RootClus - 2) * bpb->BPB_SecPerClus) +
                          info->FirstDataSector;
    }
    info->SecPerClus = bpb->BPB_SecPerClus;
    info->ResvdSecCnt = bpb->BPB_ResvdSecCnt;
    info->BytsPerSec = bpb->BPB_BytsPerSec;

    strncpy(info->name, "UNNAMED", 48);
    info->totalSize = (long long)info->DataSec * 512;
    info->usedSpace = 0;
    info->freeSpace = 0;

    const char *fsName = info->FATType == FAT32 ? "fat32" :
                         (info->FATType == FAT16 ?
                          "fat16" : "fat12");
    FAT_inode_t *ino = (FAT_inode_t *)vfs_inode(info->RootEntry, S_IFDIR | 0755,
                       NULL, sizeof(FAT_inode_t));
    ino->ino.length = 0;
    // ino->ino.block = /*mount->SecPerClus */ mount->BytsPerSec;
    ino->ino.lba = info->RootEntry;
    ino->vol = info;
    // ino->vol->dev = vfs_open(dev);

    fsvolume_t *fs = (fsvolume_t *)kalloc(sizeof(fsvolume_t));
    // fs->lookup = (fs_lookup)isofs_lookup;
    // fs->read = (fs_read)isofs_read;
    fs->umount = (fs_umount)fatfs_umount;
    // fs->opendir = (fs_opendir)isofs_opendir;
    // fs->readdir = (fs_readdir)isofs_readdir;
    // fs->closedir = (fs_closedir)isofs_closedir;
    // fs->read_only = true;

    vfs_mountpt(info->name, fsName, fs, (inode_t *)ino);
    kunmap(ptr, PAGE_SIZE);
    return &ino->ino;
}



void fatfs_setup()
{
    register_fs("fatfs", (fs_mount)fatfs_mount);
}

void fatfs_teardown()
{
    unregister_fs("fatfs");
}

MODULE(fatfs, fatfs_setup, fatfs_teardown);

