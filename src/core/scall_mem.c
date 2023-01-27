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

void *sys_mmap(void *addr, size_t length, unsigned flags, int fd, size_t off)
{
    inode_t *ino = NULL;
    if (fd >= 0) {
        // Lock fset
        file_t *file = resx_get(__current->fset, RESX_FILE, fd);
        if (file == NULL) {
            errno = EBADF;
            return (void *) -1;
        }
        ino = vfs_open_inode(file->ino);
        // UnLock fset
    }

    unsigned vma = flags & VM_RWX;
    if (ino != NULL)
        vma |= VMA_FILE;
    else if (flags & MAP_HEAP)
        vma |= VMA_HEAP;
    else if (flags & MAP_STACK)
        vma |= VMA_STACK;
    else
        vma |= VMA_ANON;
    if (flags & MAP_POPULATE && !(flags & MAP_SHARED))
        vma |= VM_RESOLVE;

    // TODO - Transform flags !
    void *ptr = mspace_map(__current->vm, (size_t)addr, length, ino, off, vma);

    if (ino != NULL)
        vfs_close_inode(ino);
    return ptr != NULL ? ptr : (void *) -1;
}

long sys_munmap(void *addr, size_t length)
{
    return mspace_unmap(__current->vm, (size_t)addr, length);
}


long sys_mprotect(void *addr, size_t length, unsigned flags)
{
    unsigned vma = flags & VM_RWX;
    // TODO - Transform flags !
    return mspace_protect(__current->vm, (size_t)addr, length, vma);
}
