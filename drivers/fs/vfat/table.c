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


FAT_volume_t *fatfs_init(void *ptr)
{
    struct BPB_Struct *bpb = (struct BPB_Struct *)ptr;
    struct BPB_Struct32 *bpb32 = (struct BPB_Struct32 *)ptr;
    if (bpb->BS_jmpBoot[0] != 0xE9 && !(bpb->BS_jmpBoot[0] == 0xEB &&
                                        bpb->BS_jmpBoot[2] == 0x90))
        return NULL;

    FAT_volume_t *info = (FAT_volume_t *)kalloc(sizeof(FAT_volume_t));
    info->RootDirSectors = ((bpb->BPB_RootEntCnt * 32) + (bpb->BPB_BytsPerSec - 1)) / bpb->BPB_BytsPerSec;
    info->FATSz = (bpb->BPB_FATSz16 != 0 ? bpb->BPB_FATSz16 : bpb32->BPB_FATSz32);
    info->FirstDataSector = bpb->BPB_ResvdSecCnt + (bpb->BPB_NumFATs * info->FATSz) + info->RootDirSectors;
    // FirstSectorofCluster = ((N – 2) * BPB_SecPerClus) + FirstDataSector;
    info->TotSec = (bpb->BPB_TotSec16 != 0 ? bpb->BPB_TotSec16 : bpb->BPB_TotSec32);
    info->DataSec = info->TotSec - (bpb->BPB_ResvdSecCnt + (bpb->BPB_NumFATs * info->FATSz) + info->RootDirSectors);
    if (info->FATSz == 0 || bpb->BPB_SecPerClus == 0 || info->TotSec <= info->DataSec) {
        kfree(info);
        return NULL;
    }

    info->CountofClusters = info->DataSec / bpb->BPB_SecPerClus;
    info->FATType = FAT_TYPE(info->CountofClusters);
    if (info->FATType == FAT16)
        info->RootEntry = bpb->BPB_ResvdSecCnt + (bpb->BPB_NumFATs * bpb->BPB_FATSz16);
    else
        info->RootEntry = ((bpb32->BPB_RootClus - 2) * bpb->BPB_SecPerClus) + info->FirstDataSector;
    info->SecPerClus = bpb->BPB_SecPerClus;
    info->ResvdSecCnt = bpb->BPB_ResvdSecCnt;
    info->BytsPerSec = bpb->BPB_BytsPerSec;

    int lg = 11;
    memcpy(info->name, bpb->BS_VolLab, lg);
    while (lg > 0 && info->name[--lg] == ' ')
        info->name[lg] = '\0';
    if (lg == 0)
        strncpy(info->name, "UNNAMED", 48);

    info->totalSize = (long long)info->DataSec * 512;
    info->usedSpace = 0;
    info->freeSpace = 0;
    return info;
}

void fatfs_reserve_cluster_16(inode_t *bdev, FAT_volume_t *info, int cluster, int previous)
{
    int i;
    for (i = 0; i < 2; ++i) {
        unsigned lba = i * info->FATSz + 1;
        // int fat_bytes = ALIGN_UP(info->FATSz * info->BytsPerSec, PAGE_SIZE);
        uint16_t *fat_table = kmap(PAGE_SIZE, bdev, (xoff_t)lba * info->BytsPerSec, VM_RW); // (uint16_t *)bio_access(info->io_head, lba);
        // TODO - Load next pages
        if (previous > 0)
            fat_table[previous] = cluster;
        fat_table[cluster] = 0xFFFF;
        kunmap(fat_table, PAGE_SIZE);
        //bio_clean(info->io_head, lba);
    }
}

unsigned fatfs_alloc_cluster_16(inode_t *bdev, FAT_volume_t *info, int previous)
{
    unsigned i;
    unsigned lba = 1; // resvd_sector_count
    unsigned fat_bytes = ALIGN_UP(info->FATSz * info->BytsPerSec, PAGE_SIZE);
    uint16_t *fat_table = kmap(PAGE_SIZE, bdev, (xoff_t)lba * info->BytsPerSec, VM_RW); // (uint16_t *)bio_access(info->io_head, lba);
    // TODO - Load next pages
    for (i = 0; i < fat_bytes / 2; ++i) {
        if (fat_table[i] == 0) {
            fatfs_reserve_cluster_16(bdev, info, i, previous);
            kunmap(fat_table, PAGE_SIZE);
            //bio_clean(info->io_head, lba);
            return i;
        }
    }
    kunmap(fat_table, PAGE_SIZE);
    //bio_clean(info->io_head, lba);
    return -1; // No space available
}

unsigned fat_cluster_next_16(inode_t* bdev, FAT_volume_t* info, unsigned clustNo)
{
    unsigned lba = info->ResvdSecCnt;
    unsigned fat_bytes = ALIGN_UP(info->FATSz * info->BytsPerSec, PAGE_SIZE);
    
    size_t doff = lba * info->BytsPerSec + clustNo * sizeof(uint16_t);
    int add = doff / PAGE_SIZE;
    int off = doff % PAGE_SIZE;

    void *tmap = kmap(PAGE_SIZE, bdev, (xoff_t)add * PAGE_SIZE, VM_RD);
    uint16_t* fat_table = ADDR_OFF(tmap, off);
    uint16_t val = fat_table[0];
    kunmap(tmap, PAGE_SIZE);
    if (val == 0xffff)
        return 0;
    return val;
}


