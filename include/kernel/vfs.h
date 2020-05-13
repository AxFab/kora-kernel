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

#include <kernel/core.h>
#include <kernel/types.h>
#include <kora/llist.h>
#include <kora/bbtree.h>
#include <string.h>
#include <time.h>

#define VFS_MAXPATH 4096
#define VFS_MAXNAME 256
#define VFS_MAXREDIRECT 32

#define VFS_OPEN  0x01
#define VFS_CREAT  0x02
#define VFS_NOBLOCK  0x04

#define VFS_RDONLY  0x001

#define INO_ATIME  0x10
#define INO_MTIME  0x20
#define INO_CTIME  0x40
#define INO_BTIME  0x80
#define INO_SYMLINK  0x00
#define INO_CANONALIZE  0x01
#define INO_ABSOLUTE  0x02

#define VFS_ISDIR(ino)  (ino->type == FL_DIR || ino->type == FL_VOL)

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

#define X_OK 1
#define W_OK 2
#define R_OK 4

typedef enum ftype ftype_t;

enum ftype {
    FL_INVAL = 0,
    FL_REG,  /* Regular file (FS) */
    FL_BLK,  /* Block device */
    FL_PIPE,  /* Pipe */
    FL_CHR,  /* Char device */
    FL_NET,  /* Network interface */
    FL_SOCK,  /* Network socket */
    FL_LNK,  /* Symbolic link (FS) */
    FL_INFO,  /* Information file */
    FL_SFC,  /* Application surface */
    FL_VDO,  /* Video stream */
    FL_DIR,  /* Directory (FS) */
    FL_VOL,  /* File system volume */
    FL_TTY,  /* Terminal (Virtual) */
    FL_WIN,  /* Window (Virtual) */
};

struct acl {
    unsigned uid;
    unsigned gid;
    unsigned short mode;
};

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

struct ino_ops {
    // All
    int(*fcntl)(inode_t *ino, int cmd, void *params);
    int(*close)(inode_t *ino);
    void (*zero_reader)(inode_t*ino);
    void (*zero_writer)(inode_t*ino);
    // Mapping
    page_t(*fetch)(inode_t *ino, off_t off);
    void(*sync)(inode_t *ino, off_t off, page_t pg);
    void(*release)(inode_t *ino, off_t off, page_t pg);
    // Read / Write
    int(*read)(inode_t *ino, char *buf, size_t len, int flags, off_t off);
    int(*write)(inode_t *ino, const char *buf, size_t len, int flags, off_t off);
    int(*reset)(inode_t *ino);
    // Directory
    void *(*opendir)(inode_t *dir);
    inode_t *(*readdir)(inode_t *dir, char *name, void *ctx);
    int(*closedir)(inode_t *dir, void *ctx);
    // Regular file
    int(*truncate)(inode_t *ino, off_t length);
    // Framebuffer
    void(*flip)(inode_t *ino);
    void(*resize)(inode_t *ino, int width, int height);
    void(*copy)(inode_t *dst, inode_t *src, int x, int y, int lf, int tp, int rg, int bt);
};

struct inode {
    unsigned no;
    int flags;
    size_t lba;
    off_t length;
    ftype_t type;
    acl_t *acl;
    clock64_t btime;
    clock64_t ctime;
    clock64_t mtime;
    clock64_t atime;
    atomic_int rcu;
    splock_t lock;

    atomic_int links;
    llhead_t dlist; // List of dirent_t;
    bbnode_t bnode;

    atomic_int count_rd;
    atomic_int count_wr;

    void *info; // Place holder for driver info
    ino_ops_t *ops;

    device_t *dev;
    // FL_BLK, FL_CHR, FL_VDO
    // union { // Place holder for underlying device info
    //     device_t *vol; // FL_REG, FL_DIR, FL_LNK, FL_VOL
    //     // pipe_t *pipe; // FL_PIPE
    //     // ifnet_t *ifnet; // FL_NET
    //     socket_t *socket; // FL_SOCK
    //     // desktop_t *desktop; // FL_WIN, FL_TTY
    // } und;

    llnode_t lnode;
};

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

/* Look for an inode recursively */
inode_t *vfs_search(inode_t *root, inode_t *pwd, CSTR path, acl_t *acl);
/* Look for an inode on a directory */
inode_t *vfs_lookup(inode_t *dir, CSTR name);
/* Create an empty inode (DIR or REG) */
inode_t *vfs_create(inode_t *dir, CSTR name, ftype_t type, acl_t *acl, int flags);
/* Link an inode (If supported) */
int vfs_link(inode_t *dir, CSTR name, inode_t *ino);
/* Unlink / delete an inode */
int vfs_unlink(inode_t *dir, CSTR name);
/* Rename an inode (can use either link, rename or copy depending on fs) */
int vfs_rename(inode_t *dir, CSTR name, inode_t *ino);

/* Create a symlink */
inode_t *vfs_symlink(inode_t *dir, CSTR name, CSTR path);
/* Read a link */
int vfs_readlink(inode_t *ino, char *buf, int len, int flags);

/* Update meta-data, owners */
int vfs_chown(inode_t *ino, acl_t *acl);
/* Update meta-data, access rights */
int vfs_chmod(inode_t *ino, int mode);
/* Update meta-data, times */
int vfs_chtimes(inode_t *ino, struct timespec *ts, int flags);
/* Update meta-data, size */
int vfs_truncate(inode_t *ino, off_t lengtg);

/* Check if a user have access to a file */
int vfs_access(inode_t *ino, int access, acl_t *acl);

/* IO operations - read - only for BLK or REG */
int vfs_read(inode_t *ino, char *buf, size_t size, off_t offset, int flags);
/* IO operations - write - only for BLK or REG */
int vfs_write(inode_t *ino, const char *buf, size_t size, off_t offset, int flags);

/* Open an inode - increment usage as concerned to RCU mechanism. */
inode_t *vfs_open(inode_t *ino, int access);
/* Close an inode - decrement usage as concerned to RCU mechanism. */
void vfs_close(inode_t *ino, int access);

/* Create a context to enumerate directory entries */
void *vfs_opendir(inode_t *dir, acl_t *acl);
inode_t *vfs_readdir(inode_t *dir, char *name, void *ctx);
int vfs_closedir(inode_t *dir, void *ctx);

inode_t *vfs_mount(const char *dev, const char *fs, const char *name);
int vfs_mkdev(inode_t *ino, const char *name);
char *vfs_inokey(inode_t *ino, char *buf);

void vfs_init();
void vfs_fini();
int vfs_fdisk(CSTR dname, long parts, long *sz);

static inline int vfs_puts(inode_t *ino, const char *buf)
{
    return vfs_write(ino, buf, strlen(buf), 0, 0);
}

#endif /* _KERNEL_VFS_H */
