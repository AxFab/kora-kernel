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
#include <errno.h>
#include <limits.h>

vfs_share_t *__vfs_share = NULL;
inode_t *devfs_setup();

void devfs_sweep();
void devfs_register(inode_t *ino, const char *name);

fs_anchor_t *vfs_init()
{
    assert(__vfs_share == NULL);
    fs_anchor_t *fsanchor = kalloc(sizeof(fs_anchor_t));
    __vfs_share = kalloc(sizeof(vfs_share_t));
    atomic_inc(&__vfs_share->rcu);
    hmp_init(&__vfs_share->fs_hmap, 16);

    inode_t *ino = devfs_setup();

    fnode_t *node = kalloc(sizeof(fnode_t));
    mtx_init(&node->mtx, mtx_plain);
    node->parent = NULL;
    node->ino = ino;
    node->rcu = 2;
    node->mode = FN_OK;

    __vfs_share->fsanchor = fsanchor;
    fsanchor->root = node;
    fsanchor->pwd = node;
    fsanchor->umask = 022;
    fsanchor->rcu = 1;
    return fsanchor;
}

fs_anchor_t *vfs_open_vfs(fs_anchor_t *fsanchor)
{
    atomic_inc(&fsanchor->rcu);
    return fsanchor;
}

fs_anchor_t *vfs_clone_vfs(fs_anchor_t *fsanchor)
{
    fs_anchor_t *cpy = kalloc(sizeof(fs_anchor_t));
    cpy->rcu = 1;
    atomic_inc(&__vfs_share->rcu);
    cpy->root = vfs_open_fnode(fsanchor->root);
    cpy->pwd = vfs_open_fnode(fsanchor->pwd);
    cpy->umask = fsanchor->umask;
    return cpy;
}

void vfs_close_vfs(fs_anchor_t *fsanchor)
{
    if (atomic_xadd(&fsanchor->rcu, -1) != 1)
        return;

    vfs_close_fnode(fsanchor->pwd);
    vfs_close_fnode(fsanchor->root);
    atomic_dec(&__vfs_share->rcu);
    kfree(fsanchor);
}

void vfs_dev_scavenge(device_t *dev, int max);

int vfs_sweep(fs_anchor_t *fsanchor)
{
    char tmp[16];
    vfs_close_vfs(fsanchor);
    if (__vfs_share->rcu != 0)
        return -1;

    kprintf(-1, "Destroy all VFS data\n");

    // Unmount all
    fnode_t *node = ll_first(&__vfs_share->mnt_list, fnode_t, nlru);
    while (node) {
        kprintf(KL_FSA, "Force unmount of '%s/%s'\n", vfs_inokey(node->parent->ino, tmp), node->name);
        vfs_umount_at(node, NULL, 0);
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
EXPORT_SYMBOL(vfs_mkdev, 0);

void vfs_rmdev(const char *name)
{
}
EXPORT_SYMBOL(vfs_rmdev, 0);

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
EXPORT_SYMBOL(vfs_addfs, 0);

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
EXPORT_SYMBOL(vfs_rmfs, 0);


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

int vfs_chdir(fs_anchor_t *fsanchor, const char *path, user_t *user, bool root)
{
    fnode_t *node = vfs_search(fsanchor, path, user, true, true);
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
        prev = fsanchor->root;
        fsanchor->root = node;
    } else {
        prev = fsanchor->pwd;
        fsanchor->pwd = node;
    }
    vfs_close_fnode(prev);

    // Check pwd is below root
    if (vfs_fnode_bellow(fsanchor->root, fsanchor->pwd) != 0) {
        vfs_close_fnode(fsanchor->pwd);
        fsanchor->pwd = vfs_open_fnode(fsanchor->root);
        if (!root)
            return -1;
    }
    return 0;
}

int vfs_readlink(fs_anchor_t *fsanchor, const char *name, user_t *user, char *buf, int len)
{
    fnode_t *node = vfs_search(fsanchor, name, user, true, true);
    if (node == NULL)
        return -1;

    inode_t *ino = vfs_inodeof(node);
    if (ino == NULL) {
        errno = ENOENT;
        return -1;
    } else if (ino->mode != FL_LNK) {
        vfs_close_inode(ino);
        errno = ENOLINK;
        return -1;
    }

    int ret = vfs_readsymlink(ino, buf, len);
    vfs_close_inode(ino);
    return ret;
}

int vfs_readpath(fs_anchor_t *fsanchor, const char *name, user_t *user, char *buf, int len, bool relative)
{
    fnode_t *node = vfs_search(fsanchor, name, user, true, true);
    if (node == NULL)
        return -1;

    if (node == fsanchor->root) {
        vfs_close_fnode(node);
        strncpy(buf, "/", len);
        return 0;
    } else if (relative && node == fsanchor->pwd) {
        vfs_close_fnode(node);
        strncpy(buf, ".", len);
        return 0;
    } else if (node->parent == NULL) {
        vfs_close_fnode(node);
        strncpy(buf, ":/", len);
        return 0;
    }

    char *cpy = kalloc(len);
    strncpy(cpy, node->name, len);
    for (;;) {
        node = node->parent;
        int ret = 0;
        if (node == fsanchor->root)
            ret = snprintf(buf, len, "/%s", cpy);
        else if (relative && node == fsanchor->pwd)
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

static void vfs_pipe_close(inode_t *ino)
{
    // TODO -- release unique NO
}

ino_ops_t pipe_ino_ops = {
    .read = NULL,
    .close = vfs_pipe_close,
};

inode_t *vfs_pipe()
{
    // TODO -- Assign uniq NO -- Keep one device only -- No need to be use in kmap!
    inode_t *ino = vfs_inode(1, FL_PIPE, NULL, &pipe_ino_ops);
    ino->dev->block = 1;
    return ino;
}
EXPORT_SYMBOL(vfs_pipe, 0);
