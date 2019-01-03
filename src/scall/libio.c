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
#include <kernel/core.h>
#include <kernel/scall.h>
#include <kernel/files.h>
#include <kernel/device.h>
#include <kernel/task.h>
#include <errno.h>




int sys_open(int fd, const char *path, unsigned long flags, unsigned long mode)
{
    task_t *task = kCPU.running;
    inode_t *dir = task->pwd;
    if (fd >= 0)
        dir = resx_get(task->resx, fd);
    inode_t *ino = vfs_search(task->root, dir, path, NULL);
    if (ino == NULL)
        return -1;
    errno = 0;
    return resx_set(task->resx, ino);
}

int sys_close(int fd)
{
    task_t *task = kCPU.running;
    return resx_rm(task->resx, fd);
}

int sys_read(int fd, const struct iovec *vec, unsigned long vlen)
{
    unsigned i;
    task_t *task = kCPU.running;
    inode_t *ino = resx_get(task->resx, fd);
    if (ino == NULL)
        return -1;
    int bytes = 0, ret;
    for (i = 0; i < vlen; ++i) {
        switch (ino->mode & S_IFMT) {
        case S_IFREG:
        case S_IFBLK:
            ret = ioblk_read(ino, vec[i].buffer, vec[i].length, 0);
            break;
        case S_IFIFO:
        case S_IFCHR:
        // ret = pipe_read(ino, vec[i].buffer, vec[i].length, IO_ATOMIC);
        // break;
        default:
            errno = ENOSYS;
            return -1;
        }

        assert((ret >= 0) != (errno != 0));
        if (ret < 0)
            return -1;
        bytes += ret;
    }
    return bytes;
}

int sys_write(int fd, const struct iovec *vec, unsigned long vlen)
{
    unsigned i;
    task_t *task = kCPU.running;
    inode_t *ino = resx_get(task->resx, fd);
    if (ino == NULL)
        return -1;
    int bytes = 0, ret;
    for (i = 0; i < vlen; ++i) {
        switch (ino->mode & S_IFMT) {
        case S_IFREG:
        case S_IFBLK:
            ret = ioblk_write(ino, vec[i].buffer, vec[i].length, 0);
            break;
        case S_IFIFO:
        // ret = pipe_write(ino, vec[i].buffer, vec[i].length, IO_ATOMIC);
        // break;
        default:
            errno = ENOSYS;
            return -1;
        }

        assert((ret >= 0) != (errno != 0));
        if (ret < 0)
            return -1;
        bytes += ret;
    }
    return bytes;
}

int sys_seek(int fd, off_t off, unsigned long whence)
{
    // task_t *task = kCPU.running;
    return -1;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int sys_pipe(int *fd, size_t size)
{
    task_t *task = kCPU.running;
    inode_t *ino = vfs_inode(0, S_IFIFO | 0600, NULL, ALIGN_UP(size, PAGE_SIZE));
    fd[0] = resx_set(task->resx, ino);
    fd[1] = resx_set(task->resx, ino);
    errno = 0;
    return 0;
}

int sys_window(void *img, int fd, void *info, unsigned long features,
               unsigned long events)
{
    // int width = ALIGN_UP(img->width, 16);
    // int height = ALIGN_UP(img->height, 16);
    // size_t size = (size_t)width * height * 4;
    // inode_t * ino = vfs_inode(3, S_FWIN, 0);
    // ino->block = (size_t)width * 4;
    // ino->length = (size_t)width * height * 4;
    // // No operations !
    // task_setfile(kCPU.running, ino);
    // // ALLOC ALL PAGES !
    // // Create a new inode of type frame buffer !
    return 0;
}
