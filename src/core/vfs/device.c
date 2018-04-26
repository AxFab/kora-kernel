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
#include <kernel/mods/fs.h>
#include <string.h>
#include <errno.h>

llhead_t dev_list = INIT_LLHEAD;


device_t *vfs_lookup_device_(CSTR name)
{
    device_t *dev;
    for ll_each(&dev_list, dev, device_t, node) {
        if (!strcmp(name, dev->name)) {
            return dev;
        }
    }
    return NULL;
}

int vfs_mkdev(CSTR name, device_t *dev, inode_t *ino)
{
    if (dev == NULL || ino == NULL) {
        errno = EINVAL;
        return -1;
    }

    dev->name = strdup(name);
    dev->ino = vfs_open(ino);
    ino->dev = dev;
    if (S_ISBLK(ino->mode) && ino->length)
        kprintf(-1, "%s %s %s <%s>\n", dev->class, dev->vendor ? dev->vendor : "", sztoa(ino->length), name);
    else
        kprintf(-1, "%s %s <%s>\n", dev->class, dev->vendor ? dev->vendor : "", name);

    ll_append(&dev_list, &dev->node);
    // TODO -- Use id to check if we know the device
    return 0;
}

void vfs_rmdev(CSTR name)
{
    device_t *dev = vfs_lookup_device_(name);
    if (dev == NULL)
        return;
    inode_t *ino = dev->ino;
    vfs_close(ino);
    // ino->dev = NULL; // !?
    ll_remove(&dev_list, &dev->node);
    kfree(dev->name);
}


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

bool fs_init = false;
HMP_map fs_hmap;

void register_fs(CSTR name, fs_mount mount)
{
    if (!fs_init) {
        hmp_init(&fs_hmap, 16);
        fs_init = true;
    }
    hmp_put(&fs_hmap, name, strlen(name), mount);
}

void unregister_fs(CSTR name)
{
    if (!fs_init)
        return;
    hmp_remove(&fs_hmap, name, strlen(name));
    if (fs_hmap.count_ == 0) {
        hmp_destroy(&fs_hmap, 0);
        fs_init = false;
    }
}

int vfs_mountpt(CSTR name, CSTR fsname, mountfs_t *fs, inode_t *ino)
{
    fs->name = strdup(name);
    fs->fsname = strdup(fsname);
    fs->rcu = 2; // ROOT + DRIVER
    ino->fs = fs;
    fs->root = vfs_open(ino);
    hmp_init(&fs->hmap, 16);
    splock_init(&fs->lock);
    return 0;
}

void vfs_mountpt_rcu_(mountfs_t *fs)
{
    if (atomic_fetch_add(&fs->rcu, -1) > 1)
        return;
    kfree(fs->name);
    kfree(fs->fsname);
    hmp_destroy(&fs->hmap, 16);
    splock_init(&fs->lock);
    kfree(fs);
}

inode_t *vfs_mount(CSTR dev, CSTR fs)
{
    if (!fs_init) {
        errno = ENOSYS;
        return NULL;
    }

    fs_mount mount = (fs_mount)hmp_get(&fs_hmap, fs, strlen(fs));
    if (mount == NULL) {
        errno = ENOSYS;
        return NULL;
    }

    device_t *devc = NULL;
    if (dev) {
        devc = vfs_lookup_device_(dev);
        if (devc == NULL) {
            errno = ENODEV;
            return NULL;
        }
    }

    inode_t *ino = mount(devc ? devc->ino : NULL);
    if (ino == NULL) {
        assert(errno != 0);
        return NULL;
    }

    kprintf(-1, "Mount %s as %s (%s)\n", dev, ino->fs->name, ino->fs->fsname);
    errno = 0;
    return ino;
}

int vfs_umount(inode_t *ino)
{
    mountfs_t *fs = ino->fs;
    if (fs->root != ino) {
        errno = EINVAL;
        return -1;
    }

    if (fs->umount)
        fs->umount(ino);
    vfs_close(fs->root);
    fs->root = NULL;
    fs->lookup = NULL;
    fs->read = NULL;
    fs->umount = NULL;
    fs->open = NULL;
    fs->close = NULL;
    vfs_mountpt_rcu_(fs);
    return 0;
}
