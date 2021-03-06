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
#ifndef _KERNEL_VFS_H
#define _KERNEL_VFS_H 1

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <kernel/stdc.h>
#include <kora/splock.h>
#include <kora/bbtree.h>
#include <kora/llist.h>
#include <kora/hmap.h>
#include <kora/atomic.h>
#include <threads.h>

typedef struct vfs vfs_t;
typedef struct inode inode_t;
typedef struct device device_t;
typedef struct fl_ops fl_ops_t;
typedef struct ino_ops ino_ops_t;
typedef struct fsnode fsnode_t;
typedef struct path path_t;
typedef enum ftype ftype_t;
typedef struct diterator diterator_t;

// typedef struct acl acl_t;

typedef inode_t *(*fsmount_t)(inode_t *dev, const char *options);

enum ftype {
    FL_INVAL = 0,
    FL_REG,  /* Regular file (FS) */
    FL_BLK,  /* Block device */
    FL_PIPE,  /* Pipe */
    FL_CHR,  /* Char device */
    FL_NET,  /* Network interface */
    FL_SOCK,  /* Network socket */
    FL_LNK,  /* Symbolic link (FS) */
    FL_FRM,  /* Video stream */
    FL_DIR,  /* Directory (FS) */
    // FL_INFO,  /* Information file
    // FL_SFC,  /* Application surface */
    // FL_VOL,  /* File system volume */
    // FL_TTY,  /* Terminal (Virtual) */
    // FL_WIN,  /* Window (Virtual) */
};

enum {
    IO_OPEN = (1 << 0), // on open allow to open existing file
    IO_CREAT = (1 << 1), // on open allow to create the file

    IO_NOBLOCK = (1 << 2), // read or write should not block
    IO_ATOMIC = (1 << 3), // complete operation or nothing

    FD_RDONLY = (1 << 5),
};

struct inode {
    unsigned no;
    ftype_t type;

    size_t lba;
    xoff_t length;
    // User / Group or ACL
    xtime_t ctime;
    xtime_t atime;
    xtime_t mtime;
    xtime_t btime;

    const ino_ops_t *ops;
    const fl_ops_t *fops;
    device_t *dev;
    void *drv_data;
    void *fl_data;

    splock_t lock;
    atomic_int rcu;
    bbnode_t bnode;
};

struct device {
    int no;
    int flags;
    char *devname;
    char *devclass;
    char *model;
    char *vendor;
    uint8_t uuid[16];
    hmap_t map;
    bbtree_t btree;
    splock_t lock;
    atomic_int rcu;
    llhead_t llru;
    size_t block;
    inode_t *underlying;
};

struct vfs {
    fsnode_t *root;
    fsnode_t *pwd;
    int umask;
    atomic_int rcu;
};

struct ino_ops {
    inode_t *(*open)(inode_t *dir, const char *name, ftype_t type, void *acl, int flags);
    void (*close)(inode_t *dir, inode_t *ino);
    int (*unlink)(inode_t *dir, const char *name);

    int (*read)(inode_t *ino, char *buf, size_t len, xoff_t off, int flags);
    int (*write)(inode_t *dir, const char *buf, size_t len, xoff_t off, int flags);

    void *(*opendir)(inode_t *dir);
    inode_t *(*readdir)(inode_t *dir, char *name, void *iterator);
    void (*closedir)(inode_t *dir, void *);

    int(*truncate)(inode_t *ino, xoff_t length);

    page_t(*fetch)(inode_t *ino, xoff_t off);
    void(*release)(inode_t *ino, xoff_t off, page_t pg, bool dirty);

    int (*ioctl)(inode_t *ino, int cmd, void **params);
};

struct fl_ops {
    int (*read)(inode_t *ino, char *buf, size_t len, xoff_t, int flags);
    int (*write)(inode_t *dir, const char *buf, size_t len, xoff_t, int flags);

    page_t(*fetch)(inode_t *ino, xoff_t off);
    void(*release)(inode_t *ino, xoff_t off, page_t pg, bool dirty);
    int (*seek)(inode_t *ino, xoff_t off);

    void(*usage)(inode_t *ino, int flgas, int use);
    int (*fcntl)(inode_t *ino, int cmd, void **params);
    void(*destroy)(inode_t *ino);
};

struct fsnode {
    fsnode_t *parent;
    char name[256];
    inode_t *ino;
    atomic_int rcu;
    llnode_t nlru;
    int mode;
    mtx_t mtx;

    splock_t lock;
    llhead_t mnt;
    llnode_t nmt;
};

enum {
    FN_EMPTY = 0,
    FN_NOENTRY,
    FN_LRU,
    FN_OK,
};




// Generic
vfs_t *vfs_init();
vfs_t *vfs_open_vfs(vfs_t *vfs);
vfs_t *vfs_clone_vfs(vfs_t *vfs);
void vfs_sweep(vfs_t *vfs);
fsnode_t *vfs_mount(vfs_t *vfs, const char *devname, const char *fs, const char *name, const char *options);
// unmount
char *vfs_inokey(inode_t *ino, char *buf);

// For drivers
inode_t *vfs_inode(unsigned no, ftype_t type, device_t *dev, const ino_ops_t *ops);
inode_t *vfs_open_inode(inode_t *ino);
void vfs_close_inode(inode_t *ino);

int vfs_mkdev(inode_t *ino, const char *name);
void vfs_rmdev(const char *name);
void vfs_addfs(const char *name, fsmount_t mount);
void vfs_rmfs(const char *name);

fsnode_t *vfs_mknod(fsnode_t *parent, const char *name, int devno);


// For kernel
fsnode_t *vfs_search(vfs_t *vfs, const char *pathname, void *user, bool resolve);
int vfs_lookup(fsnode_t *node);
fsnode_t *vfs_open_fsnode(fsnode_t *node);
void vfs_close_fsnode(fsnode_t *node);
int vfs_chdir(vfs_t *vfs, const char *path, bool root);
int vfs_readlink(vfs_t *vfs, fsnode_t *node, char *buf, int len, bool relative);

void vfs_usage(fsnode_t *node, int flags, int use);

diterator_t *vfs_opendir(fsnode_t *dir, void *acl);
fsnode_t *vfs_readdir(fsnode_t *dir, diterator_t *it);
int vfs_closedir(fsnode_t *dir, diterator_t *it);


int vfs_read(inode_t *ino, char *buf, size_t size, xoff_t off, int flags);
int vfs_write(inode_t *ino, const char *buf, size_t size, xoff_t off, int flags);


// Internal
fsnode_t *vfs_fsnode_from(fsnode_t *parent, const char *name);

int block_read(inode_t *ino, char *buf, size_t len, xoff_t off, int flags);
int block_write(inode_t *ino, const char *buf, size_t len, xoff_t off, int flags);

int vfs_access(fsnode_t *node, int access);

int vfs_fcntl(inode_t *ino, int cmd, void **args);
int vfs_seek(inode_t *ino, xoff_t off);

inode_t *tar_mount(void *base, size_t length, const char *name);


#define FB_RESIZE 0x8001
#define FB_FLIP 0x8002


#endif /* _KERNEL_VFS_H */
