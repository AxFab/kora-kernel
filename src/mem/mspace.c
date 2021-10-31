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
// #include <kernel/vfs.h>
#include <kora/mcrs.h>
#include <kora/mcrs.h>
// #include "mspace.h"
#include <assert.h>
#include <errno.h>


/** Check in an address interval is available on this address space */
static bool mspace_is_available(mspace_t *mspace, size_t address, size_t length)
{
    assert(splock_locked(&mspace->lock));
    if (address < mspace->lower_bound || address + length > mspace->upper_bound)
        return false;
    vma_t *vma = bbtree_search_le(&mspace->tree, address, vma_t, node);
    if (vma == NULL) {
        vma = bbtree_first(&mspace->tree, vma_t, node);
        if (vma != NULL && address + length > vma->node.value_)
            return false;
    } else {
        if (vma->node.value_ + vma->length > address)
            return false;
        vma = bbtree_next(&vma->node, vma_t, node);
        if (vma != NULL && address + length > vma->node.value_)
            return false;
    }
    return true;
}

/** Find the first available interval on this address space of the desired length */
static size_t mspace_find_free(mspace_t *mspace, size_t length)
{
    assert(splock_locked(&mspace->lock));
    vma_t *next;
    vma_t *vma = bbtree_first(&mspace->tree, vma_t, node);
    if (vma == NULL || mspace->lower_bound + length <= vma->node.value_)
        return mspace->lower_bound;

    for (;;) {
        next = bbtree_next(&vma->node, vma_t, node);
        if (next == NULL
            || vma->node.value_ + vma->length + length <= next->node.value_)
            break;
        vma = next;
    }
    if (next == NULL
        && vma->node.value_ + vma->length + length > mspace->upper_bound)
        return 0;
    return vma->node.value_ + vma->length;
}

/** Check and transfrom the flags before assign to a new VMA. */
static int mspace_set_flags(int flags, bool is_kernel)
{
    int caps = flags & VM_RWX;
    /* Check capabilities are set correcly */
    if ((flags & caps) != (flags & VM_RWX))
        return 0;
    /* Forbid a map write and execute */
    // if (caps & VMA_WRITE && caps & VMA_EXEC)
    //     return 0;
    /* Flags forbidden, intern code only */
    if (flags & VMA_COW)
        return 0;

    int mask = VM_RESOLVE;
    int type = flags & VMA_TYPE;
    if (type == 0)
        type = (is_kernel && flags & VM_PHYSIQ) ? VMA_PHYS : VMA_ANON;

    switch (type) {
    case VMA_HEAP:
        /* Heaps are always private and RW */
        if (caps != VM_RW || flags & VMA_SHARED)
            return 0;
        break;
    case VMA_STACK:
        /* Stacks are always private and RW */
        if (caps != VM_RW || flags & VMA_SHARED)
            return 0;
        break;
    case VMA_PIPE:
        /* Pipes are always on kernel and RW */
        if (caps != VM_RW || flags & VMA_SHARED || !is_kernel)
            return 0;
        break;
    case VMA_PHYS:
        /* Physical mapping is always on kernel, can't be executed */
        if (flags & VMA_SHARED || !is_kernel || caps & VM_EX)
            return 0;
        mask |= VM_PHYSIQ & VM_UNCACHABLE;
        break;
    case VMA_ANON:
        if (flags & VMA_SHARED || caps & VM_EX)
            return 0;
        break;
    case VMA_FILE:
        if (caps & VM_EX)
            return 0;
        break;
    case VMA_EXEC:
        break; // TODO - put RWX but only RX on capability !
    default:
        return 0;
    }

    int nflags = (caps << 3) | caps | type | (flags & mask);

    if (flags & VMA_SHARED)
        nflags |= VMA_SHARED;

    return nflags;
}

/** Utility method used to apply a change on a address interval.
 *
 * This method browse VMA on an interval and split VMA partially in or out
 * the interval. Note that every addresses on the interval need to be
 * accessible or the function will stoy mid-way and return an error.
 */
static int mspace_interval(mspace_t *mspace, size_t address, size_t length, int(*action)(mspace_t *, vma_t *, int), int arg)
{
    vma_t *vma;
    assert(address >= mspace->lower_bound
           && address + length <= mspace->upper_bound);
    assert((address & (PAGE_SIZE - 1)) == 0);
    assert((length & (PAGE_SIZE - 1)) == 0);
    if (address < mspace->lower_bound || address >= mspace->upper_bound ||
        length == 0 || address + length >= mspace->upper_bound ||
        address + length < address) {
        errno = EINVAL;
        return -1;
    }

    splock_lock(&mspace->lock);
    while (length != 0) {
        vma = bbtree_search_le(&mspace->tree, address, vma_t, node);
        if (vma == NULL) {
            errno = ENOENT;
            splock_unlock(&mspace->lock);
            return -1;
        }

        if (vma->node.value_ != address) {
            assert(vma->node.value_ < address);
            vma = vma_split(mspace, vma, address - vma->node.value_);
            if (vma == NULL) {
                errno = EINVAL;
                return -1;
            }
        }

        assert(vma->node.value_ == address);
        if (vma->length > length)
            vma_split(mspace, vma, length);

        address += vma->length;
        length -= vma->length;
        if (action(mspace, vma, arg)) {
            assert(errno != 0);
            splock_unlock(&mspace->lock);
            return -1;
        }
    }

    splock_unlock(&mspace->lock);
    errno = 0;
    return 0;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

/* Map a memory area inside the provided address space. */
void *mspace_map(mspace_t *mspace, size_t address, size_t length, inode_t *ino, xoff_t offset, int flags)
{
    /* Check parameters */
    if ((address & (PAGE_SIZE - 1)) != 0 || (length & (PAGE_SIZE - 1)) != 0) {
        errno = EINVAL;
        return NULL;
    }
    if (ino != NULL && (offset & (PAGE_SIZE - 1)) != 0) {
        errno = EINVAL;
        return NULL;
    }

    int vflags = mspace_set_flags(flags, mspace == kMMU.kspace);
    if (length == 0 || length > kMMU.max_vma_size || vflags == 0) {
        errno = EINVAL;
        return NULL;
    }

    splock_lock(&mspace->lock);

    /* If we have an address, check availability */
    if (address != 0) {
        if (!mspace_is_available(mspace, address, length)) {
            if (flags & VMA_FIXED) {
                splock_unlock(&mspace->lock);
                errno = ERANGE;
                return NULL;
            }
            address = 0;
        }
    }

    /* If we don't have address yet, look for one */
    if (address == 0) {
        address = mspace_find_free(mspace, length);
        if (address == 0) {
            splock_unlock(&mspace->lock);
            errno = ENOMEM;
            return NULL;
        }
    }

    /* Create the VMA */
    vma_t *vma = vma_create(mspace, address, length, ino, offset, vflags);
    if (flags & VM_RESOLVE)
        vma_resolve(vma, address, length);
    errno = 0;
    splock_unlock(&mspace->lock);
    return (void *)address;
}

/* Change the flags of a memory area. */
int mspace_protect(mspace_t *mspace, size_t address, size_t length, int flags)
{
    return mspace_interval(mspace, address, length, vma_protect, flags);
}

int mspace_unmap(mspace_t *mspace, size_t address, size_t length)
{
    return mspace_interval(mspace, address, length, vma_close, 0);
}

/* Create a memory space for a user application */
mspace_t *mspace_create()
{
    mspace_t *mspace = (mspace_t *)kalloc(sizeof(mspace_t));
    bbtree_init(&mspace->tree);
    splock_init(&mspace->lock);
    mmu_create_uspace(mspace);
    atomic_inc(&mspace->users);
    return mspace;
}

mspace_t *mspace_clone(mspace_t *model)
{
    mspace_t *mspace = mspace_create();
    splock_lock(&model->lock);
    assert(mspace->lower_bound = model->lower_bound);
    assert(mspace->upper_bound = model->upper_bound);

    vma_t *vma = bbtree_first(&model->tree, vma_t, node);
    for (; vma ; vma = bbtree_next(&vma->node, vma_t, node)) {
        int type = vma->flags & VMA_TYPE;
        if (type == VMA_STACK || type == VMA_PIPE || type == VMA_PHYS)
            continue;
        vma_clone(mspace, vma);
    }

    splock_unlock(&model->lock);
    return mspace;
}


mspace_t *mspace_open(mspace_t *mspace)
{
    atomic_inc(&mspace->users);
    return mspace;
}

/* Close all VMAs */
void mspace_sweep(mspace_t *mspace)
{
    splock_lock(&mspace->lock);
    vma_t *vma = bbtree_first(&mspace->tree, vma_t, node);
    while (vma != NULL) {
        vma_close(mspace, vma, 0);
        vma = bbtree_first(&mspace->tree, vma_t, node);
    }
    /* Free MMU releated data */
    splock_unlock(&mspace->lock);
}

/* Decrement RCU and might release all VMA and free data structures. */
void mspace_close(mspace_t *mspace)
{
    if (atomic_xadd(&mspace->users, -1) == 1) {
        mspace_sweep(mspace);
        mmu_destroy_uspace(mspace);
        /* Free memory space */
        kfree(mspace);
    }
}
/* Search a VMA structure at a specific address.
 *
 * @mspace  The instance of the address space object
 * @address  An address included in the VMA we are looking for.
 *
 * Return a VMA structure.
 */
vma_t *mspace_search_vma(mspace_t *mspace, size_t address)
{
    vma_t *vma = NULL;
    if (address >= kMMU.kspace->lower_bound && address < kMMU.kspace->upper_bound) {
        splock_lock(&kMMU.kspace->lock);
        vma = bbtree_search_le(&kMMU.kspace->tree, address, vma_t, node);
        if (vma == NULL) {
            splock_unlock(&kMMU.kspace->lock);
            return NULL;
        }
    } else if (mspace != NULL && address >= mspace->lower_bound && address < mspace->upper_bound) {
        splock_lock(&mspace->lock);
        vma = bbtree_search_le(&mspace->tree, address, vma_t, node);
        if (vma == NULL) {
            splock_unlock(&mspace->lock);
            return NULL;
        }
    }

    if (vma == NULL)
        return NULL;

    if (vma->node.value_ + vma->length <= address) {
        splock_unlock(&vma->mspace->lock);
        return NULL;
    }
    return vma;
}


/* Display the state of the current address space */
void mspace_display(mspace_t *mspace)
{
    if (mspace == NULL)
        mspace = kMMU.kspace;
    splock_lock(&mspace->lock);
    kprintf(KL_DBG,
            "%p-%p mapped: %d KB   physical: %d KB   shared: %d KB   used: %d KB\n",
            mspace->lower_bound, mspace->upper_bound,
            mspace->v_size / 1024, mspace->p_size * 4/* / 1024*/,
            mspace->s_size / 1024, mspace->a_size / 1024);
    kprintf(KL_DBG, "------------------------------------------------\n");
    char *buf = kalloc(4096);
    vma_t *vma = bbtree_first(&mspace->tree, vma_t, node);
    while (vma) {
        vma_print(buf, 4096, vma);
        kprintf(KL_DBG, "%s\n", buf);
        vma = bbtree_next(&vma->node, vma_t, node);
    }
    kfree(buf);
    splock_unlock(&mspace->lock);
}


int mspace_check(mspace_t *mspace, const void *ptr, size_t len, int flags)
{
    vma_t *vma = mspace_search_vma(mspace, (size_t)ptr);
    if (vma == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (vma->mspace == kMMU.kspace) {
        splock_unlock(&vma->mspace->lock);
        errno = EINVAL;
        return -1;
    }

    if (vma->node.value_ + vma->length <= (size_t)ptr + len) {
        splock_unlock(&vma->mspace->lock);
        errno = EINVAL;
        return -1;
    }

    if ((flags & VM_WR) && !(vma->flags & VM_WR)) {
        splock_unlock(&vma->mspace->lock);
        errno = EINVAL;
        return -1;
    }

    splock_unlock(&vma->mspace->lock);
    return 0;
}

int mspace_check_str(mspace_t *mspace, const char *str, size_t max)
{
    vma_t *vma = mspace_search_vma(mspace, (size_t)str);
    if (vma == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (vma->mspace == kMMU.kspace) {
        splock_unlock(&vma->mspace->lock);
        errno = EINVAL;
        return -1;
    }

    max = MIN(max, vma->node.value_ + vma->length - (size_t)str);
    if (strnlen(str, max) >= max) {
        splock_unlock(&vma->mspace->lock);
        errno = EINVAL;
        return -1;
    }

    splock_unlock(&vma->mspace->lock);
    return 0;
}
