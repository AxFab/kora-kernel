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
#include <kora/mcrs.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>


#if _ADD_STDC_MISS
int strnlen(const char *str, int max)
{
    register const char *p = str;
    while (*p && max--) {
        p++;
    }
    return str - p;
}

void *valloc (size_t size)
{
    void *ptr = malloc(size + 4096);
    ptr = (void *)ALIGN_UP((int)ptr, 4096);
    return ptr;
}

#endif
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void __perror_fail(int err, const char *file, int line, const char *msg)
{
    printf("ERROR] Process fails (%d) at %s:%d -- %s\n", err, file, line, msg);
    abort();
}

void *heap_map(size_t length)
{
    void *ptr = valloc(ALIGN_UP(length, 4096));
    printf("TRACE] Heap map %p  - %x\n", ptr, length);
    return ptr;
}

void heap_unmap(void *address, size_t length)
{
    printf("TRACE] Heap unmap %p  - %x\n", address, length);
    free(address);
}

/* Store in a temporary buffer a size in bytes in a human-friendly format. */
char *sztoa(size_t number)
{
    static char sz_format[20];
    static const char *prefix[] = { "bs", "Kb", "Mb", "Gb", "Tb", "Pb", "Eb" };
    int k = 0;
    int rest = 0;

    while (number > 1024) {
        k++;
        rest = number & (1024 - 1);
        number /= 1024;
    };

    if (k == 0) {
        snprintf (sz_format, 20, "%d bytes", (int)number);

    } else if (number < 10) {
        float value = (rest / 1024.0f) * 100;
        snprintf (sz_format, 20, "%1d.%02d %s", (int)number, (int)value, prefix[k]);

    } else if (number < 100) {
        float value = (rest / 1024.0f) * 10;
        snprintf (sz_format, 20, "%2d.%01d %s", (int)number, (int)value, prefix[k]);

    } else {
        // float value = (rest / 1024.0f) + number;
        //snprintf (sz_format, 20, "%.3f %s", (float)value, prefix[k]);
        snprintf (sz_format, 20, "%4d %s", (int)number, prefix[k]);
    }

    return sz_format;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
#include <kernel/core.h>
#include <kernel/memory.h>

void kprintf(int log, const char *msg, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, msg);
    vsnprintf(buf, 256, msg, ap);
    va_end(ap);
    // LOCK
    printf("%s", buf);
    // UNLOCK
}

void *kalloc(size_t size)
{
    return calloc(1, size);
}

void kfree(void *ptr)
{
    free(ptr);
}

void *kmap(size_t length, inode_t *ino, off_t offset, int flags)
{
    flags &= ~(VMA_RIGHTS << 4);
    flags |= (flags & VMA_RIGHTS) << 4;
    uint8_t *ptr = (uint8_t *)mspace_map(kMMU.kspace, 0, length, ino, offset,
                                         0, flags);
    uint8_t *add = ptr;
    while (length) {
        page_fault(NULL, (size_t)ptr, 0);
        ptr += PAGE_SIZE;
        length -= PAGE_SIZE;
    }
    return add;
}

void kunmap(void *address, size_t length)
{
    mspace_protect(kMMU.kspace, (size_t)address, length, VMA_DEAD);
}

void kpanic(const char *msg, ...)
{
    kprintf(0, "\033[31m;Kernel panic: %s \033[0m;\n", msg);
    abort();
}

void kclock(struct timespec *ts)
{
#if _ADD_STDC_MISS
    ts->tv_sec = time(NULL);
    ts->tv_nsec = 0;
#else
    clock_gettime(CLOCK_REALTIME, ts);
#endif
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

#if 0
int main (int argc, char **argv)
{
    kernel_start();
    return 0;
}
#endif