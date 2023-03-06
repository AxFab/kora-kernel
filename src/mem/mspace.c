///*
// *      This file is part of the KoraOS project.
// *  Copyright (C) 2015-2021  <Fabien Bavent>
// *
// *  This program is free software: you can redistribute it and/or modify
// *  it under the terms of the GNU Affero General Public License as
// *  published by the Free Software Foundation, either version 3 of the
// *  License, or (at your option) any later version.
// *
// *  This program is distributed in the hope that it will be useful,
// *  but WITHOUT ANY WARRANTY; without even the implied warranty of
// *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// *  GNU Affero General Public License for more details.
// *
// *  You should have received a copy of the GNU Affero General Public License
// *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
// *
// *   - - - - - - - - - - - - - - -
// */
//#include <kernel/memory.h>
//#include <kora/mcrs.h>
//#include <assert.h>
//#include <errno.h>
//#include <kernel/dlib.h>
//
//mspace_t *mspace_from(size_t vaddr)
//{
//	mspace_t *mspace = NULL;
//	if (vaddr >= __mmu.kspace->lower_bound && vaddr < __mmu.kspace->upper_bound)
//		mspace = __mmu.kspace;
//	else if (vaddr >= __mmu.uspace->lower_bound && vaddr < __mmu.uspace->upper_bound)
//		mspace = __mmu.uspace;
//	return mspace;
//}
//
// /* Check in an address interval is available on this address space */
//static bool mspace_is_available(mspace_t *mspace, size_t address, size_t length)
//{
//	assert(splock_locked(&mspace->lock));
//	if (address < mspace->lower_bound || address + length > mspace->upper_bound)
//		return false;
//	vma_t *vma = bbtree_search_le(&mspace->tree, address, vma_t, node);
//	if (vma == NULL) {
//		vma = bbtree_first(&mspace->tree, vma_t, node);
//		if (vma != NULL && address + length > vma->node.value_)
//			return false;
//	} else {
//		if (vma->node.value_ + vma->length > address)
//			return false;
//		vma = bbtree_next(&vma->node, vma_t, node);
//		if (vma != NULL && address + length > vma->node.value_)
//			return false;
//	}
//	return true;
//}
//
///* Find the first available interval on this address space of the desired length */
//static size_t mspace_find_free(mspace_t *mspace, size_t length)
//{
//	assert(splock_locked(&mspace->lock));
//	vma_t *next;
//	vma_t *vma = bbtree_first(&mspace->tree, vma_t, node);
//	if (vma == NULL || mspace->lower_bound + length <= vma->node.value_)
//		return mspace->lower_bound;
//
//	for (;;) {
//		next = bbtree_next(&vma->node, vma_t, node);
//		if (next == NULL
//			|| vma->node.value_ + vma->length + length <= next->node.value_)
//			break;
//		vma = next;
//	}
//	if (next == NULL
//		&& vma->node.value_ + vma->length + length > mspace->upper_bound)
//		return 0;
//	return vma->node.value_ + vma->length;
//}
//
///* Utility method used to apply a change on a address interval.
// *
// * This method browse VMA on an interval and split VMA partially in or out
// * the interval. Note that every addresses on the interval need to be
// * accessible or the function will stoy mid-way and return an error.
// */
//static int mspace_interval(mspace_t *mspace, size_t address, size_t length, int(*action)(mspace_t *, vma_t *, int), int arg)
//{
//	vma_t *vma;
//	if (address < mspace->lower_bound || address >= mspace->upper_bound ||
//		length == 0 || address + length >= mspace->upper_bound ||
//		address + length < address || (address & (PAGE_SIZE - 1)) != 0 || 
//		(length & (PAGE_SIZE - 1)) != 0) {
//		errno = EINVAL;
//		return -1;
//	}
//
//	splock_lock(&mspace->lock);
//	while (length != 0) {
//		vma = bbtree_search_le(&mspace->tree, address, vma_t, node);
//		if (vma == NULL || vma->node.value_ + vma->length <= address) {
//			errno = ENOENT;
//			splock_unlock(&mspace->lock);
//			return -1;
//		}
//
//		if (vma->node.value_ != address) {
//			assert(vma->node.value_ < address);
//			vma = vma_split(mspace, vma, address - vma->node.value_);
//			if (vma == NULL) {
//				errno = EINVAL;
//				splock_unlock(&mspace->lock);
//				return -1;
//			}
//		}
//
//		assert(vma->node.value_ == address);
//		if (vma->length > length)
//			vma_split(mspace, vma, length);
//
//		address += vma->length;
//		length -= vma->length;
//		if (action(mspace, vma, arg)) {
//			assert(errno != 0);
//			splock_unlock(&mspace->lock);
//			return -1;
//		}
//	}
//
//	splock_unlock(&mspace->lock);
//	errno = 0;
//	return 0;
//}
//
///* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
//
///* Map a memory area inside the provided address space. */
//void *mspace_map(mspace_t *mspace, size_t address, size_t length, inode_t *ino, xoff_t offset, int flags)
//{
//	// Check parameters
//	if ((address & (PAGE_SIZE - 1)) != 0 || (length & (PAGE_SIZE - 1)) != 0) {
//		errno = EINVAL;
//		return NULL;
//	} else if ((offset & (PAGE_SIZE - 1)) != 0) {
//		errno = EINVAL;
//		return NULL;
//	} else if (length == 0 || length > __mmu.max_vma_size) {
//		errno = E2BIG;
//		return NULL;
//	}
//
//	splock_lock(&mspace->lock);
//
//	// If we have an address, check availability
//	if (address != 0) {
//		if (!mspace_is_available(mspace, address, length)) {
//			if (flags & VMA_FIXED) {
//				splock_unlock(&mspace->lock);
//				errno = ERANGE;
//				return NULL;
//			}
//			address = 0;
//		}
//	}
//
//	// If we don't have address yet, look for one
//	if (address == 0) {
//		address = mspace_find_free(mspace, length);
//		if (address == 0) {
//			splock_unlock(&mspace->lock);
//			errno = ENOMEM;
//			return NULL;
//		}
//	}
//
//	// Create the VMA
//	vma_t *vma = vma_create(mspace, address, length, ino, offset, flags);
//	if (vma == NULL) {
//		splock_unlock(&mspace->lock);
//		return NULL;
//	}
//
//	errno = 0;
//	splock_unlock(&mspace->lock);
//	return (void *)address;
//}
//
///* Change the flags of a memory area. */
//int mspace_protect(mspace_t *mspace, size_t address, size_t length, int flags)
//{
//	return mspace_interval(mspace, address, length, vma_protect, flags);
//}
//
///* Close the memory area. */
//int mspace_unmap(mspace_t *mspace, size_t address, size_t length)
//{
//	return mspace_interval(mspace, address, length, vma_close, 0);
//}
//
///* Create a memory space for a user application */
//mspace_t *mspace_create()
//{
//	mspace_t *mspace = (mspace_t *)kalloc(sizeof(mspace_t));
//	bbtree_init(&mspace->tree);
//	splock_init(&mspace->lock);
//	mmu_create_uspace(mspace);
//	atomic_inc(&mspace->users);
//	return mspace;
//}
//
///* - */
//mspace_t *mspace_open(mspace_t *mspace)
//{
//	atomic_inc(&mspace->users);
//	return mspace;
//}
//
///* - */
//mspace_t *mspace_clone(mspace_t *model)
//{
//	assert(model != __mmu.kspace);
//	mspace_t *mspace = mspace_create();
//	splock_lock(&model->lock);
//	assert(mspace->lower_bound == model->lower_bound);
//	assert(mspace->upper_bound == model->upper_bound);
//
//	vma_t *vma = bbtree_first(&model->tree, vma_t, node);
//	for (; vma; vma = bbtree_next(&vma->node, vma_t, node)) {
//		// int type = vma->flags & VMA_TYPE;
//		vma_clone(mspace, vma);
//	}
//
//	splock_unlock(&model->lock);
//	return mspace;
//}
//
///* Release all VMAs */
//void mspace_sweep(mspace_t *mspace)
//{
//	splock_lock(&mspace->lock);
//	vma_t *vma = bbtree_first(&mspace->tree, vma_t, node);
//	while (vma != NULL) {
//		vma_close(mspace, vma, 0);
//		vma = bbtree_first(&mspace->tree, vma_t, node);
//	}
//	splock_unlock(&mspace->lock);
//}
//
///* Decrement RCU and might release all VMAs and free data structures. */
//void mspace_close(mspace_t *mspace)
//{
//	if (atomic_xadd(&mspace->users, -1) != 1)
//		return;
//	mspace_sweep(mspace);
//	// Free memory space
//	mmu_destroy_uspace(mspace);
//	if (mspace->proc) {
//		dlib_destroy(mspace->proc);
//	}
//	kfree(mspace);
//}
//
///* Search a VMA structure at a specific address. */
//vma_t *mspace_search_vma(mspace_t *mspace, size_t address)
//{
//	vma_t *vma = NULL;
//	if (address < mspace->lower_bound || address >= mspace->upper_bound)
//		return NULL;
//
//	splock_lock(&mspace->lock);
//	vma = bbtree_search_le(&mspace->tree, address, vma_t, node);
//	if (vma == NULL || vma->node.value_ + vma->length <= address) {
//		splock_unlock(&mspace->lock);
//		return NULL;
//	}
//
//	return vma;
//}
//
//void vma_debug(vma_t *vma);
//
///* Display the state of the current address space */
//void mspace_display(mspace_t *mspace)
//{
//	splock_lock(&mspace->lock);
//	kprintf(KL_DBG, "------------------------------------------------\n");
//	kprintf(KL_DBG,
//		"%p-%p virtual: %d KB   physical: %d KB   shared: %d KB   used: %d KB   table: %d KB\n",
//		mspace->lower_bound, mspace->upper_bound,
//		mspace->v_size * 4, mspace->p_size * 4/* / 1024*/,
//		mspace->s_size * 4, mspace->a_size * 4, mspace->t_size * 4);
//	kprintf(KL_DBG, "------------------------------------------------\n");
//	char *buf = kalloc(512);
//	vma_t *vma = bbtree_first(&mspace->tree, vma_t, node);
//	while (vma) {
//		vma_print(buf, 512, vma);
//		if (vma->length <= 128 * _Kib_) {
//			kprintf(KL_DBG, "%s", buf);
//			vma_debug(vma);
//		} else {
//			kprintf(KL_DBG, "%s\n", buf);
//		}
//		vma = bbtree_next(&vma->node, vma_t, node);
//	}
//	kfree(buf);
//	splock_unlock(&mspace->lock);
//}
//
