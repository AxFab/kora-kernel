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
#include <kernel/files.h>
#include <kernel/device.h>
#include <kora/mcrs.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include "vfs.h"


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
    unsigned int cnt = atomic32_xadd(&ino->rcu, -1);
    if (cnt <= 1) {
        // TODO -- Close IO file
        // if (ino->dev != NULL) {
        //     vfs_dev_destroy(ino);
        // } else if (ino->fs != NULL) {
        //     vfs_mountpt_rcu_(ino->fs);
        // }
        kfree(ino);
        return;
    }

    // if (cnt == ino->links + 1) {
    //     kprintf(KLOG_DBG, "Only links for %d \n", ino->no);
    //     // vfs_dirent_rcu(ino);
    //     // Push dirent on LRU !?
    // }
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

/* Look for an inode, using root as file system root and pwd as local
 * directory.
 * The inode is found only if every directory on the way are accessible (--x).
 * Every path part must be valid UTF-8 names, length should be under
 * VFS_MAXNAME for a total length bellow VFS_MAXPATH. If symlinks are founded
 * they are resolved for a maximum of VFS_MAXREDIRECT redirections.
 * This routine must be called in a free-lock state and might trigger
 * file-systems calls.
 * The inode return will be considered used as concerned to RCU mechanism.
 */
inode_t *vfs_search(inode_t *root, inode_t *pwd, CSTR path, acl_t *acl)
{
    assert(path != NULL && strnlen(path, VFS_MAXPATH) < VFS_MAXPATH);
    inode_t *top = path[0] == '/' ? root : pwd;
    // TODO -- Should we look for `mnt:/`
    if (top == NULL || !VFS_ISDIR(top)) {
        errno = ENOTDIR;
        return NULL;
    } else if (vfs_access(top, X_OK, acl) != 0) {
        assert(errno == EACCES);
        return NULL;
    }

    int links = 0;
    return vfs_search_(top, path, acl, &links);
}

/* Create an empty inode (DIR or REG) */
inode_t *vfs_create(inode_t *dir, CSTR name, ftype_t type, acl_t *acl, int flags)
{
    inode_t *ino;
    assert(name != NULL && strnlen(name, VFS_MAXNAME) < VFS_MAXNAME);
    if (dir == NULL || !VFS_ISDIR(dir)) {
        errno = ENOTDIR;
        return NULL;
    } else if (vfs_access(dir, W_OK, acl) != 0) {
        assert(errno == EACCES);
        return NULL;
    } else if (type != FL_DIR && type != FL_REG) {
        errno = EINVAL;
        return NULL;
    } else if (dir->und.vol->flags & VFS_RDONLY) {
        errno = EROFS;
        return NULL;
    }

    /* Can we ask the file-system */
    if (dir->und.vol->ops->open == NULL) {
        errno = ENOSYS;
        return NULL;
    }

    /* Reserve a cache directory entry */
    dirent_t *ent = vfs_dirent_(dir, name, !(flags & VFS_BLOCK));
    if (ent == NULL) {
        assert(errno != 0);
        return NULL;
    }

    if (ent->ino) {
        if (!(flags & VFS_OPEN)) {
            errno = EEXIST;
            rwlock_rdunlock(&ent->lock);
            return NULL;
        } else if (type == FL_DIR && !VFS_ISDIR(ent->ino)) {
            errno = ENOTDIR;
            rwlock_rdunlock(&ent->lock);
            return NULL;
        } else if (type == FL_REG && VFS_ISDIR(ent->ino)) {
            errno = EISDIR;
            rwlock_rdunlock(&ent->lock);
            return NULL;
        } else if (type == FL_REG && ent->ino->type != FL_REG) {
            errno = EPERM;
            rwlock_rdunlock(&ent->lock);
            return NULL;
        }
        errno = 0;
        ino = ent->ino;
        rwlock_rdunlock(&ent->lock);
        return ino;
    }

    /* Request send to the file system */
    ino = dir->und.vol->ops->open(dir, name, type, acl, flags | VFS_CREAT);
    if (ino == NULL) {
        assert(errno != 0);
        return NULL;
    }

    vfs_record_(dir, ino);
    vfs_set_dirent_(ent, ino);
    errno = 0;
    rwlock_rdunlock(&ent->lock);
    return ino;
}

/* Link an inode (If supported) */
int vfs_link(inode_t *dir, CSTR name, inode_t *ino);

/* Unlink / delete an inode */
int vfs_unlink(inode_t *dir, CSTR name)
{
    assert(name != NULL && strnlen(name, VFS_MAXNAME) < VFS_MAXNAME);
    if (dir == NULL || !VFS_ISDIR(dir)) {
        errno = ENOTDIR;
        return -1;
    } else if (dir->und.vol->flags & VFS_RDONLY) {
        errno = EROFS;
        return -1;
    }

    /* Reserve a cache directory entry */
    dirent_t *ent = vfs_dirent_(dir, name, true);
    if (ent == NULL) {
        assert(errno != 0);
        return -1;
    }

    /* Can we ask the file-system */
    if (dir->und.vol->ops->unlink == NULL) {
        errno = ENOSYS;
        return -1;
    }

    /* Request send to the file system */
    int ret = dir->und.vol->ops->unlink(dir, name);
    if (ret == 0 || ent->ino == NULL)
        vfs_rm_dirent_(ent);
    else
        rwlock_rdunlock(&ent->lock);
    return ret;
}

/* Rename an inode (can use either link, rename or copy depending on fs) */
int vfs_rename(inode_t *dir, CSTR name, inode_t *ino);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

/* Create a symbolic link */
inode_t *vfs_symlink(inode_t *dir, CSTR name, CSTR path);
/* Read a link */
int vfs_readlink(inode_t *ino, char *buf, int len, int flags)
{
    buf[0] = ':';
    buf[1] = '\0';
    return -1;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

/* Update meta-data, owners */
int vfs_chown(inode_t *ino, acl_t *acl);
/* Update meta-data, access rights */
int vfs_chmod(inode_t *ino, int mode);
/* Update meta-data, times */
int vfs_chtimes(inode_t *ino, struct timespec *ts, int flags);

/* Update meta-data, size */
int vfs_truncate(inode_t *ino, off_t length)
{
	if (ino == NULL || ino->type != FL_REG) {
		errno = EINVAL;
		return -1;
	} else if (ino->und.vol->flags & VFS_RDONLY) {
        errno = EINVAL;
		return -1;
	}

	/* Can we ask the file-system */
    if (ino->ops->truncate == NULL) {
        errno = ENOSYS;
        return -1;
    }

    /* Request send to the file system */
    return ino->ops->truncate(ino, length);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

/* Check if a user have access to a file */
int vfs_access(inode_t *ino, int access, acl_t *acl)
{
    if (ino->acl == NULL) {
        errno = 0;
        return 0;
    }
    int shft = 0;
    if (ino->acl->uid == acl->uid)
        shft = 6;
    else if (ino->acl->gid == acl->gid)
        shft = 3;
    if ((ino->acl->mode >> shft) & access) {
        errno = 0;
        return 0;
    }
    errno = EACCES;
    return -1;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

#if 0
/* IO operations - read - only for BLK or REG */
int vfs_read(inode_t *ino, void *buf, size_t len, off_t off)
{
    assert(ino != NULL);
    assert(buf != NULL && len != 0);
    // assert(((size_t)buf & (PAGE_SIZE - 1)) == 0);
    // assert((len & (PAGE_SIZE - 1)) == 0);

    fs_read read = NULL;
    if (S_ISREG(ino->type == FL_REG)) {
        read = ino->fs->read;
        if (ino->dev->is_detached)
            read = NULL;
    } else if (S_ISBLK(ino->mode)) {
        read = ino->blk->read;
        if (ino->dev->is_detached)
            read = NULL;
    } else {
        errno = EINVAL;
        return -1;
    }

    if (read == NULL) {
        errno = ENOSYS;
        return -1;
    }
    // assert((off & (ino->fs->block - 1)) == 0);
    return read(ino, buf, len, off);
}

/* IO operations - write - only for BLK or REG */
int vfs_write(inode_t *ino, const void *buf, size_t len, off_t off)
{
    assert(ino != NULL);
    assert(buf != NULL && len != 0);
    assert(((size_t)buf & (PAGE_SIZE - 1)) == 0);
    assert((len & (PAGE_SIZE - 1)) == 0);

    fs_write write = NULL;
    bool ro = false;
    if (S_ISREG(ino->mode)) {
        ro = ino->dev->read_only;
        write = ino->fs->write;
        if (ino->dev->is_detached)
            write = NULL;
    } else if (S_ISBLK(ino->mode)) {
        ro = ino->dev->read_only;
        write = ino->blk->write;
        if (ino->dev->is_detached)
            write = NULL;
    } else {
        errno = EINVAL;
        return -1;
    }

    if (ro) {
        errno = EROFS;
        return -1;
    } else if (write == NULL) {
        errno = ENOSYS;
        return -1;
    }

    return write(ino, buf, len, off);
}
#endif

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


