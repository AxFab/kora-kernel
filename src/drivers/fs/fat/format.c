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
#include "fatfs.h"


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
        clustSz = POW2_UP(ino->length / 4096);
    } else if (ino->length < 256 * _Mib_) {
        fatType = 16;
        clustSz = POW2_UP(ino->length / 65525);
    } else {
        fatType = 32;
        clustSz = POW2_UP(ino->length / 0xFFFFFFFF);
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
    }

    // read volume info
    // fatfs_mkdir(info, lba);
    // fatfs_put_entry(info, inode_t, inode, name);
    // fatfs_alloc_cluster(int lba, size_t )
    struct FAT_volume *info = fatfs_init(ptr, ino);
    assert(info);
    fatfs_mkdir(info, NULL, info->RootEntry);
    kfree(info);

    // TODO Create FAT
    // TODO Create Root Directory ( . / .. )
    kunmap(ptr, PAGE_SIZE);
    return -1;
}
