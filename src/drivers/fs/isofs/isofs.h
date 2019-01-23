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
#ifndef _SRC_ISOFS_H
#define _SRC_ISOFS_H 1

#include <kernel/device.h>
#include <string.h>
#include <errno.h>

#define FILENAME_MAX 255

/* Identificators for volume descriptors */
#define ISOFS_STD_ID1    0x30444300
#define ISOFS_STD_ID2    0x00003130
#define ISOFS_VOLDESC_BOOT 0
#define ISOFS_VOLDESC_PRIM 1
#define ISOFS_VOLDESC_SUP  2
#define ISOFS_VOLDESC_VOL  3
#define ISOFS_VOLDESC_TERM 255

typedef struct ISOFS_entry ISOFS_entry_t;
typedef struct ISOFS_descriptor ISOFS_descriptor_t;

#define ISOFS_nextEntry(e)  ((ISOFS_entry_t*)&(((char*)(e))[(e)->lengthRecord]));

/* Structure of a Directory entry in isofs FS */
PACK(struct ISOFS_entry {
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


/* Structure of the Primary volume descriptor in isofs FS */
PACK(struct ISOFS_descriptor {
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
    ISOFS_entry_t rootDir;
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


typedef struct ISO_info ISO_info_t;
typedef struct ISO_dirctx ISO_dirctx_t;

struct ISO_info {
    time_t created;
    char bootable;
    int lbaroot;
    int lgthroot;
    int sectorCount;
    int sectorSize;
    char name[128];
    bio_t *io;
};

struct ISO_dirctx {
    size_t off;
    int lba;
    uint8_t *base;
};



inode_t *isofs_readdir(inode_t *dir, char *name, ISO_dirctx_t *ctx);

inode_t *isofs_mount(inode_t *dev);
int isofs_umount(inode_t *ino);
int isofs_read(inode_t *ino, void *buffer, size_t length, off_t offset);

#endif  /* _SRC_ISOFS_H */
