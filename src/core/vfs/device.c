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
        kprintf(-1, "%s %s %s <\e[33m%s\e[0m>\n", dev->class, dev->vendor ? dev->vendor : "", sztoa(ino->length), name);
    else
        kprintf(-1, "%s %s <\e[33m%s\e[0m>\n", dev->class, dev->vendor ? dev->vendor : "", name);

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
    dev->is_detached = true;
    vfs_close(ino);
}

void vfs_dev_destroy(inode_t *ino)
{
    device_t *dev = ino->dev; // !?
    ll_remove(&dev_list, &dev->node);
    kfree(dev->name);
    if (ino->dev->release != NULL)
        ino->dev->release(ino->dev);
}

