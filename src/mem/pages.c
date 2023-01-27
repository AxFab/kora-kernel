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

void bitsset(uint8_t *ptr, int start, int count);
int bitschrz(uint8_t *ptr, int len);
void bitsclr(uint8_t *ptr, int start, int count);

typedef struct mzone mzone_t;
struct mzone
{
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

	if (__mmu.upper_physical_page < (size_t)(start + count))
		__mmu.upper_physical_page = (size_t)(start + count);

	__mmu.pages_amount += count;
	__mmu.free_pages += count;
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
	for ll_each(&lzone, mz, mzone_t, node)
	{
		splock_lock(&mz->lock);
		if (mz->available == mz->free && mz->ptr) {
			kfree(mz->ptr);
			mz->ptr = NULL;
		}
		splock_unlock(&mz->lock);
	}

	mzone_t *it = ll_first(&lzone, mzone_t, node);
	while (it) {
		mz = it;
		it = ll_next(&it->node, mzone_t, node);
		kfree(mz);
	}
}


/* Allocate a single page for the system and return it's physical address */
size_t page_new()
{
	mzone_t *mz;
	/* Look on each memory zone */
	for ll_each(&lzone, mz, mzone_t, node)
	{
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
		atomic_dec(&__mmu.free_pages);
		bitsset(mz->ptr, idx, 1);
		idx += mz->offset;
		splock_unlock(&mz->lock);
		// kprintf(-1, "===> %p (%d)\n", (size_t)(idx * PAGE_SIZE), __mmu.free_pages);
		return idx * PAGE_SIZE;
	}

	kprintf(KL_ERR, "Error, no more pages available\n");
	for (;;);
}


/* Mark a physique page, returned by `page_new`, as available again */
void page_release(size_t paddress)
{
	mzone_t *mz;
	long idx = paddress / PAGE_SIZE;
	/* Look on each memory zone */
	for ll_each(&lzone, mz, mzone_t, node)
	{
		splock_lock(&mz->lock);
		if (idx < mz->offset + mz->reserved || idx >= mz->offset + mz->reserved + mz->available) {
			splock_unlock(&mz->lock);
			continue;
		}
		/* Release page */
		bitsclr(mz->ptr, idx - mz->offset, 1);
		mz->free++;
		atomic_inc(&__mmu.free_pages);
		splock_unlock(&mz->lock);
		// kprintf(-1, "<=== %p (%d)\n", (size_t)(idx * PAGE_SIZE), __mmu.free_pages);
		return;
	}
	kprintf(KL_ERR, "Page '%p' provided to page_release is not referenced.\n", paddress);
}


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

/* Free all used pages into a range of virtual addresses */
//void page_sweep(mspace_t *mspace, size_t address, size_t length, bool clean)
//{
//    assert((address & (PAGE_SIZE - 1)) == 0);
//    assert((length & (PAGE_SIZE - 1)) == 0);
//    while (length) {
//        if (clean && mmu_read(address) != 0)
//            memset((void *)address, 0, PAGE_SIZE);
//        size_t pg = mmu_drop(address);
//        if (pg != 0) {
//            mspace->p_size--;
//            page_release(pg);
//        }
//        address += PAGE_SIZE;
//        length -= PAGE_SIZE;
//    }
//}

#define PF_ERR "\033[91mSIGSEGV\033[0m "

/* Resolve a page fault */
int page_fault(size_t address, int reason)
{
	int ret = 0;
	mspace_t *mspace = mspace_from(address);
	if (mspace == NULL) {
		kprintf(KL_PF, PF_ERR"Address is outside of addressable space '%p'\n", (void *)address);
		return -1;
	}

	// assert(kCPU.irq_semaphore == 0); //But IRQ must still be disabld!
	// kprintf(KLOG_PF, "\033[91m#PF\033[31m %p\033[0m\n", address);
	vma_t *vma = mspace_search_vma(mspace, address);
	if (vma == NULL) {
		kprintf(KL_PF, PF_ERR"No mapping at this address '%p'\n", (void *)address);
		return -1;
	}

	if (reason & PGFLT_WRITE && !(vma->flags & VM_WR) && !(vma->flags & VMA_COW)) {
		splock_unlock(&mspace->lock);
		kprintf(KL_PF, PF_ERR"Can't write on read-only memory at '%p'\n", (void *)address);
		return -1;
	}

	size_t vaddr = ALIGN_DW(address, PAGE_SIZE);
	if (reason & PGFLT_MISSING) {
		++__mmu.soft_page_fault;
		// size_t pg = vma_readpage(vma, vaddr);
		// if (pg == 0) {
		//    splock_unlock(&mspace->lock);
		//    page = look_for_page();
		//    -- redo find and check we don't already have the page !?
		// }
		ret = vma_resolve(vma, vaddr, PAGE_SIZE);
		if (ret != 0) {
			splock_unlock(&mspace->lock);
			kprintf(KL_PF, PF_ERR"Unable to resolve page at '%p'\n", (void *)address);
			return -1;
		}
	}

	if (reason & PGFLT_WRITE && (vma->flags & VMA_COW)) {
		ret = vma_copy_on_write(vma, vaddr, PAGE_SIZE);
		if (ret != 0) {
			kprintf(KL_PF, PF_ERR"Error on copy and write at %p!\n", (void *)address);
			return -1;
		}
	}
	splock_unlock(&mspace->lock);
	return 0;
}
