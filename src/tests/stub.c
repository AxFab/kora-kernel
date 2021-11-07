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
    kprintf(-1, "\033[96m+ alloc (%p, %d) %s\033[0m\n", ptr, len, msg);
    return ptr;
}

void kfree(void *ptr)
{
    kallocCount--;
    splock_lock(&ktrack_lock);
    bbtree_remove(&ktrack_tree, (size_t)ptr);
    splock_unlock(&ktrack_lock);
    kprintf(-1, "\033[96m- free (%p)\033[0m\n", ptr);
    free(ptr);
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
};
bbtree_t map_tree;
bool map_init = false;

void *kmap(size_t len, void *ino, xoff_t off, int access)
{
    // assert(irq_ready());
    ++kmapCount;
    if (!map_init) {
        bbtree_init(&map_tree);
        map_init = true;
    }
    void *ptr = _valloc(len);
    if (ino == NULL) {
        if (access & VM_PHYSIQ && off != 0)
            memcpy(ptr, (void *)off, len);
        // kprintf(-1, "+ kmap (%p, %d, -)\n", ptr, len);
        return ptr;
    }
    // printf("KMAP %x %X %X %o\n", len, ino, off, access);
    char *buf = ptr;
    char tmp[64];
    // kprintf(-1, "+ kmap (%p, %d, %s+%llx)\n", ptr, len, vfs_inokey(ino, tmp), off);
    map_page_t *mp = kalloc(sizeof(map_page_t));
    mp->ptr = ptr;
    mp->len = len / PAGE_SIZE;
    mp->ino = ino;
    mp->access = access;
    mp->off = off;
    int i = 0;
    while (len > 0) {
        page_t pg = ((inode_t *)ino)->fops->fetch(ino, off);
        mp->pgs[i++] = pg;
        assert((void *)pg != NULL); // TODO - Better handling this
        memcpy(buf, (void *)pg, PAGE_SIZE);
        // TODO -- We're suppose to use those pages an release them at kunmap only !
        // ((inode_t *)ino)->fops->release(ino, off, pg);
        len -= PAGE_SIZE;
        buf += PAGE_SIZE;
        off += PAGE_SIZE;
    }

    mp->node.value_ = (size_t)ptr;
    bbtree_insert(&map_tree, &mp->node);
    // assert(irq_ready());
    return ptr;
}

void kunmap(void *addr, size_t len)
{
    assert(irq_ready());
    --kmapCount;
    map_page_t *mp = bbtree_search_eq(&map_tree, (size_t)addr, map_page_t, node);
    if (mp != NULL) {
        char tmp[64];
        // kprintf(-1, "- kunmap (%p, %d, %s+%llx)\n", addr, len, vfs_inokey(mp->ino, tmp), mp->off);
        bbtree_remove(&map_tree, (size_t)addr);
        bool dirty = mp->access & 2 ? true : false;
        size_t i;
        for (i = 0; i < mp->len; ++i) {
            page_t pg = mp->pgs[i];
            if (dirty)
                memcpy((void *)pg, mp->ptr, PAGE_SIZE);
            mp->ino->fops->release(mp->ino, mp->off, pg, dirty);
            mp->off += PAGE_SIZE;
            mp->ptr += PAGE_SIZE;
        }
        kfree(mp);
    } // else
        // kprintf(-1, "- kunmap (%p, %d, -)\n", addr, len);
    _vfree(addr);
    assert(irq_ready());
}

void page_release(page_t page)
{
    _vfree((void *)page);
}

page_t page_read(size_t address)
{
    void *page = _valloc(PAGE_SIZE);
    memcpy(page, (void *)address, PAGE_SIZE);
    // kprintf(-1, "Page shadow copy at %p \n", page);
    return (page_t)page;
}

page_t mmu_read(size_t address)
{
    return page_read(address);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void __assert_fail(const char *expr, const char *file, int line)
{
    fprintf(stderr, "Assertion - %s at %s:%d\n", expr, file, line);
    abort();
}

const char *ksymbol(void *ip, char *buf, int lg)
{
    return NULL;
}


splock_t kplock = INIT_SPLOCK;
char kpbuf[1024];

void kprintf(int log, const char *msg, ...)
{
    splock_lock(&kplock);
    va_list ap;
    va_start(ap, msg);
    int lg = vsnprintf(kpbuf, 1024, msg, ap);
    va_end(ap);
    write(1, kpbuf, lg);
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
