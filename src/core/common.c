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
    kprintf(KLOG_ERR, "ERROR] Process fails (%d) at %s:%d -- %s\n", err, file, line, msg);
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
    kprintf(KLOG_ERR, "Assertion failed CPU%d (%s) at %s:%d -- %s\n",        cpu_no(), expr, file, line);
    task_t *task = kCPU.running;
    if (task)
        task_core(task);
    kpanic("Assertion\n");
}

int *__errno_location()
{
    if (kCPU.running)
        return &kCPU.running->err_no;
    return &kCPU.err_no;
}

int isspace(char a)
{
    return a > 0 && a <= 0x20;
}

int isdigit(char a)
{
    return a >= '0' && a <= '9';
}

void *heap_map(size_t length)
{
    return kmap(length, NULL, 0, VMA_HEAP_RW);
}

void heap_unmap(void *address, size_t length)
{
    kunmap(address, length);
}

_Noreturn void abort()
{
    for (;;);
}


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

splock_t klog_lock;

int no_dbg = 1;
void kwrite(const char *buf, int len);
int vfprintf(FILE *fp, const char *str, va_list ap);

char buf[1024];
splock_t bf_lock;
void kprintf(int log, const char *msg, ...)
{
    if (/*(log == KLOG_DBG && no_dbg) || */log == KLOG_MEM || log == KLOG_INO)
        return;
    va_list ap;
    va_start(ap, msg);
    splock_lock(&bf_lock);
    // char *buf = kalloc(256);
    int len = vsnprintf(buf, 1024, msg, ap);
    va_end(ap);
    splock_lock(&klog_lock);
    kwrite(buf, len);
    splock_unlock(&klog_lock);
    // kfree(buf);
    splock_unlock(&bf_lock);
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
    length = ALIGN_UP(length, PAGE_SIZE);
    flags &= ~(VMA_RIGHTS << 4);
    flags |= (flags & VMA_RIGHTS) << 4;
    void *ptr = mspace_map(kMMU.kspace, 0, length, ino, offset, flags);
    return ptr;
}

void kunmap(void *address, size_t length)
{
    mspace_unmap(kMMU.kspace, (size_t)address, length);
}

_Noreturn void kpanic(const char *msg, ...)
{
    kprintf(KLOG_ERR, "\033[31mKernel panic: %s \033[0m\n", msg);
    stackdump(4);
    abort();
}


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int rand_r(unsigned int *seed);

uint64_t rand64()
{
    uint64_t r = (uint64_t)rand_r(&kCPU.seed);
    r |= (uint64_t)rand_r(&kCPU.seed) << 15;
    r |= (uint64_t)rand_r(&kCPU.seed) << 30;
    r |= (uint64_t)rand_r(&kCPU.seed) << 45;
    r |= (uint64_t)rand_r(&kCPU.seed) << 60;
    return r;
}

uint32_t rand32()
{
    uint32_t r = (uint32_t)rand_r(&kCPU.seed);
    r |= (uint32_t)rand_r(&kCPU.seed) << 15;
    r |= (uint32_t)rand_r(&kCPU.seed) << 30;
    return r;
}

uint16_t rand16()
{
    uint32_t r = (uint16_t)rand_r(&kCPU.seed);
    r |= (uint16_t)rand_r(&kCPU.seed) << 15;
    return r & 0xFFFF;
}

uint8_t rand8()
{
    uint32_t r = (uint16_t)rand_r(&kCPU.seed);
    return r & 0xFF;
}
