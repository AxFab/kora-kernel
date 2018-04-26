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
#ifndef _KERNEL_MODS_FS_H
#define _KERNEL_MODS_FS_H 1

#include <kernel/vfs.h>
#include <kora/splock.h>
#include <kora/llist.h>
#include <kora/hmap.h>
#include <time.h>

typedef struct mountfs mountfs_t;
typedef struct device device_t;

typedef inode_t *(*fs_mount)(inode_t *dev);
typedef int (*fs_umount)(inode_t *ino);


typedef inode_t *(*fs_open)(inode_t *dir, CSTR name, int mode, acl_t *acl, int flags);
typedef inode_t *(*fs_lookup)(inode_t *dir, CSTR name);
typedef int (*fs_close)(inode_t *ino);

typedef void *(*fs_opendir)(inode_t *ino);
typedef inode_t *(*fs_readdir)(inode_t *ino, char *name, void *ctx);
typedef int (*fs_closedir)(inode_t *ino, void *ctx);

typedef int (*fs_read)(inode_t *ino, void *buf, size_t len, off_t off);
typedef int (*fs_write)(inode_t *ino, const void *buf, size_t len, off_t off);
typedef int (*fs_ioctl)(inode_t *ino, int cmd, void* params);

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


struct inode {
    long no;
    int mode;
    size_t lba;
    off_t length;
    // uid_t uid;
    // uid_t gid;
    struct timespec ctime;
    struct timespec atime;
    struct timespec mtime;
    struct timespec btime;

    atomic_uint rcu;
    atomic_uint links;
    void *object;
    llhead_t dlist; // List of dirent_t;
    union {
        mountfs_t *fs;
        device_t *dev;
    };

};

struct device
{
    bool read_only;
    int block;
    splock_t lock;
    char *name;

    fs_read read;
    fs_write write;
    fs_ioctl ioctl;
    fs_close close;

    CSTR vendor;
    CSTR class;
    CSTR dname;
    uint8_t id[16];
    inode_t *ino;
    llnode_t node;
};

struct mountfs {
    bool read_only;
    int block;
    splock_t lock;
    char *name;

    fs_read read;
    fs_write write;
    fs_ioctl ioctl;
    fs_close close;

    int atimes; /* Sepcify the behaviour of atimes handling */
    HMP_map hmap;
    char *fsname;
    atomic_uint rcu;
    inode_t *root;
    llhead_t lru;

    fs_open open;
    fs_lookup lookup;
    fs_umount umount;

    fs_opendir opendir;
    fs_readdir readdir;
    fs_closedir closedir;
};


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

/* Allocate a new inode and set some mandatory fields */
inode_t *vfs_inode(int no, int mode, acl_t *acl, size_t size);


int vfs_mkdev(CSTR name, device_t *dev, inode_t *ino);
void vfs_rmdev(CSTR name);


void register_fs(CSTR, fs_mount mount);
void unregister_fs(CSTR name);
int vfs_mountpt(CSTR name, CSTR fsname, mountfs_t *fs, inode_t *ino);

inode_t *vfs_mount(CSTR dev, CSTR fs);
int vfs_umount(inode_t *ino);

#endif  /* _KERNEL_MODS_FS_H */
