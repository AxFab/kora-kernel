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
#include "mspace.h"
#include <kernel/cpu.h>
#include <kernel/vfs.h>
#include <kora/mcrs.h>
#include <kora/llist.h>
#include <kora/splock.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/signum.h>

typedef struct mzone mzone_t;
struct mzone {
    long offset;
    long reserved;
    long available;
    long count;
    long free;
    llnode_t node;
    splock_t lock;
    uint8_t *ptr;
    int flags;
};

llhead_t lzone = INIT_LLHEAD;

void page_range(long long base, long long length)
{
    long long obase = base;
    assert(base >= 0 && length > 0);
    base = ALIGN_UP(base, PAGE_SIZE);
    length = ALIGN_DW(length - (base - obase), PAGE_SIZE);
    long count = length / PAGE_SIZE;
    long start = base / PAGE_SIZE;

    if (kMMU.upper_physical_page < (unsigned)start + count)
        kMMU.upper_physical_page = (unsigned)start + count;

    kMMU.pages_amount += count;
    kMMU.free_pages += count;
    int i = start / 8;
    int j = start % 8;
#if 1
    mzone_t *zn = kalloc(sizeof(mzone_t));
    zn->offset = i * 8;
    zn->reserved = j;
    zn->available = count;
    zn->count = ALIGN_UP(count + j, 8);
    zn->free = count;
    ll_append(&lzone, &zn->node);
    kprintf(-1, "page at %d.%d count %d\n", i, j, count);
#else
    while (count > 0 && j != 0) {
        MMU_BMP[i] = MMU_BMP[i] & ~(1 << j);
        j = (j + 1) % 8;
        count--;
    }
    start = ALIGN_UP(start + 1, 8);
    memset(&MMU_BMP[start / 8], 0, count / 8);
    start += count & (~8);
    count = count % 8;
    i = start / 8;
    j = 0;
    while (count > 0) {
        MMU_BMP[i] = MMU_BMP[i] & ~(1 << j);
        j++;
        count--;
    }
#endif
}

void bitsclr(uint8_t *ptr, int start, int count)
{
    int i = start / 8;
    int j = start % 8;
    while (count > 0 && j != 0) {
        ptr[i] &= ~(1 << j);
        j = (j + 1) % 8;
        count--;
    }
    i = ALIGN_UP(start, 8) / 8;
    while (count > 8) {
        ptr[i] = 0;
        i++;
        count -= 8;
    }
    j = 0;
    while (count > 0) {
        ptr[i] &= ~(1 << j);
        j++;
        count--;
    }
}

void bitsset(uint8_t *ptr, int start, int count)
{
    int i = start / 8;
    int j = start % 8;
    while (count > 0 && j != 0) {
        ptr[i] |= (1 << j);
        j = (j + 1) % 8;
        count--;
    }
    i = ALIGN_UP(start, 8) / 8;
    while (count > 8) {
        ptr[i] = ~0;
        i++;
        count -= 8;
    }
    j = 0;
    while (count > 0) {
        ptr[i] |= (1 << j);
        j++;
        count--;
    }
}

int bitschrz(uint8_t *ptr, int len)
{
    int i = 0, j = 0;
    while (i < len / 8 && ptr[i] == 0xFF)
        i++;
    if (i >= len / 8)
        return -1;
    uint8_t by = ptr[i];
    while (by & 1) {
        j++;
        by = by >> 1;
    }
    return i * 8 + j;
}


/* Allocate a single page for the system and return it's physical address */
page_t page_new()
{
#if 1
    mzone_t *mz;
    /* Look on each memory zone */
    for ll_each(&lzone, mz, mzone_t, node) {
        splock_lock(&mz->lock);
        if (mz->free == 0) {
            splock_unlock(&mz->lock);
            continue;
        }
        /* Allocate bitmap if required */
        if (mz->ptr == NULL) {
            mz->ptr = kalloc(mz->count / 8);
            memset(mz->ptr, 0xFF, mz->count / 8);
            bitsclr(mz->ptr, mz->reserved, mz->available);
        }

        /* Look for available page */
        long idx = bitschrz(mz->ptr, mz->count);
        if (idx < 0)
            kpanic("Memory page bitmap is corrupted!");
        mz->free--;
        bitsset(mz->ptr, idx, 1);
        idx += mz->offset;
        splock_unlock(&mz->lock);
        return idx * PAGE_SIZE;
    }
    kpanic("No page available");
#else
    int i = 0, j = 0;
    while (i < MMU_LG && MMU_BMP[i] == 0xff)
        ++i;
    if (i == MMU_LG)
        kpanic("No page available");
    uint8_t page_byte = MMU_BMP[i];
    while (page_byte & 1) {
        ++j;
        page_byte = page_byte >> 1;
    }
    MMU_BMP[i] = MMU_BMP[i] | (1 << j);
    kMMU.free_pages--;
    return (page_t)(i * 8 + j) * PAGE_SIZE;
#endif
}

/* Look for count pages in continuous memory */
page_t page_get(int zone, int count)
{
    assert((count & 7) == 0);
    count = ALIGN_UP(count, 8) / 8;
    int i = 0, j = 0;
    while (i < MMU_LG) {
        while (i < MMU_LG && MMU_BMP[i] != 0)
            ++i;
        if (i + count >= MMU_LG)
            return 0;
        j = 0;
        while (j < count && MMU_BMP[i + j] == 0)
            ++j;
        if (j == count) {
            memset(&MMU_BMP[i], 0xFF, count);
            return (page_t)(i * 8 * PAGE_SIZE);
        }
        i += j;
    }
    return 0;
}

/* Mark a physique page, returned by `page_new`, as available again */
void page_release(page_t paddress)
{
#if 1
    mzone_t *mz;
    long idx = paddress / PAGE_SIZE;
    /* Look on each memory zone */
    for ll_each(&lzone, mz, mzone_t, node) {
        splock_lock(&mz->lock);
        if (idx < mz->offset + mz->reserved || idx >= mz->offset + mz->reserved + mz->available) {
            splock_unlock(&mz->lock);
            continue;
        }
        /* Release page */
        bitsclr(mz->ptr, idx - mz->offset, 1);
        mz->free++;
        splock_unlock(&mz->lock);
        return;
    }
    kpanic("Page is not referenced");
#else
    int i = (paddress / PAGE_SIZE) / 8;
    int j = (paddress / PAGE_SIZE) % 8;
    assert(i < MMU_LG && (MMU_BMP[i] & (1 << j)) != 0);
    kMMU.free_pages++;
    MMU_BMP[i] = MMU_BMP[i] & ~(1 << j);
#endif
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

/* Free all used pages into a range of virtual addresses */
void page_sweep(mspace_t *mspace, size_t address, size_t length, bool clean)
{
    assert((address & (PAGE_SIZE - 1)) == 0);
    assert((length & (PAGE_SIZE - 1)) == 0);
    while (length) {
        if (clean && mmu_read(address) != 0)
            memset((void *)address, 0, PAGE_SIZE);
        page_t pg = mmu_drop(address);
        if (pg != 0) {
            mspace->p_size--;
            page_release(pg);
        }
        address += PAGE_SIZE;
        length -= PAGE_SIZE;
    }
}


/* Resolve a page fault */
int page_fault(mspace_t *mspace, size_t address, int reason)
{
    int ret = 0;
    // assert(kCPU.irq_semaphore == 0); //But IRQ must still be disabld!
    // kprintf(KLOG_PF, "\033[91m#PF\033[31m %p\033[0m\n", address);
    vma_t *vma = mspace_search_vma(mspace, address);
    if (vma == NULL) {
        kprintf(KLOG_PF, "\033[31mSIGSEGV at %p\033[0m\n", address);
        task_fatal("No mapping at this address", SIGSEGV);
    }

    if (reason & PGFLT_WRITE && !(vma->flags & VMA_WRITE)
        && !(vma->flags & VMA_COPY_ON_WRITE)) {
        splock_unlock(&vma->mspace->lock);
        kprintf(KLOG_PF, "\033[31mSIGSEGV at %p\033[0m\n", address);
        task_fatal("Can't write on read-only memory", SIGSEGV);
    }

    address = ALIGN_DW(address, PAGE_SIZE);
    if (reason & PGFLT_MISSING) {
        ++kMMU.soft_page_fault;
        ret = vma_resolve(vma, address, PAGE_SIZE);
    }
    if (ret != 0) {
        splock_unlock(&vma->mspace->lock);
        kprintf(KLOG_PF, "\033[31mSIGSEGV at %p\033[0m\n", address);
        task_fatal("Unable to resolve page", SIGSEGV);
    }
    if (reason & PGFLT_WRITE && (vma->flags & VMA_COPY_ON_WRITE))
        ret = vma_copy_on_write(vma, address, PAGE_SIZE);
    if (ret != 0)
        kprintf(KLOG_PF, "\033[91m#PF\033[31m Error on copy and write at %p!\033[0m\n",
                address);
    splock_unlock(&vma->mspace->lock);
    return 0;
}
