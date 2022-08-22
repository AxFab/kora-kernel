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
#include <kora/mcrs.h>
#include <bits/atomic.h>
#include <threads.h>

typedef struct vfs vfs_t;
typedef struct vfs_share vfs_share_t;
typedef struct inode inode_t;
typedef struct device device_t;
typedef struct fl_ops fl_ops_t;
typedef struct ino_ops ino_ops_t;
typedef struct fnode fnode_t;
typedef struct path path_t;
typedef enum ftype ftype_t;
typedef struct diterator diterator_t;
typedef enum fnode_status fnode_status_t;
typedef struct fsreg fsreg_t;
typedef struct acl acl_t;

// typedef struct acl acl_t;

typedef inode_t *(*fsmount_t)(inode_t *dev, const char *options);
typedef int (*fsformat_t)(inode_t *dev, const char *options);

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
    int mode;
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
    int links;

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
    llnode_t node;
};

struct vfs_share
{
    splock_t lock;
    atomic_int rcu;
    llhead_t dev_list;
    llhead_t mnt_list;
    atomic_int dev_no;
    hmap_t fs_hmap;
};

struct vfs {
    vfs_share_t *share;
    fnode_t *root;
    fnode_t *pwd;
    int umask;
    atomic_int rcu;
};

struct fsreg
{
    char name[16];
    fsmount_t mount;
    fsformat_t format;
};

enum fnode_status
{
    FN_EMPTY = 0,
    FN_NOENTRY,
    FN_OK,
};


// Complexe structure that handle file tree
// Is private as data must be protected
// The state of this node is tricky
// 
// Parent node is staded once, can only be set back to NULL while holing parent mutex
// Name is fixed
// Inode and mode change by paire while holding mutex
// RCU and LRU is sinple to understand
// MOUNTED is set as creation, or edited with parent mutex, same as child nodes
struct fnode
{
    fnode_t *parent;
    char name[256];
    inode_t *ino;
    atomic_int rcu;
    llnode_t nlru;
    fnode_status_t mode;
    mtx_t mtx;
    bool is_mount;

    splock_t lock;
    llhead_t mnt;
    llnode_t nmt;

    llhead_t clist;
    llnode_t cnode;
};


struct ino_ops {
    // inode_t *(*open)(inode_t *dir, const char *name, ftype_t type, void *acl, int flags);
    inode_t *(*lookup)(inode_t *dir, const char *name, void *acl);
    inode_t *(*create)(inode_t *dir, const char *name, void *acl, int mode);
    void (*close)(inode_t *ino);
    int (*unlink)(inode_t *dir, const char *name);
    int (*link)(inode_t *dir, const char *name, inode_t *ino);

    int (*read)(inode_t *ino, char *buf, size_t len, xoff_t off, int flags);
    int (*write)(inode_t *dir, const char *buf, size_t len, xoff_t off, int flags);

    void *(*opendir)(inode_t *dir);
    inode_t *(*readdir)(inode_t *dir, char *name, void *iterator);
    void (*closedir)(inode_t *dir, void *);

    inode_t *(*mkdir)(inode_t *ino, const char *name, int mode, void *acl);
    int (*rmdir)(inode_t *ino, const char *name);

    inode_t *(*symlink)(inode_t *dir, const char *name, void *acl, const char *link);
    int (*readlink)(inode_t *ino, char *buf, int len);

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



extern vfs_share_t *__vfs_share;


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int vfs_open_access(int options);
inode_t *vfs_mkdir(vfs_t *vfs, const char *name, acl_t *acl, int mode);
int vfs_rmdir(vfs_t *vfs, const char *name, acl_t *acl);
inode_t *vfs_open(vfs_t *vfs, const char *name, acl_t *acl, int mode, int flags);
int vfs_unlink(vfs_t *vfs, const char *name, acl_t *acl);
int vfs_link(vfs_t *vfs, const char *name, acl_t *acl, fnode_t *target);
inode_t *vfs_symlink(vfs_t *vfs, const char *name, acl_t *acl, const char *path);
//int vfs_readlink(vfs_t *vfs, inode_t *node, char *buf, size_t len);
//int vfs_rename(vfs_t *vfs, const char *path, const char *newname, acl_t *acl);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

//inode_t *fs_mkfifo(vfs_t *vfs, const char *path, acl_t *acl, int mode);
inode_t *vfs_mount(vfs_t *vfs, const char *dev, const char *fstype, const char *path, acl_t *acl, const char *options);
int vfs_umount_at(fnode_t *node, acl_t *acl, int flags);
int vfs_umount(vfs_t *vfs, const char *path, acl_t *acl, int flags);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

//int vfs_chmod(inode_t *ino, acl_t *acl, int mode);
//int vfs_chown(inode_t *ino, acl_t *acl, acl_t *nacl);
//int vfs_utimes(inode_t *ino, acl_t *acl, xtime_t time, int flags);
//int vfs_truncate(inode_t *ino, acl_t *acl, xoff_t length);

// int vfs_readdir(inode_t *ino, void *dir, xstat_t *stat, char *buf, int len);
// int vfs_stat(inode_t *ino, xstat_t *stat);
int vfs_access(inode_t *ino, acl_t *acl, int flags);
int vfs_readlink(inode_t *ino, char *buf, int len);

// long vfs_read(inode_t *ino, char *buf, size_t len, xoff_t off, int flags);
// long vfs_write(inode_t *ino, const char *buf, size_t len, xoff_t off, int flags);

size_t vfs_fetch_page(inode_t *ino, xoff_t off);
int vfs_release_page(inode_t *ino, xoff_t off, size_t page, bool dirty);


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void vfs_resolve(fnode_t *node, inode_t *ino);
void vfs_fnode_zombify(fnode_t *node);
inode_t *vfs_parentof(fnode_t *node);
inode_t *vfs_inodeof(fnode_t *node);

int vfs_readpath(vfs_t *vfs, fnode_t *node, char *buf, int len, bool relative);

fnode_t *vfs_search(vfs_t *vfs, const char *pathname, acl_t *acl, bool resolve, bool follow);
inode_t *vfs_search_ino(vfs_t *vfs, const char *pathname, acl_t *acl, bool follow);


void vfs_dev_scavenge(device_t *dev, int max);


// Generic
vfs_t *vfs_init();
vfs_t *vfs_open_vfs(vfs_t *vfs);
vfs_t *vfs_clone_vfs(vfs_t *vfs);
int vfs_sweep(vfs_t *vfs);
// fnode_t *vfs_mount(vfs_t *vfs, const char *devname, const char *fs, const char *name, const char *options);
int vfs_format(const char *name, inode_t *dev, const char *options);
int vfs_umount_at(fnode_t *node, acl_t *acl, int flags);
// int vfs_umount(vfs_t *vfs, const char *name, acl_t *acl, int flags);
char *vfs_inokey(inode_t *ino, char *buf);

// For drivers
inode_t *vfs_inode(unsigned no, ftype_t type, device_t *dev, const ino_ops_t *ops);
inode_t *vfs_open_inode(inode_t *ino);
void vfs_close_inode(inode_t *ino);

int vfs_mkdev(inode_t *ino, const char *name);
void vfs_rmdev(const char *name);
void vfs_addfs(const char *name, fsmount_t mount, fsformat_t format);
void vfs_rmfs(const char *name);

fnode_t *vfs_mknod(fnode_t *parent, const char *name, int devno);
fnode_t *vfs_mkfifo(fnode_t *parent, const char *name);
void vfs_usage(inode_t *ino, int access, int count);


// For kernel
int vfs_lookup(fnode_t *node);
fnode_t *vfs_open_fnode(fnode_t *node);
void vfs_close_fnode(fnode_t *node);
int vfs_chdir(vfs_t *vfs, const char *path, bool root);
// int vfs_readlink(vfs_t *vfs, fnode_t *node, char *buf, int len, bool relative);


int vfs_create(fnode_t *node, void *acl, int mode);
// int vfs_open_access(int option);
// fnode_t *vfs_open_2(vfs_t *vfs, const char *pathname, void *user, int option, int mode);
// int vfs_symlink_2(fnode_t *node, void *user, const char *path);
// int vfs_unlink(fnode_t *node);
// int vfs_mkdir_2(fnode_t *node, int mode, void *acl);
// int vfs_rmdir_2(fnode_t *node);

// void vfs_usage(fnode_t *node, int flags, int use);

diterator_t *vfs_opendir(fnode_t *dir, void *acl);
fnode_t *vfs_readdir(fnode_t *dir, diterator_t *it);
int vfs_closedir(fnode_t *dir, diterator_t *it);


int vfs_read(inode_t *ino, char *buf, size_t size, xoff_t off, int flags);
int vfs_write(inode_t *ino, const char *buf, size_t size, xoff_t off, int flags);
int vfs_truncate(inode_t *ino, xoff_t off);

// Internal
fnode_t *vfs_fsnode_from(fnode_t *parent, const char *name);

int block_read(inode_t *ino, char *buf, size_t len, xoff_t off, int flags);
int block_write(inode_t *ino, const char *buf, size_t len, xoff_t off, int flags);

int vfs_fcntl(inode_t *ino, int cmd, void **args);
int vfs_seek(inode_t *ino, xoff_t off);

inode_t *tar_mount(void *base, size_t length, const char *name);


#define FB_RESIZE 0x8001
#define FB_FLIP 0x8002
#define FB_SIZE 0x8003

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


struct bkmap {
    size_t addr;
    size_t off;
    size_t len;
    void *map;
};

static inline void *bkmap(struct bkmap *bm, size_t blkno, size_t blksize, size_t size, void *dev, int flg)
{
    assert(POW2(blksize));
    if (size == 0)
        size = blksize;
    size_t by = blkno * blksize;
    size_t bo = ALIGN_DW(by, PAGE_SIZE);
    size_t be = ALIGN_UP(by + size, PAGE_SIZE);
    bm->addr = bo;
    bm->off = by - bo;
    bm->len = be - bo;
    bm->map = kmap(bm->len, dev, (xoff_t)bm->addr, flg); // TODO -- VM_BLK
    return ((char *)bm->map) + bm->off;
}

static inline void bkunmap(struct bkmap *bm)
{
    if (bm->map)
        kunmap(bm->map, bm->len);
    bm->map = 0;
}




#endif /* _KERNEL_VFS_H */
