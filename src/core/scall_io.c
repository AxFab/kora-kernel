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

long sys_open(const char *path, int flags, int mode)
{
    if (mspace_check_str(__current->vm, path, 4096) != 0)
        return -1;

    inode_t *ino = vfs_open(__current->fsa, path, __current->user, mode, flags);
    if (ino == NULL)
        return -1;

    if (ino == NULL)
        return -1;
    file_t *file = file_from_inode(ino, flags);
    int fd = resx_put(__current->fset, RESX_FILE, file, (void *)file_close);
    return fd;
}

long sys_close(int fd)
{
    file_t *file = resx_get(__current->fset, RESX_FILE, fd);
    if (file == NULL) {
        errno = EBADF;
        return -1;
    }
    file_close(file);
    return 0;
}

long sys_opendir(const char *path)
{
    if (mspace_check_str(__current->vm, path, 4096) != 0)
        return -1;

    diterator_t *ctx = vfs_opendir(__current->fsa, path, __current->user);
    if (ctx == NULL)
        return -1;
    int fd = resx_put(__current->fset, RESX_DIR, ctx, (void *)vfs_closedir);
    return fd;
}

long sys_readdir(int fd, char *buf, size_t len)
{
    if (mspace_check(__current->vm, buf, len, VM_WR) != 0)
        return -1;

    diterator_t *ctx = resx_get(__current->fset, RESX_DIR, fd);
    if (ctx == NULL)
        return -1;

    int count = 0;
    char name[256];
    while (len >= sizeof(struct dirent)) {
        inode_t *ino = vfs_readdir(ctx, name, 256);
        if (ino == NULL)
            break;

        struct dirent *entry = (void *)buf;
        strncpy(entry->d_name, name, 256);
        entry->d_ino = ino->no;
        entry->d_off = 0;
        entry->d_reclen = sizeof(struct dirent);

        buf += sizeof(struct dirent);
        len -= sizeof(struct dirent);
        vfs_close_inode(ino);
        ++count;
    }
    return count;
}

long sys_seek(int fd, xoff_t *poffset, int whence)
{
    if (mspace_check(__current->vm, poffset, sizeof(xoff_t), VM_RW) != 0)
        return -1;

    file_t *file = resx_get(__current->fset, RESX_FILE, fd);
    if (file == NULL)
        return -1;

    if (whence == 0) // SEEK_BEGIN
        file->off = *poffset;
    else if (whence == 1) // SEEK_SET
        file->off = file->off + *poffset;
    else if (whence == 2) // SEEK_END
        file->off = file->ino->length - *poffset;
    else {
        errno = EINVAL;
        return -1;
    }

    file->off = MIN(file->ino->length, MAX(0, file->off));
    *poffset = file->off;
    return 0;
}

long sys_read(int fd, char *buf, int len)
{
    if (mspace_check(__current->vm, buf, len, VM_WR) != 0)
        return -1;

    file_t *file = resx_get(__current->fset, RESX_FILE, fd);
    if (file == NULL)
        return -1;
    if ((file->oflags & VM_RD) == 0) {
        errno = EPERM;
        return -1;
    }
    int ret = vfs_read(file->ino, buf, len, file->off, 0);
    if (ret >= 0)
        file->off += ret;
    return ret;
}


long sys_write(int fd, const char *buf, int len)
{
    if (mspace_check(__current->vm, buf, len, VM_RD) != 0)
        return -1;

    if (fd == 1 || fd == 2) {
        kprintf(-1, "Task%d] %s\n", __current->pid, buf);
    }

    file_t *file = resx_get(__current->fset, RESX_FILE, fd);
    if (file == NULL)
        return -1;
    if ((file->oflags & VM_WR) == 0) {
        errno = EPERM;
        return -1;
    }
    int ret = vfs_write(file->ino, buf, len, file->off, 0);
    if (ret > 0)
        file->off += ret;
    return ret;
}

long sys_access(const char *path, int flags)
{
    if (mspace_check_str(__current->vm, path, 4096) != 0)
        return -1;

    inode_t *ino = vfs_search_ino(__current->fsa, path, __current->user, true);
    if (ino == NULL)
        return -1;

    int ret = vfs_access(ino, __current->user, flags & VM_RWX);
    vfs_close_inode(ino);
    return ret;
}

// long sys_fcntl(int fd, int cmd, void **args)
// {
//     file_t *file = resx_get(__current->fset, RESX_FILE, fd);
//     if (file == NULL)
//         return -1;

//     int ret = vfs_fcntl(file->ino, cmd, args);
//     return ret;
// }

long sys_ioctl(int fd, int cmd, void **args)
{
    file_t *file = resx_get(__current->fset, RESX_FILE, fd);
    if (file == NULL)
        return -1;

    int ret = vfs_ioctl(file->ino, cmd, args);
    return ret;
}
