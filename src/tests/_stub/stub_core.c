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
#include <errno.h>
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
    kprintf(KLOG_ERR, "ERROR] Process fails (%d) at %s:%d -- %s\n", err, file, line, msg);
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
        off_t off = offset;
        void *buf = ptr;
        while (length > 0) {
            page_t pg = ino->ops->fetch(ino, off);
            memcpy(buf, (void *)pg, PAGE_SIZE);
            ino->ops->release(ino, off, pg);
            length -= PAGE_SIZE;
            off += PAGE_SIZE;
            buf = ADDR_OFF(buf, PAGE_SIZE);
        }
        break;
    case VMA_STACK:
    case VMA_PIPE:
        break;
    case VMA_PHYS:
        assert(offset == 0);
        break;
    default :
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
        if (vma->flags & VMA_WRITE) {
            off_t off = vma->offset;
            void *padd = addr;
            inode_t *ino = vma->ino;
            while (vma->length > 0) {
                page_t pg = ino->ops->fetch(ino, off);
                memcpy((void *)pg, padd, PAGE_SIZE);
                ino->ops->sync(ino, off, pg);
                ino->ops->release(ino, off, pg);
                vma->length -= PAGE_SIZE;
                off += PAGE_SIZE;
                padd = ADDR_OFF(padd, PAGE_SIZE);
            }
        }
        break;
    case VMA_STACK:
    case VMA_PIPE:
    case VMA_PHYS:
        break;
    default:
        assert(false);
        break;
    }
    free(vma);
    _vfree((void *)addr);
}

void ck_reset()
{
    hmp_destroy(&krn_mmap, 0);
    krn_mmap_init = false;
}

_Noreturn void task_fatal(CSTR error, unsigned signum)
{
    assert(false);
    for (;;);
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
#if defined(_WIN32)
#include <Windows.h>

void nanosleep(struct timespec *tm, struct timespec *rs)
{
    clock64_t start = kclock();
    Sleep(tm->tv_sec * 1000 + tm->tv_nsec / 1000000);
    clock64_t elasped = start - kclock();
    elasped = tm->tv_sec * _PwNano_ + tm->tv_nsec - elasped;
    if (elasped < 0) {
        rs->tv_sec = 0;
        rs->tv_nsec = 0;
    } else {
        rs->tv_sec = elasped / _PwNano_;
        rs->tv_nsec = elasped % _PwNano_;
    }
}
#endif

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


const char *ksymbol(void *eip, char *buf, int lg)
{
    return "???";
}

