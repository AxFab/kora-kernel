/*
 *      This file is part of the SmokeOS project.
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
#include <skc/atomic.h>


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

typedef struct vfs_fs_ops vfs_fs_ops_t;
typedef struct vfs_io_ops vfs_io_ops_t;
typedef struct vfs_dir_ops vfs_dir_ops_t;

struct vfs_fs_ops {
  // mount / unmount pair is used for driver that build something over another device.
  // AFAIK it's only used for FS over Block devices.
  int (*mount)(inode_t* ino);
  int (*unmount)(inode_t* ino);

  int (*chown)(inode_t* dir, inode_t *ino, uid_t uid, gid_t gid);
  int (*chmod)(inode_t* dir, inode_t *ino, int mode);
  // int (*utimes)(inode_t* dir, inode_t *ino, ...);
};

struct vfs_io_ops {
  // Device IO operations. The methods are called without locks.
  // It can be synchronous or used a IO schedule for asynchronous jobs.
  int (*read)(inode_t* ino, void *buf, size_t size, off_t offset);
  int (*write)(inode_t* ino, const void *buf, size_t size, off_t offset);

  // This one is mainly used for REG file.
  // Nothing knowned
  int (*truncate)(inode_t* dir, inode_t* ino, off_t offset);
};

struct vfs_dir_ops {
  int (*opendir)(inode_t *ino);
  int (*readdir)(inode_t *ino);
  int (*closedir)(inode_t *ino);

  // Those create / remove / update operations are used only for FS
  // As all those function works on metadata, we need the containing directory of the concerned inodes.
  int (*create)(inode_t* dir, inode_t* ino, const char *name);
  int (*lookup)(inode_t* dir, inode_t* ino, const char *name);
  int (*mkdir)(inode_t* dir, inode_t* ino, const char *name);
  int (*symlink)(inode_t* dir, inode_t* ino, const char *name, const char *link);
  int (*mknod)(inode_t* dir, inode_t* ino, const char *name);

  int (*link)(inode_t* dir, inode_t* ino, const char *name);
  int (*unlink)(inode_t* dir, const char *name);
  int (*rmdir)(inode_t* dir, const char *name);
};


struct inode {
  int mode;
  size_t lba;
  off_t length;
  // uid_t uid;
  // uid_t gid;
  int block;
  atomic_t counter; // < Increase at creation, mmap, link, fd.
  void *object;

  vfs_fs_ops_t *fs_ops;
  vfs_io_ops_t *io_ops;
  vfs_dir_ops_t *dir_ops;

  struct timespec ctime;
  struct timespec atime;
  struct timespec mtime;
  struct timespec btime;
};


#define S_IFREG  (1 << 12)
#define S_IFBLK  (2 << 12)
#define S_IFDIR  (3 << 12)
#define S_IFCHR  (4 << 12)
#define S_IFIFO  (5 << 12)
#define S_IFLNK  (6 << 12)
#define S_IFSOCK  (7 << 12)

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


#endif /* _KERNEL_SYS_INODE_H */
