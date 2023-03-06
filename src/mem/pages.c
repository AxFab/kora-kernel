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
bool bitstest(uint8_t *ptr, int start, int count);

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
		kprintf(-1, "===> %p (%d)\n", (size_t)(idx * PAGE_SIZE), __mmu.free_pages);
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
		assert(mz->ptr != NULL);
		assert(bitstest(mz->ptr, idx - mz->offset, 1));
		/* Release page */
		bitsclr(mz->ptr, idx - mz->offset, 1);
		mz->free++;
		atomic_inc(&__mmu.free_pages);
		splock_unlock(&mz->lock);
		kprintf(-1, "<=== %p (%d)\n", (size_t)(idx * PAGE_SIZE), __mmu.free_pages);
		return;
	}
	kprintf(KL_ERR, "Page '%p' provided to page_release is not referenced.\n", paddress);
}


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

typedef struct page_sharing_entry
{
	size_t page;
	int count;
} page_sharing_entry_t;


bool page_shared(page_sharing_t *share, size_t page, int count)
{
	if (share == NULL) {
		assert(count == 0);
		return true;
	}
	bool last = false;
	splock_lock(&share->lock);
	page_sharing_entry_t *ps = hmp_get(&share->map, (char *)&page, sizeof(size_t));
	if (ps == NULL) {
		if (count == 0) {
			splock_unlock(&share->lock);
			return true;
		}
		assert(count > 0);
		ps = kalloc(sizeof(page_sharing_entry_t));
		ps->page = page;
		ps->count = count;
		hmp_put(&share->map, (char *)&page, sizeof(size_t), ps);
	} else {
		ps->count += count;
		if (ps->count <= 0) {
			hmp_remove(&share->map, (char *)&page, sizeof(size_t));
			last = true;
			kfree(ps);
		}
	}
	kprintf(-1, "- shared page %p (%d)\n", (void *)page, last ? 0 : ps->count);
	splock_unlock(&share->lock);
	return last;
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
//
//#define PF_ERR "\033[91mSIGSEGV\033[0m "
//
///* Resolve a page fault */
// int page_fault(size_t address, int reason)
// {
// 	return vmsp_resolve(memory_space_at(address), address, reason & PGFLT_MISSING, reason & PGFLT_WRITE);
// }
