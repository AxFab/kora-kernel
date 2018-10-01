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
 */
#ifndef _KERNEL_DRIVERS_H
#define _KERNEL_DRIVERS_H 1

#include <kernel/core.h>
#include <kernel/vfs.h>
#include <kora/splock.h>
#include <kora/hmap.h>
#include <time.h>

typedef inode_t *(*fs_mount)(inode_t *dev);
typedef int (*fs_umount)(inode_t *ino);
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

typedef int (*net_send)(netdev_t *ifnet, skb_t *skb);
typedef int (*net_link)(netdev_t *ifnet);

typedef int (*vds_flip)(inode_t *ino);
typedef int (*vds_resize)(inode_t *ino, int *width, int *height);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

// union {
//     void *object;
//     void *pcache;
//     mountpt_t *mnt;
//     void *chard;
//     fifo_t *fifo;
//     void *slink;
//     void *socket;
//     surface_t *win;
//     terminal_t *term;
// };

struct device {
    bool read_only;
    bool is_detached;
    splock_t lock;
    char *name;
    char *vendor;
    uint8_t id[16];
    inode_t *ino;
    llnode_t node;

    dev_ioctl ioctl;
    dev_rmdev rmdev;
};

// File types and associated devices
// CHR | FIFO -- object is pipe_t | DEPS NOTHING
// BLK | REG -- object is block_t | DEPS VOL
// VOL | DIR -- object is !? | DEPS VOL
// NET | SOCK -- object is !? | DEPS NET
// VDS | SRF -- object is surface_t | DESPS DEKSTOP
//   LNK -- object is `char*`

struct blkdev {
    device_t dev;

    int block;
    char *class;
    char *dname;

    blk_read read;
    blk_write write;

};

struct chardev {
    device_t dev;

    char *class;
    char *dname;

    chr_write write;
};

struct fsvolume {

    device_t dev;
    char *name;
    char *fsname;
    uint8_t id[16];

    int atimes; /* Specify the behavior of atimes handling */
    HMP_map hmap;
    atomic32_t rcu;
    llhead_t lru;

    fs_read read;
    fs_write write;

    fs_open open;
    fs_unlink unlink;
    fs_umount umount;
    
    fs_truncate truncate;

    fs_opendir opendir;
    fs_readdir readdir;
    fs_closedir closedir;
};
/*
struct net_ops {
    llhead_t queue;

    net_send send;
    net_link link;

};*/

struct vds_ops {
    device_t dev;

    vds_flip flip;
    vds_resize resize;
};


// struct mountfs
// {
//     bool read_only;
//     bool is_detached;
//     int block;
//     splock_t lock;
//     char *name;

//     fs_read read;
//     fs_write write;

//     int atimes; /* Sepcify the behaviour of atimes handling */
//     HMP_map hmap;
//     char *fsname;
//     atomic_uint rcu;
//     inode_t *root;
//     llhead_t lru;

//     fs_open open;
//     fs_lookup lookup;
//     fs_umount umount;

//     fs_opendir opendir;
//     fs_readdir readdir;
//     fs_closedir closedir;
// };


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

/* Allocate a new inode and set some mandatory fields */
inode_t *vfs_inode(int no, int mode, acl_t *acl, size_t size);


// int vfs_mkdev(CSTR name, inode_t *ino, CSTR vendor, CSTR class, CSTR devnm, void *ops);

int vfs_mkdev(CSTR name, device_t *dev, inode_t *ino);
void vfs_rmdev(CSTR name);
inode_t *vfs_search_device(CSTR name);

void register_fs(CSTR, fs_mount mount);
void unregister_fs(CSTR name);
int vfs_mountpt(CSTR name, CSTR fsname, fsvolume_t *fs, inode_t *ino);

inode_t *vfs_mount(CSTR dev, CSTR fs);
int vfs_umount(inode_t *ino);

#endif  /* _KERNEL_DRIVERS_H */
