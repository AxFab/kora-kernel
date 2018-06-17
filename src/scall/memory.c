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
#include <kernel/scall.h>
#include <kernel/memory.h>
#include <kernel/task.h>
#include <kernel/vfs.h>
#include <errno.h>

void *sys_mmap(size_t address, size_t length, int fd, off_t offset,
               unsigned long flags)
{
    task_t *task = kCPU.running;
    inode_t *ino = NULL;
    if (fd != -1) {
        // ino = task_getfile(task, fd);
        if (ino == NULL) {
            errno = EBADF;
            return (void *) - 1;
        }
    }
    int vmaflags = flags & VMA_RIGHTS;
    if (ino != NULL)
        vmaflags |= VMA_FILE;

    else
        vmaflags |= VMA_ANON;
    return mspace_map(task->usmem, address, length, ino, offset, 0, vmaflags);
}

int sys_mprotect(size_t address, size_t length, unsigned int flags)
{
    int vmaflags = flags & VMA_RIGHTS;
    return mspace_protect(kCPU.running->usmem, address, length, vmaflags);
}

int sys_munmap(size_t address, size_t length)
{
    return mspace_unmap(kCPU.running->usmem, address, length);
}
