#include <bits/cdefs.h>
#include <bits/types.h>
#include <stdatomic.h>
#include <kernel/types.h>
#include <kernel/core.h>
#include <kernel/task.h>
#include <kernel/arch.h>
#include <kernel/vfs2.h>
#include "check.h"

// struct kSys kSYS;

thread_local int __cpu_no = 0;
atomic_int __cpu_inc = 1;
//
//void cpu_init()
//{
//    // kSYS.cpus = kalloc(sizeof(struct kCpu) * 12);
//}

//void cpu_setup()
//{
//    __cpu_no = atomic_fetch_add(&__cpu_inc, 1);
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

const char *ksymbol(void *ip, char *buf, int lg)
{
    return NULL;
}


void __assert_fail(const char *expr, const char *file, int line)
{
    printf("Assertion - %s at %s:%d\n", expr, file, line);
    abort();
}


bool irq_enable();
bool irq_active = false;
//
//void irq_reset(bool enable)
//{
//    irq_active = true;
//    kCPU.irq_semaphore = 0;
//    if (enable)
//        IRQ_ON;
//    else
//        IRQ_OFF;
//}
//
//bool irq_enable()
//{
//    if (irq_active) {
//        assert(kCPU.irq_semaphore > 0);
//        if (--kCPU.irq_semaphore == 0) {
//            IRQ_ON;
//            return true;
//        }
//    }
//    return false;
//}
//
//void irq_disable()
//{
//    if (irq_active) {
//        IRQ_OFF;
//        ++kCPU.irq_semaphore;
//    }
//}

int kallocCount = 0;
int kmapCount = 0;

void *kalloc(size_t len)
{
    kallocCount++;
    void *ptr = calloc(len, 1);
    kprintf(-1, "+ alloc (%p, %d)\n", ptr, len);
    return ptr;
}

void kfree(void *ptr)
{
    kallocCount--;
    kprintf(-1, "- free (%p)\n", ptr);
    free(ptr);
}

#include <kernel/vfs.h>

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
        if (access & VM_PHYSIQ && off != 0)
            memcpy(ptr, (void *)off, len);
        kprintf(-1, "+ kmap (%p, %d, -)\n", ptr, len);
        return ptr;
    }
    // printf("KMAP %x %X %X %o\n", len, ino, off, access);
    if (!map_init) {
        bbtree_init(&map_tree);
        map_init = true;
    }
    char *buf = ptr;
    char tmp[64];
    kprintf(-1, "+ kmap (%p, %d, %s+%llx)\n", ptr, len, vfs_inokey(ino, tmp), off);
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
        assert(pg != NULL); // TODO - Better handling this
        memcpy(buf, (void *)pg, PAGE_SIZE);
        // TODO -- We're suppose to use those pages an release them at kunmap only !
        // ((inode_t *)ino)->fops->release(ino, off, pg);
        len -= PAGE_SIZE;
        buf += PAGE_SIZE;
        off += PAGE_SIZE;
    }

    mp->node.value_ = ptr;
    bbtree_insert(&map_tree, &mp->node);
    return ptr;
}

void kunmap(void *addr, size_t len)
{
    --kmapCount;
    map_page_t *mp = bbtree_search_eq(&map_tree, addr, map_page_t, node);
    if (mp != NULL) {
        char tmp[64];
        kprintf(-1, "- kunmap (%p, %d, %s+%llx)\n", addr, len, vfs_inokey(mp->ino, tmp), mp->off);
        bbtree_remove(&map_tree, addr);
        bool dirty = mp->access & 2 ? true : false;
        int i;
        for (i = 0; i < mp->len; ++i) {
            page_t pg = mp->pgs[i];
            if (dirty)
                memcpy(pg, mp->ptr, PAGE_SIZE);
            mp->ino->fops->release(mp->ino, mp->off, pg, dirty);
            mp->off += PAGE_SIZE;
            mp->ptr += PAGE_SIZE;
        }
        kfree(mp);
    } else
        kprintf(-1, "- kunmap (%p, %d, -)\n", addr, len);
    _vfree(addr);
}

void page_release(page_t page)
{
    _vfree((void *)page);
}

page_t page_read(size_t address)
{
    void *page = _valloc(PAGE_SIZE);
    memcpy(page, address, PAGE_SIZE);
    // kprintf(-1, "Page shadow copy at %p \n", page);
    return (page_t)page;
}


void kprintf(int log, const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    vprintf(msg, ap);
    va_end(ap);
}

// void sleep_timer(long timeout)
// {
//     usleep(MAX(1, timeout));
// }


void futex_init() {}


// page_t mmu_read(size_t address)
// {
//     return address;
// }

#include <kernel/stdc.h>
#include <Windows.h>

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

