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
#include <kora/hmap.h>
#include <errno.h>
#include <assert.h>
#include <kernel/stdc.h>
#include <kernel/vfs.h>

hmap_t fs_hmap;

inode_t *devfs_setup();
void devfs_register(inode_t *ino, const char *name);

vfs_t *vfs_init()
{
    vfs_t *vfs = kalloc(sizeof(vfs_t));
    inode_t *ino = devfs_setup();

    fsnode_t *node = kalloc(sizeof(fsnode_t));
    mtx_init(&node->mtx, mtx_plain);
    node->parent = NULL;
    node->ino = vfs_open_inode(ino);
    node->rcu = 3;
    node->mode = FN_OK;

    vfs->root = node;
    vfs->pwd = node;
    vfs->umask = 022;
    vfs->rcu = 1;
    vfs_close_inode(ino);

    hmp_init(&fs_hmap, 16);
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
    cpy->root = vfs_open_fsnode(vfs->root);
    cpy->pwd = vfs_open_fsnode(vfs->pwd);
    cpy->umask = vfs->umask;
    return cpy;
}

void vfs_dev_scavenge(device_t *dev, int max);
void vfs_sweep(vfs_t *vfs)
{
    // fsnode_t* node = vfs->root;
    vfs_close_fsnode(vfs->pwd);
    vfs_close_fsnode(vfs->root);
    // TODO --- Scavenge for all devices
    vfs_dev_scavenge(vfs->pwd->ino->dev, 20);
    vfs_dev_scavenge(vfs->root->ino->dev, 20);

    kfree(vfs);
    kprintf(-1, "Destroy all VFS data\n");
}

int vfs_mkdev(inode_t *ino, const char *name)
{
    devfs_register(ino, name);
    return 0;
}

void vfs_rmdev(const char *name)
{
}

void vfs_addfs(const char *name, fsmount_t mount)
{
    hmp_put(&fs_hmap, name, strlen(name), mount);
}

void vfs_rmfs(const char *name)
{
    hmp_remove(&fs_hmap, name, strlen(name));
}

inode_t *vfs_mount(vfs_t *vfs, const char *devname, const char *fs, const char *name, const char *options)
{
    assert(devname != NULL && fs != NULL);
    fsmount_t mount = (fsmount_t)hmp_get(&fs_hmap, fs, strlen(fs));
    if (mount == NULL) {
        errno = ENOSYS;
        return NULL;
    }

    fsnode_t *dev = NULL;
    if (devname != NULL) {
        dev = vfs_search(vfs, devname, NULL, true);

        if (dev == NULL) {
            errno = ENODEV;
            return NULL;
        }
    }

    inode_t *ino = mount(dev ? dev->ino : NULL, options);
    vfs_close_fsnode(dev);
    if (ino == NULL) {
        assert(errno != 0);
        return NULL;
    }

    if (ino->type != FL_DIR) {
        vfs_close_inode(ino);
        errno = ENOTDIR;
        return NULL;
    }

    // TODO -- Should return a fsnode or nothing !
    vfs_mkdev(ino, name);
    errno = 0;
    return ino;
}

// int vfs_umount(inode_t *ino)
// {
//     assert(ino->type == FL_VOL);
//     device_t *volume = ino->dev;
//     errno = 0;
//     if (ino->ops->close)
//         ino->ops->close(ino);
//     inode_t *file;
//     for bbtree_each(&volume->btree, file, inode_t, bnode)
//         kprintf(KLOG_INO, "Need rmlink of %3x\n", file->no);
//     // vfs_close_inode(ino, X_OK);
//     return 0;
// }

int vfs_chdir(vfs_t *vfs, const char *path, bool root)
{
    fsnode_t *node = vfs_search(vfs, path, NULL, true);
    if (node == NULL)
        return -1;
    if (node->ino->type != FL_DIR) {
        vfs_close_fsnode(node);
        errno = ENOTDIR;
        return -1;
    }
    fsnode_t *prev;
    if (root) {
        prev = vfs->root;
        vfs->root = node;
        // TODO -- Don't allow PWD to be over chroot
        // (What about opened files with `at` access !?)
    } else {
        prev = vfs->pwd;
        vfs->pwd = node;
    }
    vfs_close_fsnode(prev);
    return 0;
}

int vfs_readlink(vfs_t *vfs, fsnode_t *node, char *buf, int len, bool relative)
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

