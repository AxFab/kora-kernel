/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2021  <Fabien Bavent>
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
#include <errno.h>

#define TAR_BLOCK_SIZE  512
typedef struct tar_entry tar_entry_t;
typedef struct tar_info tar_info_t;
typedef struct tar_iterator tar_iterator_t;

struct tar_entry {
    char name[100];
    char filemode[8];
    char user_id[8];
    char group_id[8];
    char file_size[12];
    char last_mode_time[12];
    char checksum[8];
    char type_flag[1];
    char symlink[100];
    char magik[6];
    char version[2];
    char owner_user[32];
    char owner_group[32];
    char dev_major[8];
    char dev_minor[8];
    char filename_prefix[155];
};

struct tar_info {
    void *base;
    size_t length;
    atomic_int rcu;
};

struct tar_iterator {
    unsigned idx;
    int len;
    char prefix[100];
};

static int tar_read_octal(char *count)
{
    int i;
    int val = 0;
    for (i = 0; count[i] != '\0'; ++i) {
        val *= 8;
        val += count[i] - '0';
    }
    return val;
}

static inode_t *tar_inode(inode_t *dir, tar_entry_t *entry);
void tar_close(inode_t *ino);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

static char *tar_strrchr(const char *str)
{
    int lg = strlen(str) - 1;
    if (str[lg] == '/')
        --lg;
    for (; lg >= 0; --lg) {
        if (str[lg] == '/')
            return (char *)&str[lg + 1];
    }

    return NULL;
}

static void tar_start_iterate(inode_t *dir, tar_iterator_t *ctx)
{
    tar_info_t *info = dir->drv_data;
    if (dir->no >= 2) {
        tar_entry_t *entry = ADDR_OFF(info->base, (dir->no - 2) * TAR_BLOCK_SIZE);
        strncpy(ctx->prefix, entry->name, 100);
        ctx->len = strlen(ctx->prefix);
    } else
        memset(ctx, 0, sizeof(tar_iterator_t));
    ctx->idx = 0;
}

static tar_entry_t *tar_do_iterate(inode_t *dir, char *name, tar_iterator_t *ctx)
{
    tar_entry_t *entry;
    tar_info_t *info = dir->drv_data;
    char *pfx = NULL;
    for (;;) {
        if (ctx->idx * TAR_BLOCK_SIZE > info->length)
            return NULL;
        entry = ADDR_OFF(info->base, ctx->idx * TAR_BLOCK_SIZE);
        if (memcmp("ustar ", entry->magik, 6) != 0)
            return NULL;
        int length = tar_read_octal(entry->file_size);
        ctx->idx += ALIGN_UP(length + TAR_BLOCK_SIZE, TAR_BLOCK_SIZE) / TAR_BLOCK_SIZE;

        pfx = tar_strrchr(entry->name);
        if (pfx == NULL && ctx->prefix[0] == '\0')
            break;
        if (pfx == NULL || ctx->prefix[0] == '\0')
            continue;
        int slg = pfx - entry->name;
        if (slg != ctx->len)
            continue;
        int c = memcmp(ctx->prefix, entry->name, ctx->len);
        if (c == 0)
            break;
    }
    if (pfx == NULL)
        strncpy(name, entry->name, 255);
    else
        strncpy(name, pfx, 255);
    char *n = strrchr(name, '/');
    if (n)
        *n = '\0';
    return entry;
}

/* Look for entry visible on a directory */
inode_t *tar_lookup(inode_t *dir, const char *name, void *acl)
{
    tar_iterator_t ctx;
    tar_start_iterate(dir, &ctx);
    char tmp[256];
    for (;;) {
        tar_entry_t *entry = tar_do_iterate(dir, tmp, &ctx);
        if (entry == NULL) {
            errno = ENOENT;
            return NULL;
        }

        if (strcmp(name, tmp) == 0)
            return tar_inode(dir, entry);
    }
}

/* Start an iterator to walk on a directory */
tar_iterator_t *tar_opendir(inode_t *dir)
{
    tar_iterator_t *ctx = kalloc(sizeof(tar_iterator_t));
    tar_start_iterate(dir, ctx);
    return ctx;
}


/* Start iterate on a directory */
inode_t *tar_readdir(inode_t *dir, char *name, tar_iterator_t *ctx)
{
    tar_entry_t *entry = tar_do_iterate(dir, name, ctx);
    if (entry == NULL)
        return NULL;

    inode_t *ino = tar_inode(dir, entry);
    return ino;
}

/* Close directory iterator */
void tar_closedir(inode_t *dir, tar_iterator_t *ctx)
{
    kfree(ctx);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int tar_read(inode_t *ino, char *buf, size_t len, xoff_t off, int flags)
{
    tar_info_t *info = ino->drv_data;
    tar_entry_t *entry = ADDR_OFF(info->base, (ino->no - 2) * TAR_BLOCK_SIZE);
    uint8_t *data = ADDR_OFF(entry, TAR_BLOCK_SIZE);
    if ((xoff_t)len + off > ino->length) {
        if ((xoff_t)len > ALIGN_UP(ino->length, PAGE_SIZE))
            return -1;
        memset(buf, 0, len);
        memcpy(buf, &data[off], (size_t)(ino->length - off));
    } else
        memcpy(buf, &data[off], len);

    return 0;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

ino_ops_t tar_dir_ops = {
    .close = tar_close,
    .lookup = tar_lookup,
    .opendir = (void *)tar_opendir,
    .readdir = (void *)tar_readdir,
    .closedir = (void *)tar_closedir,
};

ino_ops_t tar_reg_ops = {
    .close = tar_close,
    .read = tar_read,
};

ino_ops_t tar_lnk_ops = {
    .close = tar_close,
    .lookup = tar_lookup,
};

static inode_t *tar_inode(inode_t *dir, tar_entry_t *entry)
{
    tar_info_t *info = dir->drv_data;
    int lba = ((char *)entry - (char *)info->base);
    lba = lba / TAR_BLOCK_SIZE + 2;

    inode_t *ino = NULL;
    if (entry->type_flag[0] == '0')
        ino = vfs_inode(lba, FL_REG, dir->dev, &tar_reg_ops);
    else if (entry->type_flag[0] == '2')
        ino = vfs_inode(lba, FL_LNK, dir->dev, &tar_lnk_ops);
    else if (entry->type_flag[0] == '5')
        ino = vfs_inode(lba, FL_DIR, dir->dev, &tar_dir_ops);

    if (ino == NULL)
        return NULL;

    if (ino->rcu == 1)
        atomic_inc(&info->rcu);

    ino->length = tar_read_octal(entry->file_size);
    ino->mode = tar_read_octal(entry->filemode);
    ino->drv_data = dir->drv_data;
    int mtime = tar_read_octal(entry->last_mode_time);
    ino->atime = SEC_TO_USEC(mtime);
    ino->mtime = SEC_TO_USEC(mtime);
    ino->ctime = SEC_TO_USEC(mtime);
    ino->btime = SEC_TO_USEC(mtime);
    return ino;
}

void tar_close(inode_t *ino)
{
    tar_info_t *info = ino->drv_data;
    if (atomic_xadd(&info->rcu, -1) == 1) {
        // kfree(info->base); -- TODO flags ownership of memory
        kfree(info);
    }
}

inode_t *tar_mount(void *base, size_t length, const char *name)
{
    tar_info_t *info = kalloc(sizeof(tar_info_t));
    info->rcu = 1;
    info->base = base;
    info->length = length;
    inode_t *ino = vfs_inode(1, FL_DIR, NULL, &tar_dir_ops);
    ino->lba = 0;
    ino->mode = 0755;
    ino->drv_data = info;
    ino->dev->devname = name != NULL && *name != '\0' ? kstrdup(name) : NULL;
    ino->dev->devclass = kstrdup("tar");
    return ino;
}
