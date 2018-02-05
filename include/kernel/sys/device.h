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
 */
#ifndef _KERNEL_SYS_DEVICE_H
#define _KERNEL_SYS_DEVICE_H 1

#include <kernel/core.h>
#include <kora/llist.h>
#include <kora/splock.h>
#include <kora/hmap.h>
#include <kernel/sys/inode.h>

typedef struct device device_t;
typedef struct mountpt mountpt_t;

struct device {

    char id[16];
    const char *vendor;
    const char *class;
    const char *device;
    const char *name;
    inode_t *ino;
    llnode_t node;

    int irq;
};

struct mountpt {
    const char *name;
    const char *fs;
    inode_t *ino;
    llnode_t node;
    splock_t lock;
    HMP_map hmap;
    // FS_ops
};

int vfs_mkdev(const char *name, inode_t *ino, const char *vendor,
              const char *class, const char *device, unsigned char id[16]);

inode_t *vfs_mountpt(int no, const char *name, const char *fs, size_t size);

inode_t *vfs_lookup_device(const char *name);
// inode_t *vfs_lookup_mountpt(const char *name);


void register_filesystem(const char *name, vfs_fs_ops_t *ops);
void unregister_filesystem(const char *name);
vfs_fs_ops_t *vfs_fs(const char *name);

#endif  /* _KERNEL_SYS_DEVICE_H */
