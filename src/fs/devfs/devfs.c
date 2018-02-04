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
#include <kernel/core.h>
#include <kernel/vfs.h>
#include <kernel/sys/inode.h>
#include <kora/llist.h>
#include <kora/bbtree.h>
#include <string.h>
#include <errno.h>

// #define DEVFS_MAX_PATH 64

// extern vfs_dir_ops_t devfs_dir_ops;
// extern vfs_io_ops_t devfs_io_ops;

// struct devfs_entry {
//   int no;  /* The folder id or zero if that's a device */
//   int parent; /* The parent folder id or zero for root */
//   // hmap_node
//   const char name[DEVFS_MAX_PATH];
//   llnode_t node;
//   device_t *dev;  /* The device structure */
//   llhead_t children;  /* The list of children */
// };


// struct devfs_entry devfs_dirs[] = {
//   { DEVFS_DIR_ROOT, 0, "/", LLNODE_INIT, NULL, LLHEAD_INIT },
//   { "/block", DEVFS_DIR_BLOCK },
//   { "/char", DEVFS_DIR_CHAR },
//   { "/disk", DEVFS_DIR_DISK },
//   { "/disk/by-id", DEVFS_DIR_DISK_BY_ID },
//   { "/disk/by-uuid", DEVFS_DIR_DISK_BY_UUID },
// };

// enum {
//   DEVFS_DIR_ROOT = 0,
//   DEVFS_DIR_BLOCK, // Regroup all block device by name "MAJOR:MINOR" !?
//   DEVFS_DIR_CHAR, //  Regroup all char device by name "MAJOR:MINOR" !?

//   DEVFS_DIR_DISK, //
//   DEVFS_DIR_DISK_BY_ID,
//   DEVFS_DIR_DISK_BY_UUID,
// };

// static int devfs_directory(inode_t* ino, int no)
// {
//   ino->lba = no;
//   ino->dir_ops = &devfs_dir_ops;
//   ino->length = 0;
//   return 0;
// }

// /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

// int devfs_mount(inode_t* ino)
// {
//   kprintf(0, "DEVFS] Mount Root\n");
//   return devfs_directory(ino, DEVFS_DIR_ROOT);
// }

// int devfs_unmount(inode_t* ino)
// {
//   kprintf(0, "DEVFS] Unmount Root %d\n");
//   return 0;
// }

// int devfs_not_allowed()
// {
//   errno = EROFS;
//   return -1;
// }

// /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

// int devfs_lookup(inode_t* dir, inode_t* ino, const char *name)
// {
//   struct devfs_dir *dir_info = devfs_dirs[dir->lba];
//   char *url = kalloc(strlen(dir->info.name) + strlen(name) + 2);
//   strcpy(url, dir->info.name);
//   strcat(url, "/");
//   strcat(url, name);

//   // TODO Looks on HMAP  { NO, PR, DEV if zero }
//   struct devfs_entry *entry = hmap_get(devfs_hmap, url, strlen(url));
//   if (entry == NULL) {
//     errno = ENOENT;
//     return -1;
//   }

//   if (entry->no != 0) {
//     return devfs_directory(ino, entry->no);
//   }

//   ino = entry->dev->ino;
//   errno = 0;
//   return 0;
// }

// void *devfs_opendir(inode_t *dir)
// {
//   struct devfs_entry *dir_info = devfs_dirs[dir->lba];

//   devfs_dir_iterator_t *it = (devfs_dir_iterator_t*)kalloc(sizeof(devfs_dir_iterator_t));
//   it->no = dir->lba - 1;
//   struct devfs_entry *entry = hmap_get(devfs_hmap, url, strlen(url));
//   it->current =  llfirst(&dir_info->children);
// }

// inode_t *devfs_readdir(struct devfs_dir_iterator_t *it)
// {
//   inode_t *ino = it->current->ino;
//   if (ino)
//     it->current = ll_next(it->current, struct devfs_entry, node);
//   errno = 0;
//   return ino;
// }

// /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

// vfs_fs_ops_t devfs_fs_ops {
//   .mount = devfs_mount,
//   .unmount = devfs_unmount,
//   .chown = devfs_not_allowed,
//   .chmod = devfs_not_allowed,
//   // int (*mount)(inode_t* ino);
//   // int (*unmount)(inode_t* ino);
//   // int (*chown)(inode_t* dir, inode_t *ino, uid_t uid, gid_t gid);
//   // int (*chmod)(inode_t* dir, inode_t *ino, int mode);
// };

// vfs_dir_ops_t devfs_dir_ops {

//   .opendir = devfs_opendir,
//   // int (*opendir)(inode_t *ino);
//   // int (*readdir)(inode_t *ino);
//   // int (*closedir)(inode_t *ino);

//   // // Those create / remove / update operations are used only for FS
//   // // As all those function works on metadata, we need the containing directory of the concerned inodes.
//   .lookup = devfs_lookup,

//   .create = devfs_not_allowed,
//   .mkdir = devfs_not_allowed,
//   .symlink = devfs_not_allowed,
//   // int (*create)(inode_t* dir, inode_t* ino, const char *name);
//   // int (*lookup)(inode_t* dir, inode_t* ino, const char *name);
//   // int (*mkdir)(inode_t* dir, inode_t* ino, const char *name);
//   // int (*symlink)(inode_t* dir, inode_t* ino, const char *name, const char *link);

//   .link = devfs_not_allowed,
//   .unlink = devfs_not_allowed,
//   .rmdir = devfs_not_allowed,
//   // int (*link)(inode_t* dir, inode_t* ino, const char *name);
//   // int (*unlink)(inode_t* dir, const char *name);
//   // int (*rmdir)(inode_t* dir, const char *name);
// };
