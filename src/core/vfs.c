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
#include <kernel/vfs.h>
#include <kernel/core.h>
#include <skc/mcrs.h>
#include <skc/bbtree.h>
#include <skc/splock.h>
#include <skc/hmap.h>
#include <kernel/sys/inode.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

HMP_map dir_entries;
splock_t dir_lock;
int dir_no = 1; // TODO -- Should be replaced by DEV + INO numbers

typedef struct direntry direntry_t;

#define DIRNO_SZ 4
#define DIRNAME_SZ 256


struct direntry {
  int no;
  inode_t *ino;
};

static int vfs_check_filename(const char *str)
{
  const unsigned char *name = (const unsigned char *)str;
  while (*name == '.') {
    name++;
  }
  if (*name == '\0') {
    return -1;
  }
  for (; *name; name++) {
    if (*name < 0x20) {
      return -1;
    } else if (*name >= 0xF8 || *name == '/' || *name == '\\' || *name == ':') {
      return -1;
    } else if (*name >= 0xF0) {
      if (name[1] < 0x80 || name[1] > 0xC0 || name[2] < 0x80 || name[2] > 0xC0 || name[3] < 0x80 || name[3] > 0xC0) {
        return -1;
      }
      name += 3;
    } else if (*name >= 0xE0) {
      if (name[1] < 0x80 || name[1] > 0xC0 || name[2] < 0x80 || name[2] > 0xC0) {
        return -1;
      }
      name += 2;
    } else if (*name >= 0xC0) {
      if (name[1] < 0x80 || name[1] > 0xC0) {
        return -1;
      }
      name += 1;
    } else if (*name >= 0x80) {
      return -1;
    }
  }
  return 0;
}

/* Fetch or create a `direntry_t` structure that may be, or not, linked to an
 * inode. */
static direntry_t *vfs_direntry(inode_t *dir, const char *name)
{
  if (dir == NULL || vfs_check_filename(name)) {
    errno = EINVAL;
    return NULL;
  }

  if (!S_ISDIR(dir->mode)) {
    errno = ENOTDIR;
    return NULL;
  }


  int lg = DIRNO_SZ + strnlen(name, DIRNAME_SZ);
  if (lg >= DIRNAME_SZ + DIRNO_SZ) {
    errno = ENAMETOOLONG;
    return NULL;
  }

  direntry_t *dirent = (direntry_t *)dir->object;
  char key[DIRNAME_SZ + DIRNO_SZ + 1];
  memcpy(key, &dirent->no, DIRNO_SZ);
  strcpy(&key[DIRNO_SZ], name);

  splock_lock(&dir_lock);
  direntry_t *newent = hmp_get(&dir_entries, key, lg);
  if (newent == NULL) {
    newent = (direntry_t*)kalloc(sizeof(direntry_t));
    newent->no = ++dir_no;
    hmp_put(&dir_entries, key, lg, newent);
  }

  splock_unlock(&dir_lock);
  errno = 0;
  return newent;
}

static inode_t *vfs_unlink_direntry(inode_t *dir, const char *name, int(*unlink)(inode_t *, const char *))
{
  if (!S_ISDIR(dir->mode)) {
    errno = ENOTDIR;
    return NULL;
  }

  if (vfs_check_filename(name)) {
    errno = EINVAL;
    return NULL;
  }

  int lg = DIRNO_SZ + strnlen(name, DIRNAME_SZ);
  if (lg >= DIRNAME_SZ + DIRNO_SZ) {
    errno = ENAMETOOLONG;
    return NULL;
  }

  direntry_t *dirent = (direntry_t *)dir->object;
  char key[DIRNAME_SZ + DIRNO_SZ + 1];
  memcpy(key, &dirent->no, DIRNO_SZ);
  strcpy(&key[DIRNO_SZ], name);

  splock_lock(&dir_lock);
  direntry_t *newent = hmp_get(&dir_entries, key, lg);

  if (unlink(dir, name) != 0) {
    splock_unlock(&dir_lock);
    return NULL;
  }

  inode_t *ino = NULL;
  if (newent != NULL) {
    ino = newent->ino;
    hmp_remove(&dir_entries, key, lg);
    kfree(newent);
    if (ino != NULL) {
      vfs_close(ino);
      if (errno == 0) {
        ino = NULL;
      }
    }
  }

  splock_unlock(&dir_lock);
  errno = 0;
  return ino;
}

static inode_t *vfs_remove_direntry(inode_t *dir, const char *name)
{
  int lg = DIRNO_SZ + strnlen(name, DIRNAME_SZ);
  direntry_t *dirent = (direntry_t *)dir->object;
  char key[DIRNAME_SZ + DIRNO_SZ + 1];
  memcpy(key, &dirent->no, DIRNO_SZ);
  strcpy(&key[DIRNO_SZ], name);

  splock_lock(&dir_lock);
  direntry_t *newent = hmp_get(&dir_entries, key, lg);

  inode_t *ino = NULL;
  if (newent != NULL) {
    ino = newent->ino;
    hmp_remove(&dir_entries, key, lg);
    kfree(newent);
    if (ino != NULL) {
      vfs_close(ino);
      if (errno == 0) {
        ino = NULL;
      }
    }
  }

  splock_unlock(&dir_lock);
  return ino;
}


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

inode_t *vfs_inode(int mode, void* obj)
{
  inode_t* inode = (inode_t*)kalloc(sizeof(inode_t));
  inode->mode = mode;
  inode->object = obj;
  inode->length = 0;
  inode->lba = 0;
  kclock(&inode->btime);
  inode->ctime = inode->btime;
  inode->mtime = inode->btime;
  inode->atime = inode->btime;
  inode->counter = 1;
  // inode->uid = uid;
  // inode->gid = gid;
  return inode;
}

inode_t *vfs_lookup(inode_t *dir, const char *name)
{
  direntry_t *dirent = vfs_direntry(dir, name);
  if (dirent == NULL) {
    return NULL;
  }

  inode_t *ino = dirent->ino;
  if (ino == NULL) {
    ino = vfs_inode(0, NULL);
    if (dir->dir_ops->lookup(dir, ino, name)) {
      vfs_remove_direntry(dir, name);
      kfree(ino);
      return NULL;
    }

    if (S_ISDIR(ino->mode)) {
      ino->object = dirent;
    }
    atomic_inc(&ino->counter);
    dirent->ino = ino;
  } else {
    atomic_inc(&ino->counter);
  }

  errno = 0;
  return ino;
}

inode_t *vfs_mkdir(inode_t *dir, const char *name, int mode)
{
  direntry_t *dirent = vfs_direntry(dir, name);
  if (dirent == NULL) {
    return NULL;
  }

  inode_t *ino = vfs_inode(S_IFDIR | mode, dirent);
  if (dir->dir_ops->mkdir(dir, ino, name)) {
    kfree(ino);
    return NULL;
  }

  atomic_inc(&ino->counter);
  dirent->ino = ino;
  errno = 0;
  return ino;
}

inode_t *vfs_create(inode_t *dir, const char *name, int mode)
{
  direntry_t *dirent = vfs_direntry(dir, name);
  if (dirent == NULL) {
    return NULL;
  }

  inode_t *ino = vfs_inode(S_IFREG | mode, NULL);
  if (dir->dir_ops->create(dir, ino, name)) {
    kfree(ino);
    return NULL;
  }

  atomic_inc(&ino->counter);
  dirent->ino = ino;
  errno = 0;
  return ino;
}

int vfs_mknod(inode_t *dir, const char *name, inode_t *ino)
{
  if (dir->dir_ops->mknod == NULL) {
    errno = EROFS;
    return -1;
  }

  direntry_t *dirent = vfs_direntry(dir, name);
  if (dirent == NULL) {
    return -1;
  }

  if (dir->dir_ops->mknod(dir, ino, name)) {
    kfree(ino);
    return -1;
  }

  atomic_inc(&ino->counter);
  dirent->ino = ino;
  errno = 0;
  return 0;
}

// inode_t *vfs_symlink(inode_t *dir, const char *name, const char *link)
// {
//   errno = ENOSYS;
//   return NULL;
// }

// inode_t *vfs_mkfifo(inode_t *dir, const char *name, int mode)
// {
//   errno = ENOSYS;
//   return NULL;
// }

int vfs_unlink(inode_t *dir, const char *name)
{
  vfs_unlink_direntry(dir, name, dir->dir_ops->unlink);
  return errno == 0 ? 0 : -1;
}

int vfs_rmdir(inode_t *dir, const char *name)
{
  vfs_unlink_direntry(dir, name, dir->dir_ops->rmdir);
  return errno == 0 ? 0 : -1;
}

int vfs_chown(inode_t *ino, uid_t uid, gid_t gid)
{
  // Use of dir !?
  if (ino->fs_ops->chown(NULL, ino, uid, gid)) {
    return -1;
  }
  return 0;
}

int vfs_chmod(inode_t *ino, int mode)
{
  // Use of dir !?
  if (ino->fs_ops->chmod(NULL, ino, mode)) {
    return -1;
  }
  return 0;
}

int vfs_utimes(inode_t *ino, struct timespec *time)
{
  // TODO Will request to all parent...
  // if (ino->fs_ops->utimes(ino, mode)) {
    // return -1;
  // }
  return 0;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

inode_t *vfs_open(inode_t *ino)
{
  atomic_inc(&ino->counter);
  return ino;
}

int vfs_close(inode_t *ino)
{
  atomic_dec(&ino->counter);
  if (ino->counter == 0) {
    kfree(ino);
    errno = 0;
    return 0;
  }
  errno = EBUSY;
  return 0;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
int TMPFS_mount(inode_t *ino);
int TMPFS_unmount(inode_t *ino);

struct kVfs kVFS;

inode_t *vfs_initialize()
{
  hmp_init(&dir_entries, 16);
  direntry_t *root_ent = (direntry_t*)kalloc(sizeof(direntry_t));
  root_ent->no = ++dir_no;

  inode_t *root = vfs_inode(S_IFDIR | 0777, root_ent);
  TMPFS_mount(root);
  kVFS.root = root;
  return root;
}

void vfs_sweep(inode_t *root)
{
  root = kVFS.root;
  if (TMPFS_unmount(root)) {
    errno = -1;
  }
  hmp_destroy(&dir_entries, 0);
  kfree(root->object);
  kfree(root);
  errno = 0;
}

