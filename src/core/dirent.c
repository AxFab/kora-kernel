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
#include <kernel/sys/inode.h>
#include <kernel/sys/device.h>
#include <string.h>
#include <errno.h>

/* If the driver forget `errno' we set as unable to work properly */
#define DEF_ERRNO(e)  do { if(errno == 0) errno = (e); } while(0)

typedef struct direntry direntry_t;

struct direntry {
    mountpt_t *mnt;
    inode_t *ino;
    int lg;
    char key[256 + 4];
};

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

/**
 * The function look into the VFS hash table for an entry and if not existing
 * create one.
 *
 * @dir  Parent directory of the new entry
 * @name  Filename of the new entry
 * @block  Allow the function to block or not
 *
 * The function might block if this entry is in creation state.
 * The entry is on creation state after this method create it until a call to
 * `vfs_wdentry' or `vfs_rmentry' which will set the status of the inode.
 * The function return NULL in three cases, `block' argument is false and the
 * function will block, the function have been blocked but is release by
 * `vfs_rmentry' in which case the inode doesn't exist anyway, or the arguments
 * are invalid.
 *
 * @errno  EWOULDBLOCK  ENOENT  EINVAL
 */
static direntry_t *vfs_rdentry(inode_t *dir, const char *name, bool block)
{
    direntry_t *dre;
    /* Check arguments - we assume name is a checked valid filename */
    if (dir == NULL || !S_ISDIR(dir->mode) || name == NULL) {
        errno = EINVAL;
        return NULL;
    }

    int lg = strlen(name) + 4;
    char *key = kalloc(lg + 1);
    ((int *)key)[0] = dir->no;
    strcpy(&key[4], name);
    mountpt_t *mnt = dir->mnt;

    /* Try to read without locking
    dre = hmp_get_unlock(&mnt->hmap, key, lg);
    if (dre != NULL && dre->ino != NULL) {
      errno = NULL;
      return dre;
    } */

    splock_lock(&mnt->lock);
    dre = hmp_get(&mnt->hmap, key, lg);
    if (dre == NULL) {
        dre = (direntry_t *)kalloc(sizeof(direntry_t));
        dre->mnt = mnt;
        dre->/*map_ent.*/lg = lg;
        memcpy(dre->key, key, lg + 1);
        hmp_put(&mnt->hmap, key, lg, dre);
    } else if (dre->ino == NULL) {
        // if (!block) {
        kfree(key);
        splock_unlock(&mnt->lock);
        errno = EWOULDBLOCK;
        return NULL;
        // }
        //
        // TODO - Wait

    }
    kfree(key);
    splock_unlock(&mnt->lock);
    return dre;
}

static void vfs_wdentry(direntry_t *dre, inode_t *ino)
{
    splock_lock(&dre->mnt->lock);
    dre->ino = ino;
    // TODO -- Wake up other !
    splock_unlock(&dre->mnt->lock);
}

static void vfs_rmentry(direntry_t *dre)
{
    splock_lock(&dre->mnt->lock);
    hmp_remove(&dre->mnt->hmap, dre->key, dre->lg);
    // TODO -- Wake up other !
    splock_unlock(&dre->mnt->lock);
}


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

static direntry_t *vfs_lookup_(inode_t *dir, const char *name)
{
    if (unlikely(dir->dir_ops == NULL || dir->dir_ops->lookup == NULL)) {
        errno = ENOSYS;
        return NULL;
    }

    direntry_t *dre = vfs_rdentry(dir, name, true);
    if (dre == NULL) {
        return NULL;
    } else if (dre->ino == NULL) {
        inode_t *ino = dir->dir_ops->lookup(dir, name);
        if (ino == NULL) {
            DEF_ERRNO(ENOSYS);
            vfs_rmentry(dre);
            return NULL;
        }

        if (S_ISDIR(ino->mode)) {
            ino->mnt = dir->mnt;
        }
        vfs_wdentry(dre, ino);
    }
    errno = 0;
    return dre;
}

inode_t *vfs_lookup(inode_t *dir, const char *name)
{
    direntry_t *dre = vfs_lookup_(dir, name);
    if (dre == NULL) {
        return NULL;
    }
    return vfs_open(dre->ino);
}

int vfs_mount(inode_t *dir, const char *name, const char *device,
              const char *fs)
{
    vfs_fs_ops_t *ops = vfs_fs(fs);
    if (ops == NULL || ops->mount == NULL || ops->unmount == NULL) {
        errno = ENOSYS;
        return -1;
    }

    direntry_t *dre = vfs_lookup_(dir, name);
    if (dre == NULL) {
        return -1;
    }

    inode_t *dev = NULL;
    if (device != NULL) {
        dev = vfs_lookup_device(device);
        if (dev == NULL) {
            errno = ENODEV;
            return -1;
        }
    }

    inode_t *mnt = ops->mount(dev);
    if (mnt == NULL) {
        DEF_ERRNO(ENOSYS);
        return -1;
    }

    vfs_wdentry(dre, mnt);
    return 0;
}

inode_t *vfs_mount_as_root(const char *device, const char *fs)
{
    vfs_fs_ops_t *ops = vfs_fs(fs);
    if (ops == NULL || ops->mount == NULL || ops->unmount == NULL) {
        errno = ENOSYS;
        return NULL;
    }

    inode_t *dev = NULL;
    if (device != NULL) {
        dev = vfs_lookup_device(device);
        if (dev == NULL) {
            errno = ENODEV;
            return NULL;
        }
    }

    inode_t *mnt = ops->mount(dev);
    if (mnt == NULL) {
        DEF_ERRNO(ENOSYS);
        return NULL;
    }

    return mnt;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

static inode_t *vfs_search_(inode_t *root, inode_t *pwd, const char *path,
                            int *links)
{
    if (root == NULL || pwd == NULL) {
        errno = EINVAL;
        return NULL;
    }

    char *rent;
    char *path_cpy = strdup(path);
    char *fname = strtok_r(path_cpy, "/\\", &rent);
    inode_t *top = vfs_open(path[0] == '/' ? root : pwd);
    while (fname != NULL) {
        if (S_ISLNK(top->mode)) {
            vfs_close(top);
            kfree(path_cpy);
            errno = ENOSYS;
            return NULL;
        }
        if (!S_ISDIR(top->mode)) {
            vfs_close(top);
            kfree(path_cpy);
            errno = ENOTDIR;
            return NULL;
        }
        inode_t *ino = vfs_lookup(top, fname);
        vfs_close(top);
        if (ino == NULL) {
            kfree(path_cpy);
            errno = ENOENT;
            return NULL;
        }
        fname = strtok_r(NULL, "/\\", &rent);
        top = ino;
    }
    kfree(path_cpy);
    errno = 0;
    return top;
}

inode_t *vfs_search(inode_t *root, inode_t *pwd, const char *path)
{
    int links = 0;
    return vfs_search_(root, pwd, path, &links);
}

