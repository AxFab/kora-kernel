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
#include <stdio.h>
#include <stddef.h>
#include <bits/atomic.h>
#include <bits/cdefs.h>
#include <kora/bbtree.h>
#include <kora/mcrs.h>
#include <kernel/stdc.h>
#include <kernel/vfs.h>
#include <Windows.h>


int kallocCount = 0;
int kmapCount = 0;

#if !defined(_WIN32)
# define _valloc(s)  valloc(s)
# define _vfree(p)  free(p)
#else
# define _valloc(s)  _aligned_malloc(s, PAGE_SIZE)
# define _vfree(p)  _aligned_free(p)
#endif


void *kalloc(size_t len)
{
    kallocCount++;
    void *ptr = calloc(len, 1);
    kprintf(KL_MAL, "+ alloc (%p, %d)\n", ptr, len);
    return ptr;
}

void kfree(void *ptr)
{
    kallocCount--;
    kprintf(KL_MAL, "- free (%p)\n", ptr);
    free(ptr);
}


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
    ++kmapCount;
    void *ptr = _valloc(len);
    if (ino == NULL) {
        if (access & VMA_PHYS && off != 0)
            memcpy(ptr, (void *)off, len);
        kprintf(KL_MAL, "+ kmap (%p, %d, -)\n", ptr, len);
        return ptr;
    }
    // printf("KMAP %x %X %X %o\n", len, ino, off, access);
    if (!map_init) {
        bbtree_init(&map_tree);
        map_init = true;
    }
    char *buf = ptr;
    char tmp[64];
    kprintf(KL_MAL, "+ kmap (%p, %d, %s+%llx)\n", ptr, len, vfs_inokey(ino, tmp), off);
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
        assert(pg != 0); // TODO - Better handling this
        memcpy(buf, (void *)pg, PAGE_SIZE);
        // TODO -- We're suppose to use those pages an release them at kunmap only !
        // ((inode_t *)ino)->fops->release(ino, off, pg);
        len -= PAGE_SIZE;
        buf += PAGE_SIZE;
        off += PAGE_SIZE;
    }

    mp->node.value_ = (size_t)ptr;
    bbtree_insert(&map_tree, &mp->node);
    return ptr;
}

void kunmap(void *addr, size_t len)
{
    if (!map_init) {
        bbtree_init(&map_tree);
        map_init = true;
    }
    --kmapCount;
    map_page_t *mp = (void *)bbtree_search_eq(&map_tree, addr, map_page_t, node);
    if (mp != NULL) {
        char tmp[64];
        kprintf(KL_MAL, "- kunmap (%p, %d, %s+%llx)\n", addr, len, vfs_inokey(mp->ino, tmp), mp->off);
        bbtree_remove(&map_tree, (size_t)addr);
        bool dirty = mp->access & 2 ? true : false;
        unsigned i;
        for (i = 0; i < mp->len; ++i) {
            page_t pg = mp->pgs[i];
            if (dirty)
                memcpy((void *)pg, mp->ptr, PAGE_SIZE);
            mp->ino->fops->release(mp->ino, mp->off, pg, dirty);
            mp->off += PAGE_SIZE;
            mp->ptr += PAGE_SIZE;
        }
        kfree(mp);
    } else
        kprintf(KL_MAL, "- kunmap (%p, %d, -)\n", addr, len);
    _vfree(addr);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


const char *ksymbol(void *ip, char *buf, int lg)
{
    return NULL;
}


char __buf[1024];
splock_t __bf_lock;
unsigned __log_filter = BIT(KL_MAL);

void kprintf(klog_t log, const char *msg, ...)
{
    if (__log_filter & (1 << log))
        return;
    va_list ap;
    va_start(ap, msg);
    splock_lock(&__bf_lock);
    int len = vsnprintf(__buf, 1024, msg, ap);
    va_end(ap);
    fwrite(__buf, len, 1, stdout);
    splock_unlock(&__bf_lock);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

xtime_t xtime_read(xtime_name_t name)
{
    FILETIME tm;
    GetSystemTimePreciseAsFileTime(&tm);
    uint64_t cl = (uint64_t)tm.dwHighDateTime << 32 | tm.dwLowDateTime;
    return cl / 10LL - SEC_TO_USEC(11644473600LL);
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

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void page_release(page_t page)
{
    _vfree((void *)page);
}

page_t mmu_read(size_t address)
{
    void *page = _valloc(PAGE_SIZE);
    memcpy(page, (void *)address, PAGE_SIZE);
    // kprintf(-1, "Page shadow copy at %p \n", page);
    return (page_t)page;
}


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


// mmu_enable
// mmu_leave
// mmu_create_uspace
// mmu_read
// mmu_drop
// mmu_resolve
// mmu_protect

#include <kernel/memory.h>

/* - */
void mmu_enable() {}
/* - */
void mmu_leave() {}
/* - */
void mmu_context(mspace_t *mspace) {}
/* - */
int mmu_resolve(size_t vaddr, page_t phys, int falgs)
{
    assert(0);
    return -1;
}
/* - */
// page_t mmu_read(size_t vaddr) {}
/* - */
page_t mmu_drop(size_t vaddr)
{
    assert(0);
    return 0;
}
/* - */
page_t mmu_protect(size_t vaddr, int falgs)
{
    assert(0);
    return 0;
}
/* - */
void mmu_create_uspace(mspace_t *mspace) {}
/* - */
void mmu_destroy_uspace(mspace_t *mspace) {}



#include <kernel/arch.h>

void cpu_setjmp(cpu_state_t *buf, void *stack, void *func, void *arg)
{
    printf("Cpu set jmp\n");
    assert(0);
}

int cpu_save(cpu_state_t *buf)
{
    printf("Cpu save \n");
    assert(0);
    return -1;
}

_Noreturn void cpu_restore(cpu_state_t *buf)
{
    assert(0);
    for (;;);
}

_Noreturn void cpu_halt()
{
    assert(0);
    for (;;);
}

int cpu_no()
{
    return 0;
}






// struct kSys kSYS;

//thread_local int __cpu_no = 0;
//atomic_int __cpu_inc = 1;

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


int __irq_semaphore = 0;

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


// void sleep_timer(long timeout)
// {
//     usleep(MAX(1, timeout));
// }


// page_t mmu_read(size_t address)
// {
//     return address;
// }
