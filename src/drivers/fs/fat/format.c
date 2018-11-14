/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2018  <Fabien Bavent>
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
#include "fatfs.h"

uint8_t fatfs_noboot_code[] = {
    0x0e, 0x1f, 0xbe, 0x5b, 0x7c, 0xac, 0x22, 0xc0, 0x74,
    0x0b, 0x56, 0xb4, 0x0e, 0xbb, 0x07, 0x00, 0xcd, 0x10,
    0x5e, 0xeb, 0xf0, 0x32, 0xe4, 0xcd, 0x16, 0xcd, 0x19,
    0xeb, 0xfe, 0x54, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73,
    0x20, 0x6e, 0x6f, 0x74, 0x20, 0x61, 0x20, 0x62, 0x6f,
    0x6f, 0x74, 0x61, 0x62, 0x6c, 0x65, 0x20, 0x64, 0x69,
    0x73, 0x6b, 0x2e, 0x20, 0x20, 0x50, 0x6c, 0x65, 0x61,
    0x73, 0x65, 0x20, 0x69, 0x6e, 0x73, 0x65, 0x72, 0x74,
    0x20, 0x61, 0x20, 0x62, 0x6f, 0x6f, 0x74, 0x61, 0x62,
    0x6c, 0x65, 0x20, 0x66, 0x6c, 0x6f, 0x70, 0x70, 0x79,
    0x20, 0x61, 0x6e, 0x64, 0x0d, 0x0a, 0x70, 0x72, 0x65,
    0x73, 0x73, 0x20, 0x61, 0x6e, 0x79, 0x20, 0x6b, 0x65,
    0x79, 0x20, 0x74, 0x6f, 0x20, 0x74, 0x72, 0x79, 0x20,
    0x61, 0x67, 0x61, 0x69, 0x6e, 0x20, 0x2e, 0x2e, 0x2e,
    0x20, 0x0d, 0x0a,
};

struct disksize_secpercluster {
    size_t sectors;
    int cluster_size;
};

struct disksize_secpercluster dsk_table_fat16[] = {
    { 8400, 0 }, /* Up to 4.1 MB, error */
    { 32680, 2 }, /* Up to 16 MB, 1k cluster */
    { 262144, 4 }, /* Up to 128 MB, 2k cluster */
    { 524288, 8 }, /* Up to 256 MB, 4k cluster */
    { 1048576, 16 }, /* Up to 512 MB, 8k cluster */
    { 2097152, 32 }, /* Up to 1 GB, 16k cluster - best FAT 32 */
    { 4194304, 64 }, /* Up to 2 GB, 32k cluster - best FAT 32 */
    { ~0, 0 }, /* Larger than 2 GB, error */
};

struct disksize_secpercluster dsk_table_fat32[] = {
    { 66600, 0 }, /* Up to 32.5 MB, error */
    { 532480, 1 }, /* Up to 260 MB, 0.5k cluster */
    { 16777216, 8 }, /* Up to 8 GB, 4k cluster */
    { 33554432, 16 }, /* Up to 16 GB, 8k cluster */
    { 67108864, 32 }, /* Up to 32 GB, 16k cluster */
    { ~0, 64 }, /* Larger than 32 GB, 32k cluster */
};


int fatfs_format(inode_t *ino)
{
    assert(ino->type == FL_BLK);
    // TODO -- config
    const char *volume = "TestDrive";
    int cluster_size = 2048;
    int sec_size = ino->und.dev->block;

    bio_t *io_head = bio_create(ino, VMA_FILE_RW, sec_size, 0);
    uint8_t *ptr = bio_access(io_head, 0);
    memset(ptr, 0, sec_size);
    struct BPB_Struct *bpb = (struct BPB_Struct *)ptr;
    struct BPB_Struct32 *bpb32 = (struct BPB_Struct32 *)ptr;

    int cluster_count = ino->length / cluster_size;
    int fatType = FAT_TYPE(cluster_count);
    int fatSz = cluster_count / (sec_size * 8 / fatType);


    memcpy(bpb->BS_OEMName, "Kora FAT", 8);
    bpb->BPB_BytsPerSec = sec_size;
    bpb->BPB_SecPerClus = cluster_size / sec_size;
    bpb->BPB_ResvdSecCnt = fatType == 32 ? 32 : 1;
    bpb->BPB_NumFATs = 2;
    bpb->BPB_Media = 0xF8;

    // Disk Geometry
    bpb->BPB_SecPerTrk = 32;
    bpb->BPB_NumHeads = 64;
    bpb->BPB_HiddSec = 0;

    unsigned root_lba = 0;
    if (fatType != 32) {
        bpb->BPB_TotSec16 = (unsigned short)(ino->length / sec_size);
        bpb->BPB_TotSec32 = 0;
        bpb->BS_DrvNum = 0x80; // Get
        bpb->BS_Reserved1 = 0;
        bpb->BS_BootSig = 0x29;
        bpb->BPB_RootEntCnt = 512;
        bpb->BS_VolID = rand32();
        memset(bpb->BS_VolLab, ' ', 11);
        memcpy(bpb->BS_VolLab, volume, MIN(strlen(volume), 11));
        memcpy(bpb->BS_FilSysType, fatType == 12 ? "FAT12   " : "FAT16   ", 8);
        bpb->BPB_FATSz16 = fatSz;
        root_lba = bpb->BPB_ResvdSecCnt + (bpb->BPB_NumFATs * bpb->BPB_FATSz16);

        // Copy boot code
        bpb->BS_jmpBoot[0] = 0xEB;
        bpb->BS_jmpBoot[1] = 0x3C;
        bpb->BS_jmpBoot[2] = 0x90;
        memcpy(&ptr[0x3E], fatfs_noboot_code, sizeof(fatfs_noboot_code));

    } else {
        bpb->BPB_TotSec16 = 0;
        bpb->BPB_TotSec32 = ino->length / sec_size;
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
        bpb32->BS_VolID = rand32();
        memset(bpb->BS_VolLab, ' ', 11);
        memcpy(bpb->BS_VolLab, volume, MIN(strlen(volume), 11));
        memcpy(bpb32->BS_FilSysType, "FAT32   ", 8);
        // root_lba
        // boot code
    }

    ptr[510] = 0x55;
    ptr[511] = 0xaa;

    FAT_volume_t *info = fatfs_init(ptr);
    assert(info);

    // Write FAT16
    int i;
    unsigned j;
    for (i = 0; i < 2; ++i) {
        int lba = i * info->FATSz + 1; // TODO - resvd_sector_count;
        for (j = 0; j < info->FATSz; ++j) {
            uint16_t *fat_table = (uint16_t *)bio_access(io_head, lba + j);
            memset(fat_table, 0, sec_size);
            if (j == 0) {
                fat_table[0] = 0xfff8;
                fat_table[1] = 0xffff;
            }
            bio_clean(io_head, lba + j);
        }
    }

    assert(root_lba == info->RootEntry);
    int origin_sector = info->FirstDataSector - 2 * info->SecPerClus;
    bio_t *io_data = bio_create(ino, VMA_FILE_RW, info->BytsPerSec * info->SecPerClus, origin_sector * info->BytsPerSec);
    uint8_t *ptr_root = bio_access(io_data, 1);
    // ...
    struct FAT_ShortEntry *entry = (struct FAT_ShortEntry *)ptr_root;
    memset(entry, 0, sizeof(*entry));
    memset(entry->DIR_Name, ' ', 11);
    memcpy(entry->DIR_Name, volume, MIN(strlen(volume), 11));
    entry->DIR_Attr = ATTR_VOLUME_ID;
    fatfs_settime(&entry->DIR_CrtDate, &entry->DIR_CrtTime, time64());
    entry->DIR_WrtDate = entry->DIR_CrtDate;
    entry->DIR_WrtTime = entry->DIR_CrtTime;
    entry->DIR_LstAccDate = entry->DIR_CrtDate;

    memset(&entry[1], 0, sizeof(*entry));

    bio_clean(io_data, 0);
    bio_destroy(io_data);

    bio_clean(io_head, 0);
    bio_destroy(io_head);

    kfree(info);
    return 0;
}
