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
 *
 *      Memory managment unit configuration.
 */
#ifndef _KERNEL_SYS_INODE_H
#define _KERNEL_SYS_INODE_H 1

#include <kernel/types.h>
#include <kora/atomic.h>


typedef struct fifo fifo_t;
typedef struct mountpt mountpt_t;
// #define O_SYNC 1

// int call_method(void *entry, int argc, void *argv);

// __stinline int async_job(int (*func)(), int argc, ...)
// {
//   int *argv = (&argc) + 1;
//   // TODO -- Create a thread with previous arg and func as entry point
//   // and resume here when done.
//   int ret_value = call_method(func, argc, argv);
//   return ret_value;
// }

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

typedef inode_t *(*fs_mount)(inode_t *dev);
typedef int (*fs_unmount)(inode_t *ino);
typedef int (*fs_close)(inode_t *ino);

typedef inode_t *(*fs_lookup)(inode_t *dir, const char *name);
typedef inode_t *(*fs_create)(inode_t *dir, const char *name, int mode);

typedef int(*fs_unlink)(inode_t *dir, const char *name);

typedef int(*fs_read)(inode_t *ino, void *buffer, size_t length,
                      off_t offset);
typedef int(*fs_write)(inode_t *ino, const void *buffer, size_t length,
                       off_t offset);
typedef int(*fs_truncate)(inode_t *ino, off_t offset);


typedef struct vfs_fs_ops vfs_fs_ops_t;
typedef struct vfs_io_ops vfs_io_ops_t;
typedef struct vfs_dir_ops vfs_dir_ops_t;

struct vfs_fs_ops {
    // mount / unmount pair is used for driver that build something over another device.
    // AFAIK it's only used for FS over Block devices.
    fs_mount mount;
    fs_unmount unmount;
};

struct vfs_io_ops {
    // Device IO operations. The methods are called without locks.
    // It can be synchronous or used a IO schedule for asynchronous jobs.
    fs_read read;
    fs_write write;

    // This one is mainly used for REG file.
    // AFAIK Nothing prevent device to implement this one, but might damage FS.
    fs_truncate truncate;
};

struct vfs_ino_ops {

    fs_close close;
    // int (*chown)(inode_t* dir, inode_t *ino, uid_t uid, gid_t gid);
    // int (*chmod)(inode_t* dir, inode_t *ino, int mode);
    // int (*utimes)(inode_t* dir, inode_t *ino, ...);
};

struct vfs_dir_ops {
    // int (*opendir)(inode_t *ino);
    // int (*readdir)(inode_t *ino);
    // int (*closedir)(inode_t *ino);

    // Those create / remove / update operations are used only for FS
    // As all those function works on metadata, we need the containing directory of the concerned inodes.
    fs_lookup lookup;

    fs_create create;
    fs_create mkdir;

    // int (*symlink)(inode_t* dir, inode_t* ino, const char *name, const char *link);

    // int (*link)(inode_t* dir, inode_t* ino, const char *name);
    fs_unlink unlink;
    fs_unlink rmdir;
};


struct inode {
    long no;
    int mode;
    size_t lba;
    off_t length;
    // uid_t uid;
    // uid_t gid;
    int block;
    atomic_t counter; // < Increase at creation, mmap, link, fd.
    atomic_t links;

    union {
        void *object;
        void *pcache;
        mountpt_t *mnt;
        void *chard;
        fifo_t *fifo;
        void *slink;
        void *socket;
    };

    vfs_fs_ops_t *fs_ops;
    vfs_io_ops_t *io_ops;
    vfs_dir_ops_t *dir_ops;

    struct timespec ctime;
    struct timespec atime;
    struct timespec mtime;
    struct timespec btime;
};


#define S_IFREG  (010 << 12)
#define S_IFBLK  (006 << 12)
#define S_IFDIR  (004 << 12)
#define S_IFCHR  (002 << 12)
#define S_IFIFO  (001 << 12)
#define S_IFLNK  (012 << 12)
#define S_IFSOCK (014 << 12)

#define S_IFMT  (15 << 12)

#define S_ISREG(m)  (((m) & S_IFMT) == S_IFREG)
#define S_ISBLK(m)  (((m) & S_IFMT) == S_IFBLK)
#define S_ISDIR(m)  (((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m)  (((m) & S_IFMT) == S_IFCHR)
#define S_ISIFO(m)  (((m) & S_IFMT) == S_IFIFO)
#define S_ISLNK(m)  (((m) & S_IFMT) == S_IFLNK)
#define S_ISSOCK(m)  (((m) & S_IFMT) == S_IFSOCK)

#define S_ISGID  01000
#define S_IXGRP  02000
#define S_ISVTX  04000


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

inode_t *vfs_inode(int no, int mode, size_t size);
inode_t *vfs_open(inode_t *ino);
void vfs_close(inode_t *ino);
// void vfs_link(dirent_t *den, inode_t* ino);

int vfs_read(inode_t *ino, void *buffer, size_t size, off_t offset);
int vfs_write(inode_t *ino, const void *buffer, size_t size, off_t offset);
page_t vfs_page(inode_t *ino, off_t offset);


int vfs_mount(inode_t *dir, const char *name, const char *device,
              const char *fs);
inode_t *vfs_mount_as_root(const char *device, const char *fs);

vfs_fs_ops_t *vfs_fs(const char *fs);


inode_t *vfs_lookup(inode_t *dir, const char *name);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

inode_t *vfs_search(inode_t *root, inode_t *pwd, const char *path);
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


#define FP_NOBLOCK (1 << 0)
#define FP_EOL (1 << 1)
#define FP_WR (1 << 2)


/* Instanciate a new FIFO */
fifo_t *fifo_init(void *buf, size_t lg);
/* Look for a specific bytes in consumable data */
size_t fifo_indexof(fifo_t *fifo, char ch) ;
/* Read some bytes from the FIFO */
size_t fifo_out(fifo_t *fifo, void *buf, size_t lg, int flags);
/* Write some bytes on the FIFO */
size_t fifo_in(fifo_t *fifo, const void *buf, size_t lg, int flags);
/* Reinitialize the queue */
void fifo_reset(fifo_t *fifo);


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

#endif /* _KERNEL_SYS_INODE_H */
