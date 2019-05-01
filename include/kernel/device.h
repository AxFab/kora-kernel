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
#ifndef _KERNEL_DRIVERS_H
#define _KERNEL_DRIVERS_H 1

#include <kernel/core.h>
#include <kernel/vfs.h>
#include <kora/splock.h>
#include <kora/rwlock.h>
#include <kora/hmap.h>
#include <time.h>

typedef inode_t *(*fs_mount)(inode_t *dev);
// typedef int (*fs_umount)(inode_t *ino);
typedef void (*fs_umount)(volume_t *vol);
// typedef void (*fs_release_dev)(device_t *dev);

typedef int (*fs_read)(inode_t *ino, void *buf, size_t len, off_t off);
typedef int (*fs_write)(inode_t *ino, const void *buf, size_t len, off_t off);

typedef inode_t *(*fs_open)(inode_t *dir, CSTR name, int mode, acl_t *acl, int flags);
typedef int (*fs_unlink)(inode_t *dir, CSTR name);

typedef int (*fs_truncate)(inode_t *ino, off_t length);

typedef void *(*fs_opendir)(inode_t *ino);
typedef inode_t *(*fs_readdir)(inode_t *ino, char *name, void *ctx);
typedef int (*fs_closedir)(inode_t *ino, void *ctx);

typedef int (*blk_read)(inode_t *ino, void *buf, size_t len, off_t off);
typedef int (*blk_write)(inode_t *ino, const void *buf, size_t len, off_t off);
typedef int (*chr_write)(inode_t *ino, const void *buf, size_t len);

typedef int (*dev_rmdev)(inode_t *ino, void *ops);
typedef int (*dev_ioctl)(inode_t *ino, int cmd, void *params);

// typedef int (*net_send)(netdev_t *ifnet, skb_t *skb);
// typedef int (*net_link)(netdev_t *ifnet);

// typedef int (*vds_flip)(inode_t *ino);
// typedef int (*vds_resize)(inode_t *ino, int *width, int *height);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

struct device {
    uint8_t id[16];
    unsigned flags;
    char *devname;
    char *devclass;
    dev_ops_t *ops;
    char *vendor;
    char *model;
    int block;
    void *info; // Place holder for driver info
};

struct volume {
    uint8_t id[16];
    unsigned flags;
    char *volname;
    char *volfs;
    fs_ops_t *ops;
    inode_t *dev;
    atomic_int rcu;
    llhead_t lru;
    HMP_map hmap;
    bbtree_t btree;
    rwlock_t brwlock;
    splock_t lock;
    void *info; // Place holder for driver info
};



struct dev_ops {
    int(*ioctl)(inode_t *ino, int cmd, void *params);
};

struct net_ops {
    void(*link)(inode_t *ifnet);
    int(*send)(inode_t *ifnet, skb_t *skb);
};

struct fs_ops {
    inode_t *(*open)(inode_t *dir, const char *name, ftype_t type, acl_t *acl, int flags);
    void(*chacl)(inode_t *ino, acl_t *acl);
    void(*utimes)(inode_t *ino, clock64_t time, int flags);
    int(*link)(inode_t *ino, inode_t *dir, const char *name);
    int(*unlink)(inode_t *dir, const char *name);
    int(*rename)(inode_t *ino, inode_t *dir, const char *name);
    void (*umount)(volume_t *vol);
};

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

/* Allocate a new inode and set some mandatory fields */
inode_t *vfs_inode(unsigned no, ftype_t type, volume_t *volume);

int vfs_mkdev(inode_t *ino, CSTR name);
void vfs_rmdev(CSTR name);
inode_t *vfs_search_device(CSTR name);
void vfs_show_devices();

void register_fs(CSTR, fs_mount mount);
void unregister_fs(CSTR name);

inode_t *vfs_mount(CSTR dev, CSTR fs);
int vfs_umount(inode_t *ino);

#endif  /* _KERNEL_DRIVERS_H */
