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
#include <kernel/memory.h>
#include <kernel/task.h>
#include <kora/mcrs.h>
#include <kora/iofile.h>
#include <kora/splock.h>
// #include <stdlib.h>
#include <stdarg.h>
#include <string.h>

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void __perror_fail(int err, const char *file, int line, const char *msg)
{
    kprintf(-1, "ERROR] Process fails (%d) at %s:%d -- %s\n", err, file, line,
            msg);
}

/**
 * Routine that handle the case when an assertion failed. This method is never
 * called directly but is part of the `assert()' macro.
 *
 * @expr  The literal expression that failed
 * @file  The name of the source file which contains the assertion
 * @line  The line number where is the assertion on the source file
 */
_Noreturn void __assert_fail(const char *expr, const char *file, int line)
{
    kprintf(0, "Assertion failed (%s) at %s:%d -- %s\n", expr, file, line);
    kpanic("Assertion\n");
}

int *__errno_location()
{
    if (kCPU.running)
        return &kCPU.running->errno;
    return &kCPU.errno;
}

int isspace(char a)
{
    return a > 0 && a <= 0x20;
}

int isdigit(char a)
{
    return a >= '0' && a <= '9';
}

// int *__ctype_b_loc()
// {
//     return NULL;
// }

void *heap_map(size_t length)
{
    return kmap(length, NULL, 0, VMA_FG_HEAP);
}

void heap_unmap(void *address, size_t length)
{
    kunmap(address, length);
}

_Noreturn void abort()
{
    for (;;);
}

int snprintf(char *, int, const char *, ...);
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
FILE *klogs = NULL;
// splock_t klog_lock;

int vfprintf(FILE *fp, const char *str, va_list ap);

void kprintf(int log, const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    // splock_lock(&klog_lock);
    if (klogs != NULL) {
        vfprintf(klogs, msg, ap);
    }
    // splock_unlock(&klog_lock);
    va_end(ap);
}

void *kalloc(size_t size)
{
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

void *kmap(size_t length, inode_t *ino, off_t offset, int flags)
{
    flags &= ~(VMA_RIGHTS << 4);
    flags |= (flags & VMA_RIGHTS) << 4;
    void *ptr = mspace_map(kMMU.kspace, 0, length, ino, offset, 0, flags);
    if (flags & VMA_RESOLVE) {
        page_resolve(kMMU.kspace, (size_t)ptr, length);
    }
    return ptr;
}

void kunmap(void *address, size_t length)
{
    mspace_protect(kMMU.kspace, (size_t)address, length, VMA_DEAD);
}

_Noreturn void kpanic(const char *msg, ...)
{
    kprintf(0, "\033[31m;Kernel panic: %s \033[0m;\n", msg);
    abort();
}

void kclock(struct timespec *ts)
{
    ts->tv_sec = 0;
    ts->tv_nsec = 0;
}
