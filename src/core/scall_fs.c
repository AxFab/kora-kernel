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
#include <kernel/tasks.h>
#include <kernel/memory.h>
#include <kernel/vfs.h>
#include <errno.h>
#include <kernel/syscalls.h>

#include <bits/mman.h>
#include <bits/io.h>

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

long sys_fstat(const char *path, struct filemeta *meta, int flags)
{
    if (mspace_check_str(__current->vm, path, 4096) != 0)
        return -1;

    fnode_t *node = vfs_search(__current->fsa, path, NULL, true, true);
    if (node == NULL)
        return -1;

    meta->ino = node->ino->no;
    meta->ftype = node->ino->type;
    meta->size = node->ino->length;
    meta->ctime = node->ino->ctime;
    meta->mtime = node->ino->mtime;
    meta->atime = node->ino->atime;
    meta->btime = node->ino->btime;
    vfs_close_fnode(node);
    return 0;
}


// #define SYS_WINDOW  18

long sys_pipe(int *fds, int flags)
{
    inode_t *ino = vfs_pipe();
    if (ino == NULL)
        return -1;

    int oflags = flags & (O_DIRECT);

    // Parse oflags (flags)
    file_t *reader = file_from_inode(ino, VM_RD | oflags);
    fds[0] = resx_put(__current->fset, RESX_FILE, reader, (void *)file_close);

    file_t *writer = file_from_inode(ino, VM_WR | oflags);
    fds[1] = resx_put(__current->fset, RESX_FILE, writer, (void *)file_close);

    vfs_close_inode(ino);
    return 0;
}

int sys_mount(const char *device, const char *dir, const char *fstype, const char *options, int flags)
{
    if (device && mspace_check_str(__current->vm, device, 4096) != 0)
        return -1;
    if (dir && mspace_check_str(__current->vm, dir, 4096) != 0)
        return -1;
    if (fstype && mspace_check_str(__current->vm, fstype, 4096) != 0)
        return -1;
    if (options && mspace_check_str(__current->vm, options, 4096) != 0)
        return -1;

    int ret = vfs_mount(__current->fsa, device, fstype, dir, NULL, options ? options : "");
    return ret;
}

int sys_mkfs(const char *device, const char *fstype, const char *options, int flags)
{
    if (device && mspace_check_str(__current->vm, device, 4096) != 0)
        return -1;
    if (fstype && mspace_check_str(__current->vm, fstype, 4096) != 0)
        return -1;
    if (options && mspace_check_str(__current->vm, options, 4096) != 0)
        return -1;

    inode_t* blk = vfs_search_ino(__current->fsa, device, NULL, true);
    if (blk == NULL)
        return -1; // "Unable to find the device\n"

    if (blk->type != FL_BLK && blk->type != FL_REG) {
        // "We can only format block devices\n";
        vfs_close_inode(blk);
        return -1;
    }

    int ret = vfs_format(fstype, blk, options);
    vfs_close_inode(blk);
    return ret;
}

