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
#include <errno.h>
#include <kernel/vfs.h>
#include <kernel/device.h>
#include "../check.h"


/* An inode must be created by the driver using a call to `vfs_inode()' */
inode_t *vfs_inode(unsigned no, ftype_t type, volume_t *volume)
{
    inode_t *inode = (inode_t *)kalloc(sizeof(inode_t));
    inode->no = no;
    inode->type = type;

    inode->rcu = 1;
    inode->links = 0;

    switch (type) {
    case FL_REG:  /* Regular file (FS) */
    case FL_DIR:  /* Directory (FS) */
    case FL_LNK:  /* Symbolic link (FS) */
        assert(volume != NULL);
        inode->und.vol = volume;
        break;
    case FL_VOL:  /* File system volume */
        assert(volume == NULL);
        inode->und.vol = kalloc(sizeof(volume_t));
        hmp_init(&inode->und.vol->hmap, 16);
        break;
    case FL_BLK:  /* Block device */
    case FL_CHR:  /* Char device */
    case FL_VDO:  /* Video stream */
        assert(volume == NULL);
        inode->und.dev = kalloc(sizeof(device_t));
        break;
    case FL_PIPE:  /* Pipe */
    case FL_NET:  /* Network interface */
    case FL_SOCK:  /* Network socket */
    case FL_INFO:  /* Information file */
    case FL_SFC:  /* Application surface */
    case FL_TTY:  /* Terminal (Virtual) */
    case FL_WIN:  /* Window (Virtual) */
    default:
        assert(false);
        break;
    }
    return inode;
}

/* Open an inode - increment usage as concerned to RCU mechanism. */
inode_t *vfs_open(inode_t *ino)
{
    if (ino)
        atomic_inc(&ino->rcu);
    return ino;
}

/* Close an inode - decrement usage as concerned to RCU mechanism.
* If usage reach zero, the inode is freed.
* If usage is equals to number of dirent links, they are all pushed to LRU.
*/
void vfs_close(inode_t *ino)
{
    if (ino == NULL)
        return;
    unsigned int cnt = atomic_fetch_sub(&ino->rcu, 1);
    if (cnt <= 1) {
        // TODO -- Close IO file
        // if (ino->dev != NULL) {
        //     vfs_dev_destroy(ino);
        // } else if (ino->fs != NULL) {
        //     vfs_mountpt_rcu_(ino->fs);
        // }
        switch (ino->type) {
        case FL_REG:  /* Regular file (FS) */
        case FL_DIR:  /* Directory (FS) */
        case FL_LNK:  /* Symbolic link (FS) */
            //
            break;
        case FL_VOL:  /* File system volume */
            hmp_destroy(&ino->und.vol->hmap, 0);
            kfree(ino->und.vol);
            break;
        case FL_BLK:  /* Block device */
        case FL_CHR:  /* Char device */
        case FL_VDO:  /* Video stream */
            kfree(ino->und.dev);
            break;
        case FL_PIPE:  /* Pipe */
        case FL_NET:  /* Network interface */
        case FL_SOCK:  /* Network socket */
        case FL_INFO:  /* Information file */
        case FL_SFC:  /* Application surface */
        case FL_TTY:  /* Terminal (Virtual) */
        case FL_WIN:  /* Window (Virtual) */
        default:
            break;
        };
        kfree(ino);
        return;
    }

    // if (cnt == ino->links + 1) {
    //     kprintf(KLOG_DBG, "Only links for %d \n", ino->no);
    //     // vfs_dirent_rcu(ino);
    //     // Push dirent on LRU !?
    // }
}


int vfs_readlink(inode_t *ino, char *buf, int len, int flags)
{
    return -1;
}

int vfs_read(inode_t *ino, char *buf, size_t lg, off_t off, int flags)
{
    errno = EIO;
    return -1;
}

int vfs_write(inode_t *ino, const char *buf, size_t lg, off_t off, int flags)
{
    errno = EIO;
    return -1;
}
