/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2018  <Fabien Bavent>
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
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <time.h>
#include <kernel/core.h>
#include <kernel/vfs.h>
#include <kora/hmap.h>
#include <kora/splock.h>
#include "../check.h"

int __errno;
splock_t klog_lock = INIT_SPLOCK;

#if !defined(_WIN32)
#define _valloc(s) valloc(s)
#define _vfree(p) free(p)
#else
#define _valloc(s) _aligned_malloc(s, PAGE_SIZE)
#define _vfree(p) _aligned_free(p)
#endif


int *__errno_location()
{
    return &__errno;
}

_Noreturn void __assert_fail(CSTR expr, CSTR file, int line)
{
    kprintf(KLOG_ERR, "Assertion failed :%s - %s:%d\n", expr, file, line);
    longjmp(__tcase_jump, 1);
}

void __perror_fail(int err, const char *file, int line, const char *msg)
{
    kprintf(KLOG_ERR, "ERROR] Process fails (%d) at %s:%d -- %s\n", err, file, line,
            msg);
    longjmp(__tcase_jump, 1);
}


void *kalloc(size_t size)
{
    return calloc(size, 1);
}

void kfree(void *ptr)
{
    free(ptr);
}

/* ------------------------------------------------------------------------ */

void kprintf(int lvl, CSTR msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    splock_lock(&klog_lock);
    vprintf(msg, ap);
    splock_unlock(&klog_lock);
    va_end(ap);
}

_Noreturn void kpanic(CSTR msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    splock_lock(&klog_lock);
    vprintf(msg, ap);
    splock_unlock(&klog_lock);
    va_end(ap);
    __assert_fail("Kernel panic!", __FILE__, __LINE__);
}

void kclock(struct timespec *ts)
{
    time64_t ticks = time64();
    ts->tv_sec = ticks / _PwNano_;
    ts->tv_nsec = ticks % _PwNano_;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

struct kCpu kCPU0;
struct kSys kSYS = {
    .cpus = &kCPU0,
};

__thread int __cpu_no = 0;

int cpu_no()
{
    return __cpu_no;
}

void irq_ack(int no)
{
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

bool krn_mmap_init = false;
HMP_map krn_mmap;

struct vma {
    void *address;
    size_t length;
    off_t offset;
    inode_t *ino;
    int flags;
};

void *kmap(size_t length, inode_t *ino, off_t offset, int flags)
{
    if (!IS_ALIGNED(length, PAGE_SIZE) || !IS_ALIGNED(length, PAGE_SIZE))
        assert("unautorized operation");
    if (!krn_mmap_init) {
        hmp_init(&krn_mmap, 16);
        krn_mmap_init = true;
    }
    void *ptr = _valloc(length);
    struct vma *vma = (struct vma *)kalloc(sizeof(struct vma));
    vma->length = length;
    vma->offset = offset;
    vma->ino = ino;
    vma->flags = flags;
    hmp_put(&krn_mmap, (char *)&ptr, sizeof(void *), vma);
    switch (flags & VMA_TYPE) {
    case VMA_FILE:
        assert(ino != NULL);
        vfs_read(ino, ptr, length, offset);
        break;
    case VMA_STACK:
        break;
    default:
        assert(false);
        break;
    }
    return ptr;
}

void kunmap(void *addr, size_t length)
{
    struct vma *vma = (struct vma *)hmp_get(&krn_mmap, (char *)&addr, sizeof(void *));
    assert(vma != NULL);
    assert(vma->length == length);
    hmp_remove(&krn_mmap, (char *)&addr, sizeof(void *));
    switch (vma->flags & VMA_TYPE) {
    case VMA_FILE:
        assert(vma->ino != NULL);
        if (vma->flags & VMA_WRITE)
            vfs_write(vma->ino, (void *)addr, length, vma->offset);
        break;
    default:
        assert(false);
        break;
    }
    free(vma);
    _vfree((void *)addr);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void *heap_map(size_t length)
{
    return _valloc(length);
}

void heap_unmap(void *adddress, size_t length)
{
    _vfree((void *)adddress);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

#ifdef _FAKE_TIME
#if !defined(_WIN32)
time64_t time64()
{
    clock_t ticks = clock();
    if (_PwNano_ > CLOCKS_PER_SEC)
        ticks *= _PwNano_ / CLOCKS_PER_SEC;
    else
        ticks /= CLOCKS_PER_SEC / _PwNano_;
    return ticks;
}
#else
#include <windows.h>
time64_t time64()
{
    // January 1, 1970 (start of Unix epoch) in ticks
    const INT64 UNIX_START = 0x019DB1DED53E8000;

    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);

    LARGE_INTEGER li;
    li.LowPart = ft.dwLowDateTime;
    li.HighPart = ft.dwHighDateTime;
    // Convert ticks since EPOCH into nano-seconds
    return (li.QuadPart - UNIX_START) * 100;
}
#endif
#endif


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


#ifdef _FAKE_TASK

_Noreturn void scheduler_switch(int status, int retcode)
{
    assert(false);
    for (;;);
}

int task_resume(task_t *task)
{
    return -1;
}

void task_kill(task_t *task, int signum)
{
}

#endif

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

