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
#include <assert.h>
#include <string.h>
#include <errno.h>
#include "vfs.h"

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

/* The function look into the VFS hash table for a directory entry and if not
 * existing reserve one.
 * The function might block if this entry is in creation state.
 * The entry is on creation state after this method create it until a call to
 * `vfs_set_dirent_' or `vfs_rm_dirent_' which will set the status of the
 * inode.
 * The function return NULL in three cases, `block' argument is false and the
 * function will block, the function have been blocked but is release by
 * `vfs_rm_dirent_' in which case the inode doesn't exist anyway, or the
 * arguments are invalid.
 */
dirent_t *vfs_dirent_(inode_t *dir, CSTR name, bool block)
{
    assert(dir != NULL && VFS_ISDIR(dir));
    assert(name != NULL && strnlen(name, VFS_MAXNAME) < VFS_MAXNAME);

    /* Build a unique key for the directory */
    int lg = strlen(name) + 4;
    char *key = kalloc(lg + 1);
    ((int *)key)[0] = dir->no;
    strcpy(&key[4], name);

    volume_t *fs = dir->und.vol;

    splock_lock(&fs->lock);
    dirent_t *ent = (dirent_t *)hmp_get(&fs->hmap, key, lg);
    if (ent) {
        // Remove from LRU
        ll_remove(&fs->lru, &ent->lru);
        ll_enqueue(&fs->lru, &ent->lru);
        rwlock_rdlock(&ent->lock);
    }
    if (ent == NULL) {
        ent = (dirent_t *)kalloc(sizeof(dirent_t));
        ent->parent = dir;
        ent->/*map_ent.*/lg = lg;
        memcpy(ent->key, key, lg + 1);
        hmp_put(&fs->hmap, key, lg, ent);
        ll_enqueue(&fs->lru, &ent->lru);
        rwlock_init(&ent->lock);
        rwlock_rdlock(&ent->lock);
    } else if (ent->ino == NULL) {
        // if (!block) {
        kfree(key);
        splock_unlock(&fs->lock);
        errno = EWOULDBLOCK;
        return NULL;
        // }

        // TODO - Wait until
    }
    kfree(key);
    splock_unlock(&fs->lock);
    return ent;
}


void vfs_set_dirent_(dirent_t *ent, inode_t *ino)
{
    assert(ent != NULL);
    assert(ino != NULL);

    ent->ino = vfs_open(ino);
    atomic_inc(&ino->links);
    ll_append(&ino->dlist, &ent->node);
    // mountfs_t *fs = ent->parent->fs;
    // splock_lock(&fs->lock);
    // TODO -- Wake up other !
    // splock_unlock(&fs->lock);
}

void vfs_rm_dirent_(dirent_t *ent)
{
    assert(ent != NULL);

    volume_t *fs = ent->parent->und.vol;
    splock_lock(&fs->lock);
    hmp_remove(&fs->hmap, ent->key, ent->lg);
    rwlock_rdunlock(&ent->lock);
    // TODO -- Wake up other !
    rwlock_wrlock(&ent->lock);
    if (ent->ino != NULL) {
        vfs_close(ent->ino);
        ent->ino = NULL;
    }
    kprintf(KLOG_INO, "Freeing %p\n", ent);
    rwlock_wrunlock(&ent->lock);
    //kfree(ent);

    splock_unlock(&fs->lock);
}

void vfs_dirent_rcu_(inode_t *ino)
{
    // Push all links into RCU
}


void vfs_sweep(volume_t *fs, int max)
{
    /* Lock so that nobody can access new dirent_t */
    splock_lock(&fs->lock);
    dirent_t *ent = ll_first(&fs->lru, dirent_t, lru);
    while (max > 0 && ent) {
        /* Check nobody is currently using this entry */
        if (!rwlock_wrtrylock(&ent->lock)) {
            ent = ll_next(&ent->lru, dirent_t, lru);
            continue;
        }
        dirent_t *rm = ent;
        ent = ll_next(&ent->lru, dirent_t, lru);
        hmp_remove(&fs->hmap, rm->key, rm->lg);
        atomic_dec(&rm->ino->links);
        ll_remove(&fs->lru, &rm->lru);
        vfs_close(rm->ino);
        kfree(rm);
        --max;
    }
    if (ent != NULL)
        splock_unlock(&fs->lock);
}



/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void vfs_record_(inode_t *dir, inode_t *ino)
{
    assert(dir != NULL && VFS_ISDIR(dir));
    assert(ino != NULL);
    // assert(ino->rcu == 1);
    // assert(ino->links == 0);
    assert(ino->no != 0);
    assert(ino->ops != NULL);
}

dirent_t *vfs_lookup_(inode_t *dir, CSTR name)
{
    assert(dir != NULL && VFS_ISDIR(dir));

    if (dir->und.vol->ops->open == NULL) {
        errno = ENOSYS;
        return NULL;
    }

    dirent_t *ent = vfs_dirent_(dir, name, true);
    if (ent == NULL) {
        assert(errno != 0);
        return NULL;
    } else if (ent->ino == NULL) {
        // TODO -- We can't - lock on entry (rdlock) !?
        inode_t *ino = dir->und.vol->ops->open(dir, name, 0, NULL, VFS_OPEN);
        if (ino == NULL) {
            assert(errno != 0);
            vfs_rm_dirent_(ent);
            return NULL;
        }

        vfs_record_(dir, ino);
        vfs_set_dirent_(ent, ino);
        vfs_close(ino);
    }
    errno = 0;
    return ent;
}


inode_t *vfs_search_(inode_t *ino, CSTR path, acl_t *acl, int *links)
{
    assert(ino != NULL && path != NULL);

    char *rent;
    char *path_cpy = strdup(path);
    char *fname = strtok_r(path_cpy, "/\\", &rent);
    ino = vfs_open(ino);
    while (fname != NULL) {
        if (ino->type == FL_LNK) {
            // TODO -- Follow symbolic link
            (*links)++;
            vfs_close(ino);
            kfree(path_cpy);
            errno = ENOSYS;
            return NULL;
        } else if (!VFS_ISDIR(ino)) {
            vfs_close(ino);
            kfree(path_cpy);
            errno = ENOTDIR;
            return NULL;
        } else if (vfs_access(ino, X_OK, acl) != 0) {
            vfs_close(ino);
            kfree(path_cpy);
            errno = EACCES;
            return NULL;
        }
        dirent_t *ent = vfs_lookup_(ino, fname);
        if (ent == NULL) {
            vfs_close(ino);
            kfree(path_cpy);
            errno = ENOENT;
            // assert(errno != 0);
            return NULL;
        }
        vfs_open(ent->ino);
        vfs_close(ino);
        ino = ent->ino;
        rwlock_rdunlock(&ent->lock);
        fname = strtok_r(NULL, "/\\", &rent);
    }
    kfree(path_cpy);
    errno = 0;
    return ino;
}

inode_t *vfs_lookup(inode_t *dir, CSTR name)
{
    assert(dir != NULL && name != NULL);
    dirent_t *ent = vfs_lookup_(dir, name);
    if (ent == NULL) {
        // assert(errno != 0);
        return NULL;
    }
    inode_t *ino = ent->ino;
    rwlock_rdunlock(&ent->lock);
    return vfs_open(ino);
}
