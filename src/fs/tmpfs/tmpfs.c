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
 */
#include <kernel/core.h>
#include <kernel/vfs.h>
#include <kernel/sys/inode.h>
#include <skc/bbtree.h>
#include <string.h>
#include <errno.h>

extern vfs_dir_ops_t TMPFS_dir_ops;
extern vfs_io_ops_t TMPFS_io_ops;

size_t LBA_NB = 0;

bbtree_t TMPFS_filetree;

typedef struct tmpfs_entry {
  size_t parent;
  size_t no;
  off_t length;
  int mode;
  // uid_t uid;
  bbnode_t node;
  char name[256];
} tmpfs_entry_t;



int TMPFS_mount(inode_t* ino)
{
  bbtree_init(&TMPFS_filetree);

  ino->lba = ++LBA_NB;
  tmpfs_entry_t *entry = (tmpfs_entry_t*)kalloc(sizeof(tmpfs_entry_t));
  entry->parent = -1;
  entry->no = ino->lba;
  entry->node.value_ = entry->no;
  entry->length = 0;
  entry->mode = S_IFDIR | 0705;
  // entry->uid = 0;
  bbtree_insert(&TMPFS_filetree, &entry->node);

  ino->lba = entry->no;
  ino->dir_ops = &TMPFS_dir_ops;
  // ino->fs_ops = &TMPFS_fs_ops;
  kprintf(0, "TMPFS] Mount Root %d\n", ino->lba);
  errno = 0;
  return 0;
}

int TMPFS_unmount(inode_t* ino)
{
  kprintf(0, "TMPFS] Unmount Root %d\n", ino->lba);
  tmpfs_entry_t *entry = bbtree_search_eq(&TMPFS_filetree, ino->lba, tmpfs_entry_t, node);
  if (entry == NULL) {
    return -1;
  }
  bbtree_remove(&TMPFS_filetree, entry->node.value_);
  kfree(entry);

  // if (TMPFS_filetree)
  // TODO Remove all on TMPFS_filetree
  errno = 0;
  return 0;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

static tmpfs_entry_t *TMPFS_search(inode_t* dir, const char *name)
{
  tmpfs_entry_t *entry = bbtree_first(&TMPFS_filetree, tmpfs_entry_t, node);
  for (; entry; entry = bbtree_next(&entry->node, tmpfs_entry_t, node)) {
    if (entry->parent == dir->lba && strcmp(entry->name, name) == 0) {
      return entry;
    }
  }
  return NULL;
}

int TMPFS_lookup(inode_t* dir, inode_t* ino, const char *name)
{
  tmpfs_entry_t *entry = TMPFS_search(dir, name);
  if (entry == NULL) {
    errno = ENOENT;
    return -1;
  }

  if (ino != NULL) {
    ino->lba = entry->no;
    ino->mode = entry->mode;
    ino->length = entry->length;
    if (S_ISDIR(ino->mode)) {
      ino->dir_ops = &TMPFS_dir_ops;
    } else {
      ino->io_ops = &TMPFS_io_ops;
    }
    // ino->uid = entry->uid;
  }
  errno = 0;
  return 0;
}

int TMPFS_create(inode_t* dir, inode_t* ino, const char *name)
{
  if (TMPFS_lookup(dir, NULL, name) == 0) {
    errno = EEXIST;
    return -1;
  }
  tmpfs_entry_t *entry = (tmpfs_entry_t*)kalloc(sizeof(tmpfs_entry_t));
  entry->parent = dir->lba;
  entry->no = ++LBA_NB;
  entry->node.value_ = entry->no;
  entry->length = ino->length;
  entry->mode = S_IFREG | (ino->mode & 07777);
  strncpy(entry->name, name, 256);
  // entry->uid = ino->uid;
  bbtree_insert(&TMPFS_filetree, &entry->node);

  ino->lba = entry->no;
  ino->io_ops = &TMPFS_io_ops;
  kprintf(0, "TMPFS] File %d:%s under %d\n", ino->lba, name, dir->lba);
  errno = 0;
  return 0;
}

int TMPFS_mkdir(inode_t* dir, inode_t* ino, const char *name)
{
  if (TMPFS_lookup(dir, NULL, name) == 0) {
    errno = EEXIST;
    return -1;
  }
  tmpfs_entry_t *entry = (tmpfs_entry_t*)kalloc(sizeof(tmpfs_entry_t));
  entry->parent = dir->lba;
  entry->no = ++LBA_NB;
  entry->node.value_ = entry->no;
  entry->length = ino->length;
  entry->mode = S_IFDIR | (ino->mode & 07777);
  strncpy(entry->name, name, 256);
  // entry->uid = ino->uid;
  bbtree_insert(&TMPFS_filetree, &entry->node);

  ino->lba = entry->no;
  ino->dir_ops = &TMPFS_dir_ops;
  kprintf(0, "TMPFS] Directory %d:%s under %d\n", ino->lba, name, dir->lba);
  errno = 0;
  return 0;
}

int TMPFS_mknod(inode_t* dir, inode_t* ino, const char *name)
{
  if (TMPFS_lookup(dir, NULL, name) == 0) {
    errno = EEXIST;
    return -1;
  }
  tmpfs_entry_t *entry = (tmpfs_entry_t*)kalloc(sizeof(tmpfs_entry_t));
  entry->parent = dir->lba;
  entry->no = ++LBA_NB;
  entry->node.value_ = entry->no;
  entry->length = ino->length;
  entry->mode = 0; // ino->mode;
  strncpy(entry->name, name, 256);
  // entry->uid = ino->uid;
  bbtree_insert(&TMPFS_filetree, &entry->node);

  kprintf(0, "TMPFS] Accept node %d:%s under %d\n", entry->no, name, dir->lba);
  errno = 0;
  return 0;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

// int TMPFS_read(inode_t* ino, const char *buf, size_t size, off_t offset)
// {
//   return -1;
// }

// int TMPFS_write(inode_t* ino, char *buf, size_t size, off_t offset)
// {
//   return -1;
// }

// int TMPFS_truncate(inode_t* ino, off_t offset)
// {
//   return -1;
// }

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

// int TMPFS_link(inode_t* dir, inode_t* ino, const char *name)
// {
//   return -1;
// }

int TMPFS_unlink(inode_t* dir, const char *name)
{
  tmpfs_entry_t *entry = TMPFS_search(dir, name);
  if (entry == NULL) {
    errno = ENOENT;
    return -1;
  } else if (S_ISDIR(entry->mode)) {
    errno = EPERM;
    return -1;
  }

  bbtree_remove(&TMPFS_filetree, entry->node.value_);
  kfree(entry);
  kprintf(0, "TMPFS] Unlink entry %s under %d\n", name, dir->lba);
  errno = 0;
  return 0;
}


int TMPFS_rmdir(inode_t* dir, const char *name)
{
  tmpfs_entry_t *entry = TMPFS_search(dir, name);
  if (entry == NULL) {
    errno = ENOENT;
    return -1;
  } else if (!S_ISDIR(entry->mode)) {
    errno = ENOTDIR;
    return -1;
  }

  tmpfs_entry_t *sub = bbtree_first(&TMPFS_filetree, tmpfs_entry_t, node);
  for (; sub; sub = bbtree_next(&sub->node, tmpfs_entry_t, node)) {
    if (sub->parent == entry->no) {
      errno = ENOTEMPTY;
      return -1;
    }
  }

  bbtree_remove(&TMPFS_filetree, entry->node.value_);
  kfree(entry);
  kprintf(0, "TMPFS] Rmdir entry %s under %d\n", name, dir->lba);
  errno = 0;
  return 0;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

vfs_fs_ops_t TMPFS_fs_ops = {
  .mount = TMPFS_mount,
  .unmount = TMPFS_unmount,
};

vfs_dir_ops_t TMPFS_dir_ops = {

  .create = TMPFS_create,
  .lookup = TMPFS_lookup,
  .mkdir = TMPFS_mkdir,
  .mknod = TMPFS_mknod,

  // .read = TMPFS_read,
  // .write = TMPFS_write,
  // .truncate = TMPFS_truncate,

  // .link = TMPFS_link,
  .unlink = TMPFS_unlink,
  .rmdir = TMPFS_rmdir,
};


vfs_io_ops_t TMPFS_io_ops = {
  .read = NULL,
};

