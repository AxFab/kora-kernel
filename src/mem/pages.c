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
#include <kernel/memory.h>
#include <kernel/tasks.h>
#include <kora/mcrs.h>
#include <kora/llist.h>
#include <kora/splock.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/signum.h>

void bitsset(uint8_t *ptr, int start, int count);
int bitschrz(uint8_t *ptr, int len);
void bitsclr(uint8_t *ptr, int start, int count);

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

    if (kMMU.upper_physical_page < (size_t)(start + count))
        kMMU.upper_physical_page = (size_t)(start + count);

    kMMU.pages_amount += count;
    kMMU.free_pages += count;
    while (count > 0) {
        int i = start / 8;
        int j = start % 8;
        int pgs = count;
        if (pgs + j > PAGE_SIZE * 4)
            pgs = PAGE_SIZE * 4 - j;
        mzone_t *zn = kalloc(sizeof(mzone_t));
        zn->offset = i * 8;
        zn->reserved = j;
        zn->available = pgs;
        zn->count = ALIGN_UP(pgs + j, 8);
        zn->free = pgs;
        ll_append(&lzone, &zn->node);
        kprintf(-1, "Found %d pages at %d[%d.%d]\n", pgs, start, i, j);
        count -= pgs;
        start = zn->offset + zn->count;
    }
}

void page_teardown()
{
    mzone_t *mz;
    for ll_each(&lzone, mz, mzone_t, node) {
        splock_lock(&mz->lock);
        if (mz->available == mz->free) {
            kfree(mz->ptr);
            mz->ptr = NULL;
        }
        splock_unlock(&mz->lock);
        // kprintf(-1, "pz %x\n", mz);
    }

    mzone_t *it = ll_first(&lzone, mzone_t, node);
    while (it) {
        mz = it;
        it = ll_next(&it->node, mzone_t, node);
        //
        kfree(mz);
    }
}



#ifdef KORA_KRN
/* Allocate a single page for the system and return it's physical address */
page_t page_new()
{
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
        assert(idx >= 0);
        mz->free--;
        bitsset(mz->ptr, idx, 1);
        idx += mz->offset;
        splock_unlock(&mz->lock);
        return idx * PAGE_SIZE;
    }

    kprintf(KL_ERR, "Error, no more pages available\n");
    for (;;);
}


/* Mark a physique page, returned by `page_new`, as available again */
void page_release(page_t paddress)
{
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
    kprintf(KL_ERR, "Page '%p' provided to page_release is not referenced.\n", paddress);
}
#endif

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

/* Free all used pages into a range of virtual addresses */
//void page_sweep(mspace_t *mspace, size_t address, size_t length, bool clean)
//{
//    assert((address & (PAGE_SIZE - 1)) == 0);
//    assert((length & (PAGE_SIZE - 1)) == 0);
//    while (length) {
//        if (clean && mmu_read(address) != 0)
//            memset((void *)address, 0, PAGE_SIZE);
//        page_t pg = mmu_drop(address);
//        if (pg != 0) {
//            mspace->p_size--;
//            page_release(pg);
//        }
//        address += PAGE_SIZE;
//        length -= PAGE_SIZE;
//    }
//}


/* Resolve a page fault */
int page_fault(mspace_t *mspace, size_t address, int reason)
{
    int ret = 0;
    // assert(kCPU.irq_semaphore == 0); //But IRQ must still be disabld!
    // kprintf(KLOG_PF, "\033[91m#PF\033[31m %p\033[0m\n", address);
    vma_t *vma = mspace_search_vma(mspace, address);
    if (vma == NULL) {
        kprintf(KL_PF, "\033[31mSIGSEGV at %p\033[0m\n", address);
        task_fatal("No mapping at this address", SIGSEGV);
    }

    if (reason & PGFLT_WRITE && !(vma->flags & VM_WR) && !(vma->flags & VMA_COW)) {
        splock_unlock(&vma->mspace->lock);
        kprintf(KL_PF, "\033[31mSIGSEGV at %p\033[0m\n", address);
        task_fatal("Can't write on read-only memory", SIGSEGV);
    }

    address = ALIGN_DW(address, PAGE_SIZE);
    if (reason & PGFLT_MISSING) {
        ++kMMU.soft_page_fault;
        ret = vma_resolve(vma, address, PAGE_SIZE);
    }
    if (ret != 0) {
        splock_unlock(&vma->mspace->lock);
        kprintf(KL_PF, "\033[31mSIGSEGV at %p\033[0m\n", address);
        task_fatal("Unable to resolve page", SIGSEGV);
    }
    if (reason & PGFLT_WRITE && (vma->flags & VMA_COW))
        ret = vma_copy_on_write(vma, address, PAGE_SIZE);
    if (ret != 0)
        kprintf(KL_PF, "\033[91m#PF\033[31m Error on copy and write at %p!\033[0m\n",
                address);
    splock_unlock(&vma->mspace->lock);
    return 0;
}
