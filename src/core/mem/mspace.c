/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015  <Fabien Bavent>
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
#include <kernel/core.h>
#include <kernel/memory.h>
#include <kernel/vfs.h>
#include <kora/mcrs.h>
#include <assert.h>
#include <errno.h>

extern mspace_t kernel_space;
/** Check in an address interval is available on this address space */
static bool mspace_is_available(mspace_t *mspace, size_t address, size_t length)
{
    assert(splock_locked(&mspace->lock));
    if (address < mspace->lower_bound || address + length > mspace->upper_bound) {
        return false;
    }
    vma_t *vma = bbtree_search_le(&mspace->tree, address, vma_t, node);
    if (vma == NULL) {
        vma = bbtree_first(&mspace->tree, vma_t, node);
        if (vma != NULL && address + length > vma->node.value_) {
            return false;
        }
    } else {
        if (vma->node.value_ + vma->length > address) {
            return false;
        }
        vma = bbtree_next(&vma->node, vma_t, node);
        if (vma != NULL && address + length > vma->node.value_) {
            return false;
        }
    }
    return true;
}

/** Find the first available interval on this address space of the desired length */
static size_t mspace_find_free(mspace_t *mspace, size_t length)
{
    assert(splock_locked(&mspace->lock));
    vma_t *next;
    vma_t *vma = bbtree_first(&mspace->tree, vma_t, node);
    if (vma == NULL || mspace->lower_bound + length <= vma->node.value_) {
        return mspace->lower_bound;
    }

    for (;;) {
        next = bbtree_next(&vma->node, vma_t, node);
        if (next == NULL ||
                vma->node.value_ + vma->length + length <= next->node.value_) {
            break;
        }
        vma = next;
    }
    if (next == NULL &&
            vma->node.value_ + vma->length + length > mspace->upper_bound) {
        return 0;
    }
    return vma->node.value_ + vma->length;
}

/** Split one VMA into two.
 *
 * @mspace  The address space concerned
 * @area  The already existing VMA that need to be splited.
 * @length  The length of the new VMA
 *
 * The VMA is cut after the length provided. A new VMA is add that will map
 * the rest of the VMA serving the exact same data.
 */
static vma_t *mspace_split_vma(mspace_t *mspace, vma_t *area, size_t length)
{
    assert(splock_locked(&mspace->lock));
    assert(area->length > length);
    kprintf(KLOG_MEM, "[MEM ] Split [%08x-%08x] at %x.\n", area->node.value_,
            area->node.value_ + area->length, area->node.value_ + length);

    vma_t *vma = (vma_t *)kalloc(sizeof(vma_t));
    vma->mspace = mspace;
    vma->node.value_ = area->node.value_ + length;
    vma->length = area->length - length;
    vma->ino = area->ino ? vfs_open(area->ino) : NULL;
    vma->offset = area->offset + length;
    vma->flags = area->flags;

    area->length = length;
    if (area->limit != 0) {
        if (area->limit > (off_t)length) {
            vma->limit = area->limit - length;
            area->limit = 0;
        } else {
            vma->limit = -1;
        }
    }
    bbtree_insert(&mspace->tree, &vma->node);
    return vma;
}

/** Check and transfrom the flags before assign to a new VMA. */
static int mspace_set_flags(int flags)
{
    flags |= (flags & VMA_RIGHTS) << 4;  /* Copy rights flags as capabilties */
    if (flags & VMA_COPY_ON_WRITE) {
        flags &= ~VMA_WRITE;
    }
    return flags & 0x0FFF;
}

/** Close a VMA and release private pages.
 *
 * WARNING: Possible memory leak or other error in case page are not
 * properly release.
 */
static int mspace_close_vma(mspace_t *mspace, vma_t *vma, int arg)
{
    (void)arg;
    int type = vma->flags & VMA_TYPE;
    if ((vma->flags & VMA_SHARED) == 0 && type != VMA_PHYS && type != VMA_FILE) {
        page_sweep(vma->node.value_, vma->length, true);
    }
    if (vma->ino) {
        vfs_close(vma->ino);
    }
    bbtree_remove(&mspace->tree, vma->node.value_);
    kprintf(KLOG_MEM, "[MEM ] Free [%08x-%08x] \n", vma->node.value_,
            vma->node.value_ + vma->length);
    mspace->v_size -= vma->length;
    kfree(vma);
    return 0;
}

/** Change the flags of a VMA. */
static int mspace_protect_vma(mspace_t *mspace, vma_t* vma, int flags)
{
    (void)mspace;
    if (flags & VMA_RIGHTS) {
        // Change access right
        if ((flags & ((vma->flags >> 4) & VMA_RIGHTS)) != flags) {
            errno = EPERM;
            return -1;
            // Un-autorized!
        }

        vma->flags &= ~VMA_RIGHTS;
        vma->flags |= flags;
    } else if (flags == VMA_DEAD) {
        vma->flags = flags;
    }
    return 0;
}

/** Utility method used to apply a change on a address interval.
 *
 * This method browse VMA on an interval and split VMA partially in or out
 * the interval. Note that every addresses on the interval need to be
 * accessible or the function will stoy mid-way and return an error.
 */
static int mspace_interval(mspace_t *mspace, size_t address, size_t length, int(*action)(mspace_t *, vma_t*, int), int arg)
{
    vma_t *vma;
    assert(address >= mspace->lower_bound && address + length <= mspace->upper_bound);
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
        if (vma == NULL || vma->flags & VMA_DEAD) {
            errno = ENOENT;
            splock_unlock(&mspace->lock);
            return -1;
        }

        if (vma->node.value_ != address) {
            assert(vma->node.value_ < address);
            vma = mspace_split_vma(mspace, vma, address - vma->node.value_);
        }

        assert(vma->node.value_ == address);
        if (vma->length > length) {
            mspace_split_vma(mspace, vma, length);
        }

        address += vma->length;
        length -= vma->length;
        if (action(mspace, vma, arg)) {
            return -1;
        }
    }

    splock_unlock(&mspace->lock);
    errno = 0;
    return 0;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

/* Map a memory area inside the provided address space. */
void *mspace_map(mspace_t *mspace, size_t address, size_t length,
                 inode_t *ino, off_t offset, off_t limit, int flags)
{
    vma_t *vma;
    assert((address & (PAGE_SIZE - 1)) == 0);
    assert((length & (PAGE_SIZE - 1)) == 0);

    int vflags = mspace_set_flags(flags);
    if (length == 0 || length > VMA_CFG_MAXSIZE || vflags == 0) {
        errno = EINVAL;
        return NULL;
    }

    // kprintf(KLOG_DBG, "mspace_t { treecount:%d, dir:%x, lower:%x, upper:%x, lock:%d } \n",
    //   mspace->tree.count_, mspace->directory, mspace->lower_bound, mspace->upper_bound, mspace->lock);

    splock_lock(&mspace->lock);

    // If we have an address, check availability
    if (address != 0) {
        if (!mspace_is_available(mspace, address, length)) {
            if (flags & VMA_MAP_FIXED) {
                splock_unlock(&mspace->lock);
                errno = ERANGE;
                return NULL;
            }
            address = 0;
        }
    }

    // If we don't have address yet
    if (address == 0) {
        address = mspace_find_free(mspace, length);
        if (address == 0) {
            splock_unlock(&mspace->lock);
            errno = ENOMEM;
            return NULL;
        }
    }

    // Create the VMA
    errno = 0;
    vma = (vma_t *)kalloc(sizeof(vma_t));
    vma->mspace = mspace;
    vma->node.value_ = address;
    vma->length = length;
    vma->ino = vfs_open(ino);
    vma->offset = offset;
    vma->limit = limit;
    vma->flags = vflags;
    bbtree_insert(&mspace->tree, &vma->node);
    splock_unlock(&mspace->lock);
    mspace->v_size += length;

    static char *rights[] = { "---", "--x", "-w-", "-wx", "r--", "r-x", "rw-", "rwx"};
    char sh = vflags & VMA_COPY_ON_WRITE ? (vflags & VMA_SHARED ? 'W' : 'w') : (vflags & VMA_SHARED ? 'S' : 'p');
    // TODO Add inode info
    if (mspace == &kernel_space)
        kprintf(KLOG_MEM, " - Krn :: "FPTR"-"FPTR" %s%c {%x}\n", address, address + length, rights[vflags & 7], sh, vflags);
    else
        kprintf(KLOG_MEM, " - Usr :: "FPTR"-"FPTR" %s%c {%x}\n", address, address + length, rights[vflags & 7], sh, vflags);

    return (void *)address;
}

/* Change the flags of a memory area. */
int mspace_protect(mspace_t *mspace, size_t address, size_t length, int flags)
{
    if ((flags & VMA_RIGHTS) != flags && flags != VMA_DEAD) {
        errno = EINVAL;
        return -1;
    }

    return mspace_interval(mspace, address, length, mspace_protect_vma, flags);
}

int mspace_unmap(mspace_t *mspace, size_t address, size_t length)
{
    return mspace_interval(mspace, address, length, mspace_close_vma, 0);
}

/* Remove disabled memory area */
int mspace_scavenge(mspace_t *mspace)
{
    vma_t *vma = bbtree_first(&mspace->tree, vma_t, node);
    while (vma) {
        vma_t *next = bbtree_next(&vma->node, vma_t, node);
        if (vma->flags & VMA_DEAD) {
            mspace_close_vma(mspace, vma, 0);
        }
        vma = next;
    }
    return -1;
}

/* Display the state of the current address space */
void mspace_display(mspace_t *mspace)
{
    const char *rights[] = {
        "---p", "--xp", "-w-p", "-wxp",
        "r--p", "r-xp", "rw-p", "rwxp",
        "--- ", "--x ", "-w- ", "-wx ",
        "r-- ", "r-x ", "rw- ", "rwx ",
    };
    splock_lock(&mspace->lock);
    kprintf(KLOG_DBG, "%p-%p mapped: %d KB   writable/private: %d KB   shared: %d KB\n",
            mspace->lower_bound, mspace->upper_bound,
            mspace->v_size / 1024, mspace->p_size / 1024, mspace->s_size / 1024);
    kprintf(KLOG_DBG, "------------------------------------------------\n");
    vma_t *vma = bbtree_first(&mspace->tree, vma_t, node);
    while (vma) {
        kprintf(KLOG_DBG, "%p-%p %s %08x ",
                vma->node.value_, vma->node.value_ + vma->length,
                rights[vma->flags & 15], vma->offset);
        kprintf(KLOG_DBG, "\n");
        vma = bbtree_next(&vma->node, vma_t, node);
    }
    splock_unlock(&mspace->lock);
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

mspace_t *mspace_rcu(mspace_t *mspace, int usage)
{
    if (usage == 1)
        atomic_inc(&mspace->users);
    else {
        if (atomic_fetch_add(&mspace->users, -1) == 1) {
            // WE CAN DESALLOCATE THIS ONE !
            mspace_sweep(mspace);
            kfree(mspace);
        }
    }
    return mspace;
}

mspace_t *mspace_clone(mspace_t *model)
{
    mspace_t *mspace = (mspace_t *)kalloc(sizeof(mspace_t));
    bbtree_init(&mspace->tree);
    splock_init(&mspace->lock);
    atomic_inc(&mspace->users);
    splock_lock(&model->lock);
    mspace->lower_bound = model->lower_bound;
    mspace->upper_bound = model->upper_bound;
    // TODO Copy all VMA, put both on read on write !
    splock_unlock(&model->lock);
    return mspace;
}



/* Release all VMA and free all mspace data structure.
 *
 * WARNING: VMA might contains pages that need to be released first.
 */
void mspace_sweep(mspace_t *mspace)
{
    splock_lock(&mspace->lock);
    vma_t *vma = bbtree_first(&mspace->tree, vma_t, node);
    while (vma != NULL) {
        mspace_close_vma(mspace, vma, 0);
        vma = bbtree_first(&mspace->tree, vma_t, node);
    }

    mmu_destroy_uspace(mspace);
    splock_unlock(&mspace->lock);
}

/* Search a VMA structure at a specific address.
 *
 * @mspace  The instance of the address space object
 * @address  An address included in the VMA we are looking for.
 *
 * Return a VMA structure.
 */
vma_t *mspace_search_vma(mspace_t *kspace, mspace_t *mspace, size_t address)
{
    vma_t *vma;
    if (address >= kspace->lower_bound && address < kspace->upper_bound) {
        splock_lock(&kspace->lock);
        vma = bbtree_search_le(&kspace->tree, address, vma_t, node);
        splock_unlock(&kspace->lock);
    } else if (mspace != NULL && address >= mspace->lower_bound &&
               address < mspace->upper_bound) {
        splock_lock(&mspace->lock);
        vma = bbtree_search_le(&mspace->tree, address, vma_t, node);
        splock_unlock(&mspace->lock);
    } else {
        kprintf(KLOG_MEM, "[MEM ] Page error outside of address spaces [%d].\n", address);
        return NULL;
    }

    if (vma == NULL || vma->node.value_ + vma->length <= address) {
        kprintf(KLOG_MEM, "[MEM ] Page error without associated VMA [%d].\n", address);
        return NULL;
    }
    return vma;
}
