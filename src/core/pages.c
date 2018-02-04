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
#include <kernel/cpu.h>
#include <kernel/asm/vma.h>
#include <kernel/sys/inode.h>
#include <kora/mcrs.h>
#include <assert.h>
#include <errno.h>
#include <string.h>


mspace_t kernel_space;

struct kMmu kMMU = {
    .upper_physical_page = 0,
    .pages_amount = 0,
    .free_pages = 0,
    .page_size = PAGE_SIZE,
    .uspace_lower_bound = MMU_USPACE_LOWER,
    .uspace_upper_bound = MMU_USPACE_UPPER,
    .kheap_lower_bound = MMU_KSPACE_LOWER,
    .kheap_upper_bound = MMU_KSPACE_UPPER,
};

void page_initialize()
{
    memset(MMU_BMP, 0xff, MMU_LG);
    kMMU.upper_physical_page = 0;
    kMMU.pages_amount = 0;
    kMMU.free_pages = 0;

    mmu_detect_ram();
    cpu_enable_mmu();

    // Init Kernel memory space structure
    kernel_space.lower_bound = kMMU.kheap_lower_bound;
    kernel_space.upper_bound = kMMU.kheap_upper_bound;
    bbtree_init(&kernel_space.tree);
    splock_init(&kernel_space.lock);
    kMMU.kspace = &kernel_space;

    kprintf (-1, "[MEM ] %s available over ",
             sztoa((uintmax_t)kMMU.pages_amount * PAGE_SIZE));
    kprintf (-1, "%s\n", sztoa((uintmax_t)kMMU.upper_physical_page * PAGE_SIZE));
}

void page_range(long long base, long long length)
{
    long long obase = base;
    assert(base >= 0 && length > 0);
    base = ALIGN_UP(base, PAGE_SIZE);
    length = ALIGN_DW(length - (base - obase), PAGE_SIZE);
    int count = length / PAGE_SIZE;
    int start = base / PAGE_SIZE;

    if (kMMU.upper_physical_page < (unsigned)start + count) {
        kMMU.upper_physical_page = (unsigned)start + count;
    }

    kMMU.pages_amount += count;
    kMMU.free_pages += count;
    int i = start / 8;
    int j = start % 8;
    while (count > 0 && j != 0) {
        MMU_BMP[i] = MMU_BMP[i] & ~(1 << j);
        j = (j + 1) % 8;
        count--;
    }
    start++;
    memset(&MMU_BMP[start / 8], 0, count / 8);
    start += count / 8;
    count = count % 8;
    i = start / 8;
    j = 0;
    while (count > 0) {
        MMU_BMP[i] = MMU_BMP[i] & ~(1 << j);
        j++;
        count--;
    }
}

/* Allocat a single page for the system and return it's physical address */
page_t page_new ()
{
    int i = 0, j = 0;
    while (i < MMU_LG && MMU_BMP[i] == 0xff) {
        ++i;
    }
    if (i == MMU_LG) {
        kpanic("No page available");
    }
    uint8_t page_byte = MMU_BMP[i];
    while (page_byte & 1) {
        ++j;
        page_byte = page_byte >> 1;
    }
    MMU_BMP[i] = MMU_BMP[i] | (1 << j);
    kMMU.free_pages--;
    return (page_t)(i * 8 + j) * PAGE_SIZE;
}

/* Mark a physique page, returned by `page_new`, as available again */
void page_release(page_t paddress)
{
    int i = (paddress / PAGE_SIZE) / 8;
    int j = (paddress / PAGE_SIZE) % 8;
    assert(i < MMU_LG && (MMU_BMP[i] & (1 << j)) != 0);
    kMMU.free_pages++;
    MMU_BMP[i] = MMU_BMP[i] & ~(1 << j);
}

/* Free all used pages into a range of virtual addresses */
void page_sweep(size_t address, size_t length, bool clean)
{
    assert((address & (PAGE_SIZE - 1)) == 0);
    assert((length & (PAGE_SIZE - 1)) == 0);
    while (length) {
        page_t pg = mmu_read(address, true, clean);
        if (pg != 0) {
            page_release(pg);
        }
        address += PAGE_SIZE;
        length -= PAGE_SIZE;
    }
}




static int page_copy_vma(vma_t *vma)
{
    // TODO - lock VMA or something!?
    vma->flags &= ~VMA_COPY_ON_WRITE;
    vma->flags |= VMA_WRITE;
    size_t length = vma->length;
    size_t base = vma->node.value_;
    void *buf = kmap(length, NULL, 0, VMA_FG_ANON);
    memcpy(buf, (void *)base, PAGE_SIZE);
    size_t cpy_map = (size_t)buf;
    while (length > 0) {
        mmu_read(base, true, false);
        page_t copy_page = mmu_read(cpy_map, false, false);
        mmu_resolve(base, copy_page, vma->flags, false);
        base += PAGE_SIZE;
        cpy_map += PAGE_SIZE;
        length -= PAGE_SIZE;
    }
    kunmap(buf, vma->length);
    return 0;
}

/* Resolve a page fault */
int page_fault(mspace_t *mspace, size_t address, int reason)
{
    address = ALIGN_DW(address, PAGE_SIZE);
    vma_t *vma = mspace_search_vma(mspace, address);
    if (vma == NULL) {
        return -1;
    }

    int type = vma->flags & VMA_TYPE;
    off_t offset = vma->offset + (address - vma->node.value_);
    if (reason == PGFLT_MISSING && type == VMA_PHYS) {
        /* Resolve missing page fault - physical address mapping. */
        return mmu_resolve(address, (page_t)offset, vma->flags, false);

    } else if (reason == PGFLT_MISSING && type == VMA_FILE) {
        /* Missing file read - TODO vfs_read should be called using INTERUPT */
        page_t page = vfs_page(vma->ino, offset);
        if (page != 0) {
            return mmu_resolve(address, page, vma->flags, false);
        }
        if (mmu_resolve(address, 0, vma->flags, false) != 0) {
            return -1;
        }
        // TODO - Call only if we have a new page !?
        vfs_read(vma->ino, (void *)ALIGN_DW(address, PAGE_SIZE), PAGE_SIZE, offset);
        return 0;

    } else if (reason == PGFLT_MISSING) {
        /* Missing a blank page. */
        if (type != VMA_HEAP && type != VMA_STACK && type != VMA_ANON) {
            kprintf(-1, "[MEM ] Unqualified VMA type at 0x%08x [%x].\n", address,
                    vma->flags);
        }
        return mmu_resolve(address, 0, vma->flags, true);

    } else if (reason == PGFLT_WRITE_DENY && type == VMA_FILE) {
        if (!(vma->flags & VMA_CAN_WRITE) || !(vma->flags & VMA_COPY_ON_WRITE)) {
            return -1;
        }
        return page_copy_vma(vma);

    } else {
        kprintf(-1, "[MEM ] Unresolvable page fault 0x%08x (%d) [%x].\n", address,
                reason, vma->flags);
        return -1;
    }

    return -1;
}

int page_resolve(mspace_t *mspace, size_t address, size_t length)
{
    vma_t *vma = mspace_search_vma(mspace, address);
    if (vma == NULL) {
        return -1;
    } else if (vma->node.value_ != address || vma->length != length) {
        return -1;
    }

    page_t page = 0;
    off_t offset = vma->offset + (address - vma->node.value_);
    if ((vma->flags & VMA_TYPE) == VMA_PHYS) {
        page = offset;
        kprintf(-1, "[MEM ] Page resolve for physical pages at [%x] (%s).\n", address,
                sztoa(length));
        while (length > 0) {
            mmu_resolve(address, page, vma->flags, false);
            page += PAGE_SIZE;
            address += PAGE_SIZE;
            length -= PAGE_SIZE;
        }
    } else if ((vma->flags & VMA_TYPE) == VMA_HEAP ||
               (vma->flags & VMA_TYPE) == VMA_STACK ||
               (vma->flags & VMA_TYPE) == VMA_ANON) {
        kprintf(-1, "[MEM ] Page resolve for blank pages at [%x] (%s).\n", address,
                sztoa(length));
        while (length > 0) {
            mmu_resolve(address, page_new(), vma->flags, true);
            address += PAGE_SIZE;
            length -= PAGE_SIZE;
        }
    } else {
        kprintf(-1, "[MEM ] Page resolve called with incorrect VMA type.\n");
    }

    return -1;
}