/*
 *      This file is part of the KoraOs project.
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
#include <kora/mcrs.h>
#include <assert.h>
#include <errno.h>

static bool mspace_is_available(mspace_t *mspace, size_t address,
                                     size_t length)
{
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

static size_t mspace_find_free(mspace_t *mspace, size_t length)
{
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

static vma_t *mspace_split_vma(mspace_t *mspace, vma_t *area, size_t length)
{
    assert(area->length > length);
    kprintf(-1, "[MEM ] Split [%08x-%08x] at %x.\n", area->node.value_,
            area->node.value_ + area->length, area->node.value_ + length);

    vma_t *vma = (vma_t *)kalloc(sizeof(vma_t));
    vma->node.value_ = area->node.value_ + length;
    vma->length = area->length - length;
    vma->ino = area->ino;
    vma->offset = area->offset + length;
    vma->flags = area->flags;

    area->length = length;
    bbtree_insert(&mspace->tree, &vma->node);
    return vma;
}

static void mspace_close_vma(mspace_t *mspace, vma_t *vma)
{
    if ((vma->flags & VMA_SHARED) == 0) {
        page_sweep(vma->node.value_, vma->length, true);
    }
    bbtree_remove(&mspace->tree, vma->node.value_);
    kprintf(-1, "[MEM ] Free [%08x-%08x] \n", vma->node.value_,
            vma->node.value_ + vma->length);
    mspace->v_size -= vma->length;
    kfree(vma);
}

static int mspace_set_flags(int flags)
{
    flags |= (flags & VMA_RIGHTS) << 4;  /* Copy rights flags as capabilties */
    if (flags & VMA_COPY_ON_WRITE) {
        flags &= ~VMA_WRITE;
    }
    return flags & 0x0FFF;
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

    // kprintf(-1, "mspace_t { treecount:%d, dir:%x, lower:%x, upper:%x, lock:%d } \n",
    //   mspace->tree.count_, mspace->directory, mspace->lower_bound, mspace->upper_bound, mspace->lock);

    splock_lock(&mspace->lock);

    // If we have an address, check availability
    if (address != 0) {
        if (mspace_is_available(mspace, address, length)) {
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
    vma->node.value_ = address;
    vma->length = length;
    vma->ino = ino;
    vma->offset = offset;
    vma->limit = limit;
    vma->flags = vflags;
    bbtree_insert(&mspace->tree, &vma->node);
    splock_unlock(&mspace->lock);
    mspace->v_size += length;
    kprintf(-1, "[MEM ] Mapping [%08x-%08x] (%x, %x)\n", address,
            address + length, ino, offset);
    return (void *)address;
}

/* Change the flags of a memory area. */
int mspace_protect(mspace_t *mspace, size_t address, size_t length, int flags)
{
    vma_t *vma;
    assert((address & (PAGE_SIZE - 1)) == 0);
    assert((length & (PAGE_SIZE - 1)) == 0);
    if (address == 0 || length == 0 || length > VMA_CFG_MAXSIZE) {
        errno = EINVAL;
        return -1;
    }

    if ((flags & VMA_RIGHTS) != flags && flags != VMA_DEAD) {
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
            vma = mspace_split_vma(mspace, vma, address - vma->node.value_);
        }

        assert(vma->node.value_ == address);
        if (vma->length > length) {
            mspace_split_vma(mspace, vma, length);
        }

        if (vma->flags & VMA_DEAD) {
            splock_unlock(&mspace->lock);
            errno = ENOENT;
            return -1;
        }

        if (flags & VMA_RIGHTS) {
            // Change access right
            if ((flags & ((vma->flags & VMA_RIGHTS) >> 4)) != flags) {
                // Un-autorized!
            }

            vma->flags &= ~VMA_RIGHTS;
            vma->flags |= flags;
        } else if (flags == VMA_DEAD) {
            vma->flags |= flags;
        }

        address += vma->length;
        length -= vma->length;
    }

    splock_unlock(&mspace->lock);
    errno = 0;
    return 0;
}

/* Remove disabled memory area */
int mspace_scavenge(mspace_t *mspace)
{
    vma_t *vma = bbtree_first(&mspace->tree, vma_t, node);
    while (vma) {
        vma_t *next = bbtree_next(&vma->node, vma_t, node);
        if (vma->flags & VMA_DEAD) {
            mspace_close_vma(mspace, vma);
        }
        vma = next;
    }
    return -1;
}

/* Display the state of the current address space */
void mspace_display(int log, mspace_t *mspace)
{
    // 0] 00000000 - 00000000   (15.2Mb)   Sr-x  000000
    // 00000000-00000000 r-xp 00000000 0     /name
    // mapped: 1340 KB  writable/private: 40 KB    shared: 0KB
    const char *rights[] = {
        " ---", " --x", " -w-", " -wx",
        " r--", " r-x", " rw-", " rwx",
        "S---", "S--x", "S-w-", "S-wx",
        "Sr--", "Sr-x", "Srw-", "Srwx",
    };
    int idx;
    splock_lock(&mspace->lock);
    vma_t *vma = bbtree_first(&mspace->tree, vma_t, node);
    for (idx = 0; vma; ++idx) {
        kprintf(log, "%d] %08x - %08x   %s   %s  %6x\n", idx,
                vma->node.value_, vma->node.value_ + vma->length,
                sztoa(vma->length), rights[vma->flags & 15], vma->flags);
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
    mspace->lower_bound = kMMU.uspace_lower_bound;
    mspace->upper_bound = kMMU.uspace_upper_bound;
    mspace->directory = mmu_directory();
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
        mspace_close_vma(mspace, vma);
        vma = bbtree_first(&mspace->tree, vma_t, node);
    }

    mmu_release_dir(mspace->directory);
    splock_unlock(&mspace->lock);
    kfree(mspace);
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
    vma_t *vma;
    if (address >= kMMU.kheap_lower_bound && address < kMMU.kheap_upper_bound) {
        vma = bbtree_search_le(&kMMU.kspace->tree, address, vma_t, node);
    } else if (mspace != NULL && address >= mspace->lower_bound &&
               address < mspace->upper_bound) {
        vma = bbtree_search_le(&mspace->tree, address, vma_t, node);
    } else {
        kprintf(-1, "[MEM ] Page error outside of address spaces [%d].\n", address);
        return NULL;
    }

    if (vma == NULL || vma->node.value_ + vma->length <= address) {
        kprintf(-1, "[MEM ] Page error without associated VMA [%d].\n", address);
        return NULL;
    }
    return vma;
}
