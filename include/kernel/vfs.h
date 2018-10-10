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
#ifndef _KERNEL_VFS_H
#define _KERNEL_VFS_H 1

#include <kernel/core.h>
#include <kora/llist.h>
#include <time.h>

#define VFS_MAXPATH 4096
#define VFS_MAXNAME 256
#define VFS_MAXREDIRECT 32

#define VFS_OPEN  0x01
#define VFS_CREAT  0x02
#define VFS_BLOCK  0x04

#define INO_ATIME  0x10
#define INO_MTIME  0x20
#define INO_CTIME  0x40
#define INO_BTIME  0x80
#define INO_SYMLINK  0x00
#define INO_CANONALIZE  0x01
#define INO_ABSOLUTE  0x02

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

#define X_OK 1
#define W_OK 2
#define R_OK 4

#define S_IFREG  (0x8000)
#define S_IFBLK  (0x6000)
#define S_IFDIR  (0x4000)
#define S_IFCHR  (0x2000)
#define S_IFIFO  (0x1000)
#define S_IFLNK  (0xA000)
#define S_IFSOCK (0xC000)
#define S_IFWIN  (0xD000)

#define S_IFMT   (0xF000)

#define S_ISREG(m)  (((m) & S_IFMT) == S_IFREG)
#define S_ISBLK(m)  (((m) & S_IFMT) == S_IFBLK)
#define S_ISDIR(m)  (((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m)  (((m) & S_IFMT) == S_IFCHR)
#define S_ISIFO(m)  (((m) & S_IFMT) == S_IFIFO)
#define S_ISLNK(m)  (((m) & S_IFMT) == S_IFLNK)
#define S_ISSOCK(m)  (((m) & S_IFMT) == S_IFSOCK)
#define S_ISWIN(m)  (((m) & S_IFMT) == S_IFWIN)

#define S_ISGID  01000
#define S_IXGRP  02000
#define S_ISVTX  04000

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

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

    atomic32_t rcu;
    atomic32_t links;
    llhead_t dlist; // List of dirent_t;
    int block;

    void *info; // Place holder for driver info
    union { // Place holder for file info
        void *object;
    };
    union { // Place holder for underlying device info
        device_t *dev;
        blkdev_t *blk;
        chardev_t *chr;
        fsvolume_t *fs;
        netdev_t *ifnet;
    };
};

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

/* Look for an inode recursively */
inode_t *vfs_search(inode_t *root, inode_t *pwd, CSTR path, acl_t *acl);
/* Look for an inode on a directory */
inode_t *vfs_lookup(inode_t *dir, CSTR name);
/* Create an empty inode (DIR or REG) */
inode_t *vfs_create(inode_t *dir, CSTR name, int mode, acl_t *acl, int flags);
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
int vfs_read(inode_t *ino, void *buf, size_t size, off_t offset);
/* IO operations - write - only for BLK or REG */
int vfs_write(inode_t *ino, const void *buf, size_t size, off_t offset);

/* Open an inode - increment usage as concerned to RCU mechanism. */
inode_t *vfs_open(inode_t *ino);
/* Close an inode - decrement usage as concerned to RCU mechanism. */
void vfs_close(inode_t *ino);

/* Create a context to enumerate directory entries */
void *vfs_opendir(inode_t *dir, acl_t *acl);
inode_t *vfs_readdir(inode_t *dir, char *name, void *ctx);
int vfs_closedir(inode_t *dir, void *ctx);

void vfs_reset();
int vfs_fdisk(CSTR dname, long parts, long *sz);

#endif /* _KERNEL_VFS_H */
