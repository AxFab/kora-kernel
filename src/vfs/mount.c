/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2018  <Fabien Bavent>
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
#include <kernel/device.h>
#include <string.h>
#include <errno.h>

device_t *vfs_lookup_device_(CSTR name);



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

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int vfs_mountpt(CSTR name, CSTR fsname, fsvolume_t *fs, inode_t *ino)
{
    fs->name = strdup(name);
    fs->fsname = strdup(fsname);
    fs->rcu = 2; // ROOT + DRIVER
    ino->fs = fs;
    fs->dev.ino = vfs_open(ino);
    hmp_init(&fs->hmap, 16);
    splock_init(&fs->dev.lock);
    return 0;
}

void vfs_mountpt_rcu_(fsvolume_t *fs)
{
    if (atomic32_xadd(&fs->rcu, -1) > 1)
        return;
    kfree(fs->name);
    kfree(fs->fsname);
    hmp_destroy(&fs->hmap, 0);
    kfree(fs);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

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

    kprintf(KLOG_MSG, "Mount %s as \e[35m%s\e[0m (%s)\n", dev, ino->fs->name,
            ino->fs->fsname);
    errno = 0;
    return ino;
}

int vfs_umount(inode_t *ino)
{
    fsvolume_t *fs = ino->fs;
    if (fs->dev.ino != ino) {
        errno = EINVAL;
        return -1;
    }

    errno = 0;
    if (fs->umount)
        fs->umount(ino);
    vfs_close(fs->dev.ino);
    fs->dev.is_detached = true;
    vfs_mountpt_rcu_(fs);
    return 0;
}

