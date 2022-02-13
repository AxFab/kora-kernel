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
#include <stdlib.h>
#include <stdio.h>
#include <bits/atomic.h>
#include <bits/cdefs.h>
#include <kernel/core.h>
#include <kernel/arch.h>
#include <kernel/vfs.h>
#include <kernel/stdc.h>
#if defined(_WIN32)
#  include <Windows.h>
#endif


thread_local int __irq_semaphore = 0;

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

#if !defined(_WIN32)
# define _valloc(s)  valloc(s)
# define _vfree(p)  free(p)
#else
# define _valloc(s)  _aligned_malloc(s, PAGE_SIZE)
# define _vfree(p)  _aligned_free(p)
#endif

int kallocCount = 0;
int kmapCount = 0;
bool ktrack_init = false;
bbtree_t ktrack_tree;
splock_t ktrack_lock;
struct ktrack
{
    void *ptr;
    size_t len;
    const char *msg;
    bbnode_t bnode;
};

int kalloc_count()
{
    return kallocCount;
}

int kmap_count()
{
    return kmapCount;
}

void *kalloc_(size_t len, const char *msg)
{
    if (!ktrack_init) {
        bbtree_init(&ktrack_tree);
        splock_init(&ktrack_lock);
        ktrack_init = true;
    }
    kallocCount++;
    void *ptr = calloc(len, 1);
    splock_lock(&ktrack_lock);
    struct ktrack *tr = malloc(sizeof(struct ktrack));
    tr->ptr = ptr;
    tr->len = len;
    tr->msg = msg;
    tr->bnode.value_ = (size_t)ptr;
    bbtree_insert(&ktrack_tree, &tr->bnode);
    splock_unlock(&ktrack_lock);
    // kprintf(-1, "\033[96m+ alloc (%p, %d) %s\033[0m\n", ptr, len, msg);
    return ptr;
}

void kfree(void *ptr)
{
    kallocCount--;
    splock_lock(&ktrack_lock);
    struct ktrack *tr = bbtree_search_eq(&ktrack_tree, (size_t)ptr, struct ktrack, bnode);
    assert(tr != NULL);
    bbtree_remove(&ktrack_tree, (size_t)ptr);
    splock_unlock(&ktrack_lock);
    // kprintf(-1, "\033[96m- free (%p)\033[0m\n", ptr);
    free(ptr);
    free(tr);
}

void kalloc_check()
{
    splock_lock(&ktrack_lock);
    struct ktrack *tr = bbtree_first(&ktrack_tree, struct ktrack, bnode);
    while (tr) {
        kprintf(-1, "- \033[31mMemory leak\033[0m at %p(%d) %s\n", tr->ptr, tr->len, tr->msg);
        tr = bbtree_next(&tr->bnode, struct ktrack, bnode);
    }

    splock_unlock(&ktrack_lock);
}

char *kstrdup(const char *str)
{
    int len = strlen(str) + 1;
    char *ptr = kalloc_(len, "strdup");
    strncpy(ptr, str, len);
    return ptr;
}

char *kstrndup(const char *str, size_t max)
{
    int len = strnlen(str, max) + 1;
    char *ptr = kalloc_(len, "strndup");
    strncpy(ptr, str, len);
    ptr[len] = '\0';
    return ptr;
}

void alloc_check()
{
    struct ktrack *tr = bbtree_first(&ktrack_tree, struct ktrack, bnode);
    while (tr) {
        printf("alloc leak: %p - %s\n", tr->ptr, tr->msg);
        tr = bbtree_next(&tr->bnode, struct ktrack, bnode);
    }
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

typedef struct map_page map_page_t;
struct map_page {
    page_t pgs[16];
    char *ptr;
    size_t len;
    inode_t *ino;
    bbnode_t node;
    int access;
    xoff_t off;
    int usage;
};
bbtree_t map_tree;
bool map_init = false;

map_page_t *kmap_new(int access, void *ino, xoff_t off, size_t len, void *ptr)
{
    map_page_t *mp = malloc(sizeof(map_page_t));
    mp->usage = 1;
    mp->access = access;
    mp->ino = ino;
    mp->off = off;
    mp->len = len;
    mp->ptr = ptr;
}

map_page_t *kmap_search(void *ino, xoff_t off, size_t len, int mode)
{
    map_page_t *mp = bbtree_left(map_tree.root_, map_page_t, node);
    while (mp) {
        if (mp->ino == ino && mp->off == off && mp->len <= len) {
            mp->usage++;
            mp->access |= mode;
            return mp;
        }
        mp = bbtree_next(&mp->node, map_page_t, node);
    }

    return NULL;
}

void kmap_check()
{
    map_page_t *mp = bbtree_first(&map_tree, map_page_t, node);
    while (mp) {
        printf("kmap leak: %p - %o - %p\n", mp->ptr, mp->access, mp->ino);
        mp = bbtree_next(&mp->node, map_page_t, node);
    }
}


void *kmap(size_t len, void *ino, xoff_t off, int flags)
{
    assert(len > 0);
    assert((len & (PAGE_SIZE - 1)) == 0);
    assert((off & (PAGE_SIZE - 1)) == 0);

    int type = flags & VMA_TYPE;
    int getter = 0;
    int access = flags & (VM_RW | VM_EX | VM_RESOLVE | VM_PHYSIQ);
    if (type == 0 && ino != NULL)
        type = VMA_FILE;
    if (type == 0 && access & VM_PHYSIQ)
        type = VMA_PHYS;
    assert((ino == NULL) != (type == VMA_FILE));
    assert(((access & VM_PHYSIQ) == 0) != (type == VMA_PHYS));
    switch (type) {
    case 0:
        type = VMA_ANON;
    case VMA_ANON:
        break;
    case VMA_PIPE:
        access = VM_RW;
        // data is pipe
        break;
    case VMA_STACK:
    case VMA_HEAP:
        access = VM_RW;
        break;
    case VMA_PHYS:
        getter = 4;
        access &= VM_RW;
        access |= VM_RESOLVE;
        break;
    case VMA_FILE:
        getter = 1;
        break;
    case VMA_CODE:
        access = VM_RD | VM_EX;
        getter = 2;
        break;
    case VMA_DATA:
        access = VM_WR;
        getter = 3;
        break;
    case VMA_RODATA:
        access = VM_RD;
        getter = 2;
        break;
    }

    ++kmapCount;
    if (!map_init) {
        bbtree_init(&map_tree);
        map_init = true;
    }


    map_page_t *mp = NULL;
    if (getter == 0) {
        mp = kmap_new(access | type, NULL, 0, len, _valloc(len));
    } else if (getter == 1) {
        assert(irq_ready());
        mp = kmap_search(ino, off, len, access & 7);
        if (mp != NULL)
            return mp->ptr;
        
        mp = kmap_new(access | type, ino, off, len, NULL);
        assert(len == PAGE_SIZE);
        mp->ptr = ((inode_t *)ino)->fops->fetch(ino, off);

    } else if (getter == 4) {
        if (off == 0) {
            mp = kmap_new(access | type, NULL, 0, len, _valloc(len));
        } else {
            mp = kmap_search(ino, off, len, access & 7);
            if (mp != NULL)
                return mp->ptr;
            mp = kmap_new(access | type, off, 0, len, (void*)off);
        }
    } else { // if (getter == 3) {
        assert("No dlib supported" == NULL);
    }

    mp->node.value_ = (size_t)mp->ptr;
    bbtree_insert(&map_tree, &mp->node);
    return mp->ptr;
}

void kunmap(void *addr, size_t len)
{
    char tmp[64];

    assert(irq_ready());
    --kmapCount;
    map_page_t *mp = bbtree_search_eq(&map_tree, (size_t)addr, map_page_t, node);
    assert(mp != NULL);

    if (--mp->usage > 0)
        return;

    int getter = 0;
    switch (mp->access & VMA_TYPE) {
    case VMA_FILE:
        getter = 1;
        break;
    case VMA_CODE:
    case VMA_RODATA:
        getter = 2;
        break;
    case VMA_DATA:
        getter = 3;
        break;
    case VMA_PHYS:
        getter = 4;
        break;
    }

    bbtree_remove(&map_tree, (size_t)addr);
    if (getter == 0) {
        _vfree(addr);
    } else if (getter == 1) {

        bool dirty = mp->access & VM_WR ? true : false;
        xoff_t nx = 0;
        mp->ino->fops->release(mp->ino, mp->off, mp->ptr + nx, dirty);

    } else if (getter == 4) {

    } else { // if (getter == 3) {
        assert("No dlib supported" == NULL);
    }

    free(mp);
    assert(irq_ready());
}

void page_release(page_t page)
{
    _vfree((void *)page);
}

page_t page_read(size_t address)
{
    // void *page = _valloc(PAGE_SIZE);
    // memcpy(page, (void *)address, PAGE_SIZE);
    // kprintf(-1, "Page shadow copy at %p \n", page);
    return (page_t)address;
}

page_t mmu_read(size_t address)
{
    return page_read(address);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

//void __assert_fail(const char *expr, const char *file, int line)
//{
//    fprintf(stderr, "Assertion - %s at %s:%d\n", expr, file, line);
//    abort();
//}

const char *ksymbol(void *ip, char *buf, int lg)
{
    return NULL;
}


splock_t kplock = INIT_SPLOCK;
char kpbuf[1024];
char kpbuf2[1024];

void kprintf(klog_t log, const char *msg, ...)
{
    splock_lock(&kplock);
    va_list ap;
    va_start(ap, msg);
    int lg = vsnprintf(kpbuf, 1024, msg, ap);
    va_end(ap);
    if (log == KL_BIO) {
        lg = snprintf(kpbuf2, 1024, "\033[35m%s\033[0m", kpbuf);
        write(1, kpbuf2, lg);
    } else {
        write(1, kpbuf, lg);
    }
    splock_unlock(&kplock);
}

/* Return the current time in the 1-microsecond precision */
xtime_t xtime_read(xtime_name_t name)
{
#if defined(_WIN32)
    FILETIME tm;
    GetSystemTimePreciseAsFileTime(&tm); // 100-nanosecond
    uint64_t cl = (uint64_t)tm.dwHighDateTime << 32 | tm.dwLowDateTime;
    return cl / 10LL - SEC_TO_USEC(11644473600LL);
#else

#endif
}

uint8_t rand8()
{
    return rand() & 0xff;
}

uint16_t rand16()
{
    return rand();
}

uint32_t rand32()
{
    return rand() | (rand() << 16);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void irq_reset(bool enable)
{
    __irq_semaphore = 0;
}

bool irq_enable()
{
    assert(__irq_semaphore > 0);
    --__irq_semaphore;
    return __irq_semaphore == 0;
}

void irq_disable()
{
    ++__irq_semaphore;
}

bool irq_ready()
{
    return __irq_semaphore == 0;
}



/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

#include <threads.h>

struct task_data_start {
    char* name[64];
    void (*func)(void*);
    void* arg;
};

static _task_impl_start(void* arg)
{
    struct task_data_start* data = arg;
#ifdef _WIN32
    wchar_t wbuf[128];
    size_t len;
    mbstowcs_s(&len, wbuf, 128, data->name, 128);
    SetThreadDescription(GetCurrentThread(), wbuf);
#endif
    void(*func)(void*) = data->func;
    void* param = data->arg;
    free(data);
    func(param);
}

int task_start(const char* name, void *func, void* arg)
{
    thrd_t thrd;
    struct task_data_start* data = malloc(sizeof(struct task_data_start));
    strncpy(data->name, name, 64);
    data->func = func;
    data->arg = arg;
    thrd_create(&thrd, _task_impl_start, data);
    return 0;
}


// thread_local int __cpu_no = 0;
// atomic_int __cpu_inc = 1;

//void cpu_init()
//{
//    // kSYS.cpus = kalloc(sizeof(struct kCpu) * 12);
//}

//void cpu_setup()
//{
//    __cpu_no = atomic_xadd(&__cpu_inc, 1);
//    kCPU.running = (task_t *)calloc(1, sizeof(task_t));
//    kCPU.running->pid = __cpu_no;
//}
//
//void cpu_sweep()
//{
//    free(kCPU.running);
//}
//
//int cpu_no()
//{
//    return __cpu_no;
//}
//
//int *__errno_location()
//{
//    return &errno;
//}

// void sleep_timer(long timeout)
// {
//     usleep(MAX(1, timeout));
// }
// void futex_init() {}
