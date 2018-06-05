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
#include <kernel/memory.h>
#include <kora/mcrs.h>
#include <time.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>

#undef PAGE_SIZE
#define PAGE_SIZE 4096
#undef CLOCKS_PER_SEC
#define CLOCKS_PER_SEC  1000000L

void *malloc(size_t size);
void *valloc(size_t size);
void free(void *ptr);

_Noreturn void abort();

int vprintf(const char*, va_list);

clock_t clock();

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void *kalloc(size_t size)
{
    void *ptr = malloc(size);
    memset(ptr, 0, size);
    return ptr;
}

void kfree(void *ptr)
{
    free(ptr);
}

// void heap_map()
// {
//     abort();
// }

// void heap_unmap()
// {
//     abort();
// }

void *kmap(size_t length, inode_t *ino, off_t offset, int flags)
{
    length = ALIGN_UP(length, PAGE_SIZE);
    char *ptr = (char *)valloc(length);

    static char *rights[] = { "---", "--x", "-w-", "-wx", "r--", "r-x", "rw-", "rwx"};
    char sh = flags & VMA_COPY_ON_WRITE ? (flags & VMA_SHARED ? 'W' : 'w') : (flags & VMA_SHARED ? 'S' : 'p');
    kprintf(KLOG_MEM, " - Krn :: "FPTR"-"FPTR" %s%c {%x}\n", ptr, ptr + length, rights[flags & 7], sh, flags);

    switch (flags & VMA_TYPE) {
    case VMA_FILE:
        vfs_read(ino, ptr, length, offset);
        break;
    case VMA_PIPE:
    case VMA_ANON:
    case VMA_STACK:
        memset(ptr, 0, length);
        break;
    case VMA_HEAP:
    case VMA_PHYS:
    default:
        kprintf(KLOG_ERR, "Error kmap type %x\n", flags & VMA_TYPE);
        abort();
    }

    return ptr;
}

void kunmap(void *address, size_t length)
{
    free(address);
}


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void kclock(struct timespec *ts)
{
    clock_t c = clock();
    ts->tv_sec = c / CLOCKS_PER_SEC;
    ts->tv_nsec = (c % CLOCKS_PER_SEC) * (1000000000 / CLOCKS_PER_SEC);
}

void __perror_fail(int err, const char *file, int line, const char *msg)
{
    kprintf(KLOG_ERR, "ERROR] Process fails (%d) at %s:%d -- %s\n", err, file, line,
            msg);
    abort();
}


_Noreturn void kpanic(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    vprintf(format, ap);
    va_end(ap);
    abort();
}

splock_t klog_lock;

void kprintf(int lvl, const char *format, ...)
{
    splock_lock(&klog_lock);
    va_list ap;
    va_start(ap, format);
    vprintf(format, ap);
    va_end(ap);
    splock_unlock(&klog_lock);
}

struct kCpu kCPU0;

struct kSys kSYS = {
    .cpus[0] = &kCPU0,
};
