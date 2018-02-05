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
#include <kernel/core.h>
#include <kernel/vfs.h>
#include <kernel/sys/inode.h>
#include <kernel/sys/device.h>
#include <kora/bbtree.h>
#include <string.h>
#include <errno.h>

extern vfs_fs_ops_t TMPFS_fs_ops;
extern vfs_dir_ops_t TMPFS_dir_ops;
extern vfs_io_ops_t TMPFS_io_ops;

size_t LBA_NB = 0;


typedef struct tmpfs_inode tmpfs_inode_t;

struct tmpfs_inode {
    inode_t ino;
    bbnode_t node;
    size_t parent;
    char name[256];
    // uid_t uid;
};

bbtree_t TMPFS_filetree;
tmpfs_inode_t *TMPFS_root = NULL;

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

inode_t *TMPFS_mount(inode_t *dev)
{
    if (dev != NULL) {
        errno = EBADF;
        return NULL;
    }

    if (!TMPFS_root) {
        bbtree_init(&TMPFS_filetree);
        int no = ++LBA_NB;
        tmpfs_inode_t *ino = (tmpfs_inode_t *)vfs_mountpt(no, "tmp", "tmpfs",
                             sizeof(tmpfs_inode_t));
        ino->parent = -1;
        ino->node.value_ = no;
        ino->ino.length = 0;
        // ino->ino.uid = 0;
        bbtree_insert(&TMPFS_filetree, &ino->node);
        ino->ino.dir_ops = &TMPFS_dir_ops;
        ino->ino.fs_ops = &TMPFS_fs_ops;

        TMPFS_root = ino;
    }

    kprintf(0, "TMPFS] Mount Root %d\n", TMPFS_root->node.value_);
    errno = 0;
    return &TMPFS_root->ino;
}

int TMPFS_unmount(inode_t *ino)
{
    errno = 0;
    return 0;
}

int TMPFS_close(tmpfs_inode_t *ino)
{
    if (ino/* ->present */) {
        kfree(ino);
        kprintf(0, "TMPFS] Free inode %d\n", ino->node.value_);
    }

    errno = 0;
    return 0;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

static tmpfs_inode_t *TMPFS_search(tmpfs_inode_t *dir, const char *name)
{
    tmpfs_inode_t *ino = bbtree_first(&TMPFS_filetree, tmpfs_inode_t, node);
    for (; ino; ino = bbtree_next(&ino->node, tmpfs_inode_t, node)) {
        if (ino->parent == dir->node.value_ && strcmp(ino->name, name) == 0) {
            return ino;
        }
    }
    return NULL;
}

inode_t *TMPFS_lookup(tmpfs_inode_t *dir, const char *name)
{
    tmpfs_inode_t *ino = TMPFS_search(dir, name);
    if (ino == NULL) {
        errno = ENOENT;
        return NULL;
    }

    errno = 0;
    return &ino->ino;
}

inode_t *TMPFS_create(tmpfs_inode_t *dir, const char *name, int mode)
{
    tmpfs_inode_t *ino = TMPFS_search(dir, name);
    if (ino) {
        errno = EEXIST;
        return NULL;
    }

    int no = ++LBA_NB;
    ino = (tmpfs_inode_t *)vfs_inode(no, mode, sizeof(tmpfs_inode_t));
    ino->parent = dir->node.value_;
    ino->node.value_ = no;
    ino->ino.length = 0;
    ino->ino.mode = mode;
    // ino->ino.uid = 0;
    bbtree_insert(&TMPFS_filetree, &ino->node);
    if (S_ISDIR(mode)) {
        ino->ino.dir_ops = &TMPFS_dir_ops;
    } else {
        ino->ino.io_ops = &TMPFS_io_ops;
    }
    ino->ino.fs_ops = &TMPFS_fs_ops;
    strncpy(ino->name, name, 256);

    kprintf(0, "TMPFS] File %d:%s under %d\n", ino->node.value_, name,
            dir->node.value_);
    errno = 0;
    return &ino->ino;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

// int TMPFS_read(inode_t* ino, const char *buf, size_t size, off_t offset)
// {
//   return -1;
// }

// int TMPFS_write(inode_t* ino, char *buf, size_t size, off_t offset)
// {
//   return -1;
// }

int TMPFS_truncate(tmpfs_inode_t *ino, off_t offset)
{
    ino->ino.length = offset;
    return 0;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

// int TMPFS_link(inode_t* dir, inode_t* ino, const char *name)
// {
//   return -1;
// }

int TMPFS_unlink(tmpfs_inode_t *dir, const char *name)
{
    tmpfs_inode_t *ino = TMPFS_search(dir, name);
    if (ino == NULL) {
        errno = ENOENT;
        return -1;
    }

    if (S_ISDIR(dir->ino.mode)) {
        // TODO Unsure directory is empty!
    }

    bbtree_remove(&TMPFS_filetree, ino->node.value_);
    kprintf(0, "TMPFS] Unlink entry %s under %d\n", name, dir->node.value_);
    errno = 0;
    return 0;
}


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


int TMPFS_setup()
{
    register_filesystem("tmpfs", &TMPFS_fs_ops);
    errno = 0;
    return 0;
}

int TMPFS_teardown()
{
    unregister_filesystem("tmpfs");
    errno = 0;
    return 0;
}


vfs_fs_ops_t TMPFS_fs_ops = {
    .mount = (fs_mount)TMPFS_mount,
    .unmount = TMPFS_unmount,

    // .close = (fs_close)TMPFS_close,
};

vfs_dir_ops_t TMPFS_dir_ops = {

    .lookup = (fs_lookup)TMPFS_lookup,

    .create = (fs_create)TMPFS_create,
    .mkdir = (fs_create)TMPFS_create,

    // .link = TMPFS_link,
    .unlink = (fs_unlink)TMPFS_unlink,
    .rmdir = (fs_unlink)TMPFS_unlink,
};


vfs_io_ops_t TMPFS_io_ops = {
    .read = NULL,
    // .read = TMPFS_read,
    // .write = TMPFS_write,
    // .truncate = TMPFS_truncate,
};

