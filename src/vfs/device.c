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

llhead_t devices_list = INIT_LLHEAD;
HMP_map devices_map;
splock_t devices_lock;

void vfs_init()
{
    hmp_init(&devices_map, 16);
    splock_init(&devices_lock);
}

void vfs_fini()
{
    hmp_destroy(&devices_map, 0);
}

void vfs_show_devices()
{
    inode_t *ino;
    splock_lock(&devices_lock);
    for ll_each(&devices_list, ino, inode_t, lnode) {
        kprintf(-1, "DEV '%s' / %s (RCU:%d)\n", ino->und.dev->devname, ino->und.dev->model, ino->rcu);
    }
    splock_unlock(&devices_lock);
}

inode_t *vfs_search_device(CSTR name)
{
    splock_lock(&devices_lock);
    inode_t *ino = (inode_t*)hmp_get(&devices_map, name, strlen(name));
    splock_unlock(&devices_lock);
    return vfs_open(ino);
}

int vfs_mkdev(inode_t *ino, CSTR name)
{
    assert(ino != NULL && name != NULL);
    if (ino->type == FL_BLK && ino->length)
        kprintf(KLOG_MSG, "%s %s %s <\033[33m%s\033[0m>\n", ino->und.dev->devclass,
            ino->und.dev->model ? ino->und.dev->model : "", sztoa(ino->length), name);
    else
        kprintf(KLOG_MSG, "%s %s <\033[33m%s\033[0m>\n", ino->und.dev->devclass,
            ino->und.dev->model ? ino->und.dev->model : "", name);

    vfs_open(ino);
    splock_lock(&devices_lock);
    ll_append(&devices_list, &ino->lnode);
    hmp_put(&devices_map, name, strlen(name), ino);
    splock_unlock(&devices_lock);
    // TODO -- Use id to check if we know the device
    return 0;
}

void vfs_rmdev(CSTR name)
{
    inode_t *dev = vfs_search_device(name);
    if (dev == NULL)
        return;
    splock_lock(&devices_lock);
    ll_remove(&devices_list, &dev->lnode);
    hmp_remove(&devices_map, name, strlen(name));
    splock_unlock(&devices_lock);
    vfs_close(dev);
}

