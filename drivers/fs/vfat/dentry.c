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
#include <kernel/files.h>
#include "vfat.h"

void fatfs_settime(unsigned short *date, unsigned short *time, clock64_t value)
{
    time_t sec = value / _PwNano_;
    struct tm datetime;
    gmtime_r(&sec, &datetime);
    *date = (datetime.tm_mday & 0x1F) | (((datetime.tm_mon + 1) & 0xF) << 5) | ((datetime.tm_year - 80) << 9);
    *time = (datetime.tm_sec >> 1) | (datetime.tm_min << 5) | (datetime.tm_hour << 11);
}

clock64_t fatfs_gettime(unsigned short *date, unsigned short *time)
{
    struct tm datetime;
    memset(&datetime, 0, sizeof(datetime));
    datetime.tm_mday = (*date) & 0x1F;
    datetime.tm_mon = ((*date >> 5) & 0xF) - 1;
    datetime.tm_year = (*date >> 9) + 80;
    if (time) {
        datetime.tm_sec = (*time << 1) & 0x3F;
        datetime.tm_min = (*time >> 5) & 0x3F;
        datetime.tm_hour = (*time >> 11) & 0x1F;
    }
    time_t sec = mktime(&datetime);
    return sec * _PwNano_;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void fatfs_read_shortname(struct FAT_ShortEntry *entry, char *shortname)
{
    int lg = 7;
    memcpy(shortname, entry->DIR_Name, 8);
    while (lg > 0 && shortname[lg] == ' ')
        --lg;
    if (entry->DIR_Name[8] != ' ') {
        shortname[++lg] = '.';
        memcpy(&shortname[++lg], &entry->DIR_Name[8], 3);
        lg += 2;
        while (lg > 0 && shortname[lg] == ' ')
            --lg;
    }
    shortname[++lg] = '\0';
}

void fatfs_write_shortname(struct FAT_ShortEntry *entry, const char *shortname)
{
    char name[10] = { 0 };
    char ext[4] = { 0 };
    char *pext = strrchr(shortname, '.');
    int i = 9;
    if (pext != NULL) {
        strncpy(ext, pext + 1, 3);
        ext[3] = '\0';
        i = MIN(pext - shortname, i);
    }
    strncpy(name, shortname, i);
    name[i] = '\0';
    memset(entry->DIR_Name, ' ', 11);
    memcpy(entry->DIR_Name, name, MIN(8, strlen(name)));
    memcpy(&entry->DIR_Name[8], ext, MIN(3, strlen(ext)));
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

extern ino_ops_t fatfs_reg_ops;
extern ino_ops_t fatfs_dir_ops;

inode_t *fatfs_inode(int no, struct FAT_ShortEntry *entry, volume_t *volume, FAT_volume_t *info)
{
    unsigned cluster = (entry->DIR_FstClusHi << 16) | entry->DIR_FstClusLo;
    ftype_t type = FL_INVAL;
    if (entry->DIR_Attr & ATTR_DIRECTORY)
        type = FL_DIR;
    else if (entry->DIR_Attr & ATTR_ARCHIVE)
        type = FL_REG;

    inode_t *ino = vfs_inode(no, type, volume);
    ino->length = entry->DIR_FileSize;
    ino->lba = cluster;
    if (type == FL_REG)
        ino->info = map_create(ino, fatfs_read, fatfs_write);
    ino->atime = fatfs_gettime(&entry->DIR_LstAccDate, NULL);
    ino->ctime = fatfs_gettime(&entry->DIR_CrtDate, &entry->DIR_CrtTime);
    ino->mtime = fatfs_gettime(&entry->DIR_WrtDate, &entry->DIR_WrtTime);
    if (entry->DIR_Attr & ATTR_DIRECTORY)
        ino->ops = &fatfs_dir_ops;
    else if (entry->DIR_Attr & ATTR_ARCHIVE)
        ino->ops = &fatfs_reg_ops;
    return ino;
}

int fatfs_close(inode_t *ino)
{
    return 0;
}

void fatfs_short_entry(struct FAT_ShortEntry *entry, unsigned cluster, ftype_t type)
{
    memset(entry, 0, sizeof(*entry));
    if (type == FL_DIR)
        entry->DIR_Attr = ATTR_DIRECTORY;
    else if (type == FL_REG)
        entry->DIR_Attr = ATTR_ARCHIVE;

    fatfs_settime(&entry->DIR_CrtDate, &entry->DIR_CrtTime, kclock());
    entry->DIR_LstAccDate = entry->DIR_CrtDate;
    entry->DIR_WrtDate = entry->DIR_CrtDate;
    entry->DIR_WrtTime = entry->DIR_CrtTime;
    entry->DIR_FstClusHi = cluster >> 16;
    entry->DIR_FstClusLo = cluster & 0xFFFF;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


int fatfs_mkdir(FAT_volume_t *info, inode_t *dir)
{
    int lba = fatfs_alloc_cluster_16(info, -1);

    struct FAT_ShortEntry *entry = (struct FAT_ShortEntry *)bio_access(info->io_data_rw, lba);
    memset(entry, 0, info->BytsPerSec * info->SecPerClus);

    /* Create . and .. entries */
    fatfs_short_entry(entry, lba, FL_DIR);
    memcpy(entry->DIR_Name, FAT_DIRNAME_CURRENT, 11);
    ++entry;

    fatfs_short_entry(entry, dir->lba, FL_DIR);
    memcpy(entry->DIR_Name, FAT_DIRNAME_PARENT, 11);
    bio_clean(info->io_data_rw, lba);
    return lba;
}
