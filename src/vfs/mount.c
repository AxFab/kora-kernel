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

inode_t *vfs_mount(CSTR devname, CSTR fs)
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

    device_t *dev = NULL;
    if (devname) {
        dev = vfs_search_device(devname);
        if (dev == NULL) {
            errno = ENODEV;
            return NULL;
        }
    }

    inode_t *ino = mount(dev);
    if (ino == NULL) {
        assert(errno != 0);
        return NULL;
    }

    assert(ino->type == FL_VOL);
    kprintf(KLOG_MSG, "Mount %s as \033[35m%s\033[0m (%s)\n", devname, ino->und.vol->volname, ino->und.vol->volfs);
    errno = 0;
    return ino;
}

int vfs_umount(inode_t *ino)
{
    assert(ino == FL_VOL);
    volume_t *fs = ino->und.vol;

    errno = 0;
    if (ino->ops->close)
        ino->ops->close(ino);
    vfs_close(ino);
    return 0;
}

