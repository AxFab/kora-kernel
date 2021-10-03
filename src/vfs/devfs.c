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
#include <kernel/stdc.h>
#include <kernel/vfs.h>
#include <threads.h>
#include <errno.h>
#include <assert.h>

/*
  The devfs is an mandatory embed file system that keep a trace of every registered
  devices.
 */

typedef struct dfs_info dfs_info_t;
typedef struct dfs_table dfs_table_t;
typedef struct dfs_entry dfs_entry_t;
typedef struct dfs_iterator dfs_iterator_t;

struct dfs_info {
    inode_t *root;
    dfs_table_t *table;
    xtime_t ctime;
};

struct dfs_entry {
    int ino;
    int flags;
    char uuid[40];
    char name[32];
    char label[32];
    int show;
    int filter;
    inode_t *dev;
};

struct dfs_table {
    dfs_table_t *next;
    int length;
    int free;
    dfs_entry_t entries[0];
};

struct dfs_iterator {
    dfs_entry_t *entry;
    int idx;
};

enum {
    DF_ROOT = (1 << 0),
    DF_BUS = (1 << 1),
    DF_DISK = (1 << 2),
    DF_INPUT = (1 << 3),
    DF_MOUNT = (1 << 4),
    DF_NET = (1 << 5),
    DF_SHM = (1 << 6),
    DF_DISK1 = (1 << 7),
    DF_DISK2 = (1 << 8),
    DF_MOUNT1 = (1 << 9),
    DF_MOUNT2 = (1 << 10),
    DF_CSTM1 = (1 << 11),
    DF_CSTM2 = (1 << 12),
    DF_CSTM3 = (1 << 13),
    DF_CSTM4 = (1 << 14),
    DF_CSTM5 = (1 << 15),
    DF_CSTM6 = (1 << 16),
    DF_CSTM7 = (1 << 17),
    DF_CSTM8 = (1 << 18),
};

enum dfs_flags {
    DF_FREE = 0,
    DF_BUSY = 1,
    DF_USED = 2,
    DF_USAGE = 3,

    DF_BY_UUID = 4,
    DF_BY_LABEL= 8
};


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
/* Gets and sets file entries */

/* Extends the devfs caching table from one memory page */
static dfs_table_t *devfs_extends(int first)
{
    int i;
    dfs_table_t *table = kmap(PAGE_SIZE, NULL, 0, VM_RW);
    memset(table, 0, PAGE_SIZE);
    int sz = (PAGE_SIZE - sizeof(dfs_table_t)) / sizeof(dfs_entry_t);
    table->length = sz;
    table->free = sz;
    for (i = 0; i < table->length; ++i)
        table->entries[i].ino = first + i;
    return table;
}

/* Get access to free entry from the devfs caching table */
static dfs_entry_t *devfs_fetch_new(dfs_info_t *info)
{
    int idx;
    dfs_table_t *table = info->table;
    for (idx = 0; ; ++idx) {
        if (table->free == 0 || idx >= table->length) {
            if (table->next == NULL)
                table->next = devfs_extends(table->entries[0].ino + table->length);
            table = table->next;
            idx = 0;
        }
        if ((table->entries[idx].flags & DF_USAGE) == DF_FREE) {
            table->entries[idx].flags |= DF_BUSY;
            return &table->entries[idx];
        }
    }
}

/* Get access to a known entry of the devfs caching table */
static dfs_entry_t *devfs_fetch(dfs_info_t *info, int no)
{
    dfs_table_t *table = info->table;
    while (no > table->length) {
        no -= table->length;
        table = table->next;
    }

    assert((table->entries[no - 1].flags & DF_USAGE) == DF_USED);
    return &table->entries[no - 1];
}

/* Use a new entry to create a new directory */
static int devfs_dir(dfs_info_t *info, const char *name, int parent, int filter, int show, int flg)
{
    dfs_entry_t *entry = devfs_fetch_new(info);
    // entry->prt = prt;
    (void)parent;
    entry->show = show;
    entry->filter = filter;
    strncpy(entry->name, name, 32);
    entry->flags &= ~DF_BUSY;
    entry->flags |= DF_USED | flg;
    return entry->ino;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
/* Directories of devfs */

extern ino_ops_t devfs_dir_ops;

/* Access or create the inode of a file entry */
static inode_t *devfs_inode(dfs_info_t *info, dfs_entry_t *entry)
{
    inode_t *ino;
    if (entry->dev) {
        ino = vfs_open_inode(entry->dev);
        ino->atime = xtime_read(XTIME_CLOCK);
        return ino;
    }
    ino = vfs_inode(entry->ino, FL_DIR, info->root->dev, &devfs_dir_ops);
    ino->drv_data = info;
    ino->ctime = info->ctime;
    ino->atime = info->ctime;
    ino->mtime = info->ctime;
    ino->btime = info->ctime;
    return ino;
}

/* Look for entry visible on a directory */
inode_t *devfs_open(inode_t *dir, const char *name, ftype_t type, void *acl, int flags)
{
    dfs_info_t *info = dir->drv_data;
    if ((flags & (IO_OPEN | IO_CREAT)) == IO_CREAT) {
        errno = EROFS;
        return NULL;
    }
    dfs_entry_t *entry = devfs_fetch(info, dir->no);

    int idx;
    dfs_table_t *table = info->table;
    for (idx = 0; ; ++idx) {
        if (idx >= table->length) {
            idx = 0;
            table = table->next;
            if (table == NULL) {
                errno = ENOENT;
                return NULL;
            }
        }

        dfs_entry_t *en = &table->entries[idx];
        if ((en->show & entry->filter) == 0)
            continue;

        int k = 0;
        if (entry->flags & DF_BY_UUID)
            k = strcmp(en->uuid, name);
        else if (entry->flags & DF_BY_LABEL)
            k = strcmp(en->label, name);
        else
           k = strcmp(en->name, name);
        if (k == 0)
            return devfs_inode(info, en);
    }
    return NULL;
}

inode_t* devfs_lookup(inode_t* dir, const char* name, void* acl)
{
    dfs_info_t* info = dir->drv_data;
    dfs_entry_t* entry = devfs_fetch(info, dir->no);

    // TODO Look on name / label or uuid ?
    int idx;
    dfs_table_t* table = info->table;
    for (idx = 0; ; ++idx) {
        if (idx >= table->length) {
            idx = 0;
            table = table->next;
            if (table == NULL) {
                errno = ENOENT;
                return NULL;
            }
        }

        dfs_entry_t* en = &table->entries[idx];
        if ((en->show & entry->filter) == 0)
            continue;

        int k = strcmp(en->name, name);
        if (k == 0)
            return devfs_inode(info, en);
    }
    return NULL;
}

/* Start an iterator to walk on a directory */
dfs_iterator_t *devfs_opendir(inode_t *dir)
{
    dfs_info_t *info = dir->drv_data;
    dfs_entry_t *entry = devfs_fetch(info, dir->no);

    dfs_iterator_t *it = kalloc(sizeof(dfs_iterator_t));
    it->entry = entry;
    it->idx = 0;
    return it;
}

/* Start iterate on a directory */
inode_t *devfs_readdir(inode_t *dir, char *name, dfs_iterator_t *ctx)
{
    dfs_info_t *info = dir->drv_data;
    dfs_table_t *table = info->table;
    int no = ctx->idx;
    while (no > table->length) {
        no -= table->length;
        if (table->next == NULL)
            return NULL;
        table = table->next;
    }

    for (;; ++no) {
        if (no > table->length) {
            table = table->next;
            if (table == NULL)
                return NULL;
            no = 0;
        }
        ctx->idx++;

        dfs_entry_t *entry = &table->entries[no];
        if ((entry->flags & DF_USAGE) != DF_USED)
            continue;
        if ((entry->show & ctx->entry->filter) == 0)
            continue;

        if (ctx->entry->flags & DF_BY_UUID)
            strcpy(name, entry->uuid);
        else if (ctx->entry->flags & DF_BY_LABEL)
            strcpy(name, entry->label);
        else
            strcpy(name, entry->name);

        return devfs_inode(info, entry);
    }
}

/* Close directory iterator */
void devfs_closedir(inode_t *dir, dfs_iterator_t *ctx)
{
    (void)dir;
    kfree(ctx);
}


ino_ops_t devfs_dir_ops = {
    // .open = devfs_open,
    .lookup = devfs_lookup,
    .opendir = (void *)devfs_opendir,
    .readdir = (void *)devfs_readdir,
    .closedir = (void *)devfs_closedir,
};


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
/* Bytes devices */

/* Read opertion of '/dev/null */
int null_read(inode_t *ino, char *buf, size_t len, xoff_t off, int flags)
{
    (void)ino;
    (void)off;
    if (flags & IO_NOBLOCK) {
        errno = EWOULDBLOCK;
        return 0;
    }

    mtx_t mtx;
    mtx_init(&mtx, mtx_plain);
    for (;;)
        mtx_lock(&mtx);
}

/* Write opertion of '/dev/null */
int null_write(inode_t *ino, const char *buf, size_t len, xoff_t off, int flags)
{
    (void)ino;
    (void)buf;
    (void)off;
    (void)flags;
    errno = 0;
    return len;
}

/* Read opertion of '/dev/zero */
int zero_read(inode_t *ino, char *buf, size_t len, xoff_t off, int flags)
{
    (void)ino;
    (void)off;
    memset(buf, 0, len);
    errno = 0;
    return len;
}

/* Write opertion of '/dev/random */
int rand_read(inode_t *ino, char *buf, size_t len, xoff_t off, int flags)
{
    (void)ino;
    (void)off;
    size_t i;
    for (i = 0; i < len; ++i)
        buf[i] = rand8();
    errno = 0;
    return len;
}

ino_ops_t devfs_zero_ops = {
    .read = zero_read,
};

ino_ops_t devfs_null_ops = {
    .read = null_read,
    .write = null_write,
};

ino_ops_t devfs_rand_ops = {
    .read = rand_read,
};

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

inode_t *DEV_INO;

static inode_t *devfs_dev(dfs_info_t *info, const char *name, int type, int show, ino_ops_t *ops)
{
    dfs_entry_t *entry = devfs_fetch_new(info);
    inode_t *ino = vfs_inode(entry->ino, type, NULL, ops);
    // info->dev->devname = strdup(name);
    ino->dev->devclass = strdup("Bytes device");
    entry->show = show;
    entry->dev = ino;
    strncpy(entry->name, name, 32);
    entry->flags &= ~DF_BUSY;
    entry->flags |= DF_USED;
    kprintf(KL_MSG, "%s %s <\033[33m%s\033[0m>\n", ino->dev->devclass,
            ino->dev->model ? ino->dev->model : "", name);

    return ino;
}

void devfs_register(inode_t *ino, const char *name)
{
    // char tmp[12];
    dfs_info_t *info = DEV_INO->drv_data;
    dfs_entry_t *entry = devfs_fetch_new(info);
    switch (ino->type) {
    case FL_DIR:
        entry->show = DF_MOUNT;
        if (ino->dev->uuid[0] != '0')
            entry->show |= DF_MOUNT1;
        if (ino->dev->devname != NULL)
            entry->show |= DF_MOUNT2;
        kprintf(KL_MSG, "Mount drive as \033[35m%s\033[0m (%s)\n", ino->dev->devname, ino->dev->devclass);
        break;
    case FL_BLK:
        entry->show = DF_DISK | DF_ROOT;
        if (ino->dev->uuid[0] != '0') {
            snprintf(entry->uuid, 40, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                ino->dev->uuid[0], ino->dev->uuid[1], ino->dev->uuid[2], ino->dev->uuid[3],
                ino->dev->uuid[4], ino->dev->uuid[5], ino->dev->uuid[6], ino->dev->uuid[7],
                ino->dev->uuid[8], ino->dev->uuid[9], ino->dev->uuid[10], ino->dev->uuid[11],
                ino->dev->uuid[12], ino->dev->uuid[13], ino->dev->uuid[14], ino->dev->uuid[15]);
            entry->show |= DF_DISK1;
        }
        if (ino->dev->devname != NULL) {
            strncpy(entry->label, ino->dev->devname, 32);
            entry->show |= DF_DISK2;
        }
        if (ino->length)
            kprintf(KL_MSG, "%s %s %s <\033[33m%s\033[0m>\n", ino->dev->devclass,
                    ino->dev->model ? ino->dev->model : "", sztoa(ino->length), name);
        else
            kprintf(KL_MSG, "%s %s <\033[33m%s\033[0m>\n", ino->dev->devclass,
                    ino->dev->model ? ino->dev->model : "", name);
        break;
    default:
        entry->show = DF_ROOT;
        kprintf(KL_MSG, "%s %s <\033[33m%s\033[0m>\n", ino->dev->devclass,
                ino->dev->model ? ino->dev->model : "", name);
        break;
    }

    // kprintf(-1, "Device %s\n", vfs_inokey(ino, tmp));
    entry->dev = vfs_open_inode(ino);
    strncpy(entry->name, name, 32);
    entry->flags &= ~DF_BUSY;
    entry->flags |= DF_USED;
}

inode_t* devfs_mount(inode_t *dev, const char* options)
{
    if (dev != NULL)
        return NULL;

    return vfs_open_inode(DEV_INO);
}

inode_t *devfs_setup()
{
    dfs_info_t *info = kalloc(sizeof(dfs_info_t));
    info->ctime = xtime_read(XTIME_CLOCK);
    info->table = devfs_extends(1);

    DEV_INO = vfs_inode(1, FL_DIR, NULL, &devfs_dir_ops);
    DEV_INO->drv_data = info;
    info->root = DEV_INO;


    dfs_entry_t *entry = devfs_fetch_new(info);
    entry->dev = vfs_open_inode(DEV_INO);
    entry->filter = DF_ROOT;
    entry->flags = DF_USED;

    int bus = devfs_dir(info, "bus", 1, DF_BUS, DF_ROOT, 0);
    int dsk = devfs_dir(info, "disk", 1, DF_DISK, DF_ROOT, 0);
    devfs_dir(info, "input", 1, DF_INPUT, DF_ROOT, 0);
    int mnt = devfs_dir(info, "mnt", 1, DF_MOUNT, DF_ROOT, 0);
    devfs_dir(info, "net", 1, DF_NET, DF_ROOT, 0);
    devfs_dir(info, "shm", 1, DF_SHM, DF_ROOT, 0);

    devfs_dir(info, "by-uuid", dsk, DF_DISK1, DF_DISK, DF_BY_UUID);
    devfs_dir(info, "by-label", dsk, DF_DISK2, DF_DISK, DF_BY_LABEL);

    devfs_dir(info, "by-uuid", mnt, DF_MOUNT1, DF_MOUNT, DF_BY_UUID);
    devfs_dir(info, "by-label", mnt, DF_MOUNT2, DF_MOUNT, DF_BY_LABEL);

    devfs_dir(info, "pci", bus, DF_CSTM1, DF_BUS, 0);
    devfs_dir(info, "usb", bus, DF_CSTM2, DF_BUS, 0);

    devfs_dev(info, "zero", FL_CHR, DF_ROOT, &devfs_zero_ops);
    devfs_dev(info, "null", FL_CHR, DF_ROOT, &devfs_null_ops);
    devfs_dev(info, "random", FL_CHR, DF_ROOT, &devfs_rand_ops);

    vfs_addfs("devfs", devfs_mount);

    return vfs_open_inode(DEV_INO);
}

void devfs_sweep()
{
    int i;
    dfs_info_t *info = DEV_INO->drv_data;
    dfs_table_t *table = info->table;
    while (table) {
        for (i = 0; i < table->length; ++i) {
            dfs_entry_t *entry = &table->entries[i];
            if ((entry->flags & DF_USAGE) == 0)
                continue;
            vfs_close_inode(entry->dev);
        }

        dfs_table_t *p = table;
        table = table->next;
        kunmap(p, PAGE_SIZE);
    }

    info->table = NULL;
    vfs_close_inode(DEV_INO);
    DEV_INO = NULL;
    kfree(info);
}


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

// Currently in testing

unsigned entrop_k = 0;
unsigned entrop_arr[2];
unsigned entrop_T0 = 0; // time() % 4219;

void vfs_entropy(unsigned value)
{
    unsigned v = (entrop_k++) & 1;
    entrop_arr[v] = value;
    if (v) {
        unsigned T1 = entrop_arr[0] % 2521;
        unsigned T2 = entrop_arr[1] % 2339;

        unsigned S1 = (entrop_T0 * 3257 + T1 * 5023) % 2833;
        unsigned S2 = (entrop_T0 * 4051 + T2 * 4643) % 3923;

        entrop_T0 = (S1 * S2 * 27737 + 11831) % 4219;
        unsigned R0 = (S1 * S2 * 41263 + 11971) % 65729;
        ((void)R0);
        // pipe_write( , &R0, 2, IO_NO_BLOCK);
    }
}

