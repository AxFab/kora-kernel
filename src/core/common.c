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
#include <kernel/stdc.h>
#include <kernel/memory.h>
#include <kernel/tasks.h>

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

_Noreturn void __assert_fail(const char *expr, const char *file, unsigned line, const char *func)
{
    kprintf(KL_ERR, "Assertion failed CPU%d (%s) at %s:%d -- %s\n", cpu_no(), expr, file, line);
    for (;;);
}


int __errno = 0;

int *__errno_location()
{
    if (__current != NULL)
        return &__current->err_no;
    return &__errno;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void *kmap(size_t length, void *ino, xoff_t offset, int flags)
{
    char tmp[64];
    length = ALIGN_UP(length, PAGE_SIZE);
    int type = flags & VMA_TYPE;
    if (type == 0 && ino != NULL)
        flags |= VMA_FILE;
    void *ptr = mspace_map(kMMU.kspace, 0, length, (inode_t *)ino, offset, flags);
    if (ptr == NULL) {
        kprintf(KL_ERR, "Unable to map memory on the kernel\n");
        for (;;);
    }
    return ptr;
}

void kunmap(void *address, size_t length)
{
    mspace_unmap(kMMU.kspace, (size_t)address, length);
}


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
void *malloc(size_t size);
void free(void *ptr);


void *kalloc(size_t size)
{
    if (size > PAGE_SIZE) {
        kprintf(-1, "Page is size %x\n", size);
        stackdump(12);
    }
    assert(size <= PAGE_SIZE);
    void *ptr = malloc(size);
    memset(ptr, 0, size);
    return ptr;
}

void kfree(void *ptr)
{
    size_t size = 8; // TODO szofalloc(ptr);
    memset(ptr, 0, size);
    free(ptr);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

char __buf[1024];
splock_t __bf_lock;
unsigned __log_filter = 0;

void kprintf(klog_t log, const char *msg, ...)
{
    if (__log_filter & (1 << log))
        return;
    va_list ap;
    va_start(ap, msg);
    splock_lock(&__bf_lock);
    int len = vsnprintf(__buf, 1024, msg, ap);
    va_end(ap);
    kwrite(__buf, len);
    splock_unlock(&__bf_lock);
}

xtime_t xtime_read(xtime_name_t name)
{
    return clock_read(&__clock, name);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

unsigned __rseed = 0;

unsigned rand_r(unsigned *);

uint8_t rand8()
{
    return rand_r(&__rseed) & 0xff;
}

uint16_t rand16()
{
    return (rand_r(&__rseed) & 0xff) |
           ((rand_r(&__rseed) & 0xff) << 8);
}

uint32_t rand32()
{
    return (rand_r(&__rseed) & 0xff) |
           ((rand_r(&__rseed) & 0xff) << 8) |
           ((rand_r(&__rseed) & 0xff) << 16) |
           ((rand_r(&__rseed) & 0xff) << 24);
}
