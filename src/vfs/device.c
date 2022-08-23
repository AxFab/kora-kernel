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
#include <kora/hmap.h>
#include <errno.h>
#include <assert.h>
#include <kernel/stdc.h>
#include <kernel/vfs.h>
#include <errno.h>
#include <stdbool.h>
#include "fnode.h"
#include <errno.h>
#include <limits.h>

vfs_share_t *__vfs_share = NULL;
inode_t *devfs_setup();

void devfs_sweep();
void devfs_register(inode_t *ino, const char *name);

vfs_t *vfs_init()
{
    assert(__vfs_share == NULL);
    vfs_t *vfs = kalloc(sizeof(vfs_t));
    __vfs_share = kalloc(sizeof(vfs_share_t));
    vfs->share = __vfs_share;
    atomic_inc(&vfs->share->rcu);
    vfs->share->dev_no = 1;
    hmp_init(&vfs->share->fs_hmap, 16);

    inode_t *ino = devfs_setup();

    fnode_t *node = kalloc(sizeof(fnode_t));
    mtx_init(&node->mtx, mtx_plain);
    node->parent = NULL;
    node->ino = vfs_open_inode(ino);
    node->rcu = 2;
    node->mode = FN_OK;

    vfs->root = node;
    vfs->pwd = node;
    vfs->umask = 022;
    vfs->rcu = 1;
    vfs_close_inode(ino);

    return vfs;
}

vfs_t *vfs_open_vfs(vfs_t *vfs)
{
    atomic_inc(&vfs->rcu);
    return vfs;
}

vfs_t *vfs_clone_vfs(vfs_t *vfs)
{
    vfs_t *cpy = kalloc(sizeof(vfs_t));
    cpy->rcu = 1;
    cpy->share = vfs->share;
    atomic_inc(&vfs->share->rcu);
    cpy->root = vfs_open_fnode(vfs->root);
    cpy->pwd = vfs_open_fnode(vfs->pwd);
    cpy->umask = vfs->umask;
    return cpy;
}
void vfs_close_vfs(vfs_t *vfs)
{
    if (atomic_xadd(&vfs->rcu, -1) != 1)
        return;

    vfs_close_fnode(vfs->pwd);
    vfs_close_fnode(vfs->root);
    atomic_dec(&vfs->share->rcu);
    kfree(vfs);
}

void vfs_dev_scavenge(device_t *dev, int max);

int vfs_sweep(vfs_t *vfs)
{
    vfs_close_vfs(vfs);
    if (__vfs_share->rcu != 0)
        return -1;

    kprintf(-1, "Destroy all VFS data\n");

    // Unmount all
    fnode_t *node = ll_first(&__vfs_share->mnt_list, fnode_t, nlru);
    while (node) {
        inode_t *ino = vfs_inodeof(node);
        vfs_umount_at(node, NULL, 0);
        vfs_dev_scavenge(ino->dev, INT_MAX);
        vfs_close_inode(ino);
        node = ll_first(&__vfs_share->mnt_list, fnode_t, nlru);
    }

    // TODO -- Remove all devices

    // Scavenge all
    // vfs_scavenge(INT_MAX);
    devfs_sweep();

    if (__vfs_share->fs_hmap.count != 0)
        return -1;
    hmp_destroy(&__vfs_share->fs_hmap);
    assert(__vfs_share->rcu == 0);
    kfree(__vfs_share);
    __vfs_share = NULL;
    return 0;
}

int vfs_mkdev(/* fnode_t *parent, */inode_t *ino, const char *name)
{
    devfs_register(ino, name);
    return 0;
}

void vfs_rmdev(const char *name)
{
}


void vfs_addfs(const char *name, fsmount_t mount, fsformat_t format)
{
    fsreg_t *reg = kalloc(sizeof(fsreg_t));
    strncpy(reg->name, name, 16);
    reg->mount = mount;
    reg->format = format;
    splock_lock(&__vfs_share->lock);
    hmp_put(&__vfs_share->fs_hmap, name, strlen(name), reg);
    splock_unlock(&__vfs_share->lock);
}

void vfs_rmfs(const char *name)
{
    splock_lock(&__vfs_share->lock);
    fsreg_t *fs = hmp_get(&__vfs_share->fs_hmap, name, strlen(name));
    if (fs == NULL) {
        splock_unlock(&__vfs_share->lock);
        return;
    }
    hmp_remove(&__vfs_share->fs_hmap, name, strlen(name));
    splock_unlock(&__vfs_share->lock);
    kfree(fs);
}

int vfs_format(const char *name, inode_t *dev, const char *options)
{
    splock_lock(&__vfs_share->lock);
    fsreg_t *fs = hmp_get(&__vfs_share->fs_hmap, name, strlen(name));
    splock_unlock(&__vfs_share->lock);
    if (fs == NULL)
        return -2;
    return fs->format(dev, options);
}



int vfs_fnode_bellow(fnode_t *root, fnode_t *dir);

int vfs_chdir(vfs_t *vfs, const char *path, user_t *user, bool root)
{
    fnode_t *node = vfs_search(vfs, path, user, true, true);
    if (node == NULL)
        return -1;
    inode_t *ino = vfs_inodeof(node);
    if (ino == NULL) {
        errno = ENOENT;
        return -1;
    }
    if (ino->type != FL_DIR) {
        vfs_close_fnode(node);
        vfs_close_inode(ino);
        errno = ENOTDIR;
        return -1;
    }
    vfs_close_inode(ino);
    fnode_t *prev;
    if (root) {
        prev = vfs->root;
        vfs->root = node;
    } else {
        prev = vfs->pwd;
        vfs->pwd = node;
    }
    vfs_close_fnode(prev);

    // Check pwd is below root
    if (vfs_fnode_bellow(vfs->root, vfs->pwd) != 0) {
        vfs_close_fnode(vfs->pwd);
        vfs->pwd = vfs_open_fnode(vfs->root);
    }
    return 0;
}


int vfs_readpath(vfs_t *vfs, fnode_t *node, char *buf, int len, bool relative)
{
    if (node == vfs->root)
        return strncpy(buf, "/", len) != NULL ? 0 : -1;
    if (relative && node == vfs->pwd)
        return strncpy(buf, ".", len) != NULL ? 0 : -1;
    if (node->parent == NULL)
        return strncpy(buf, ":/", len) != NULL ? 0 : -1;

    char *cpy = kalloc(len);
    strncpy(cpy, node->name, len);
    for (;;) {
        node = node->parent;
        int ret = 0;
        if (node == vfs->root)
            ret = snprintf(buf, len, "/%s", cpy);
        else if (relative && node == vfs->pwd)
            ret = snprintf(buf, len, "./%s", cpy);
        else if (node->parent == NULL)
            ret = snprintf(buf, len, ":/%s", cpy);
        if (ret != 0) {
            kfree(cpy);
            return ret > 0 ? 0 : -1;
        }
        if (snprintf(buf, len, "%s/%s", node->name, cpy) < 0) {
            kfree(cpy);
            return -1;
        }
        strncpy(cpy, buf, len);
    }
}



/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
ino_ops_t pipe_ino_ops = {
    .read = NULL,
};

fnode_t *__private;

fnode_t *vfs_mkfifo(fnode_t *parent, const char *name)
{
    int i;
    if (__private == NULL) {
        __private = kalloc(sizeof(fnode_t));
        mtx_init(&__private->mtx, mtx_plain);
        __private->parent = NULL;
        __private->ino = vfs_inode(1, FL_DIR, NULL, &pipe_ino_ops);
        __private->rcu = 1;
        __private->mode = FN_OK;
    }
    if (parent == NULL)
        parent = __private;

    fnode_t *node = NULL;
    char *fname = NULL;
    if (name == NULL) {
        fname = kalloc(17);
        for (i = 0; i < 16; ++i)
            fname[i] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_-"[rand8() % 64];
        fname[16] = 0;
        node = vfs_fsnode_from(parent, fname);
        kfree(fname);
    } else
        node = vfs_fsnode_from(parent, name);

    mtx_lock(&node->mtx);
    if (node->mode != FN_EMPTY) {
        mtx_unlock(&node->mtx);
        vfs_close_fnode(node);
        errno = EEXIST;
        return NULL;
    }

    device_t *dev = NULL;
    ino_ops_t *ops = &pipe_ino_ops;
    int no = 1;
    int type = FL_PIPE;

    inode_t *ino = vfs_inode(no, type, dev, ops);

    vfs_resolve(node, ino);
    mtx_unlock(&node->mtx);
    return node;
}


EXPORT_SYMBOL(vfs_mkdev, 0);
EXPORT_SYMBOL(vfs_rmdev, 0);
EXPORT_SYMBOL(vfs_addfs, 0);
EXPORT_SYMBOL(vfs_rmfs, 0);
