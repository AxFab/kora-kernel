/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2018  <Fabien Bavent>
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
#include <assert.h>
#include <errno.h>
#include <string.h>



void page_range(long long base, long long length)
{
    long long obase = base;
    assert(base >= 0 && length > 0);
    base = ALIGN_UP(base, PAGE_SIZE);
    length = ALIGN_DW(length - (base - obase), PAGE_SIZE);
    int count = length / PAGE_SIZE;
    int start = base / PAGE_SIZE;

    if (kMMU.upper_physical_page < (unsigned)start + count)
        kMMU.upper_physical_page = (unsigned)start + count;

    kMMU.pages_amount += count;
    kMMU.free_pages += count;
    int i = start / 8;
    int j = start % 8;
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
}

/* Allocat a single page for the system and return it's physical address */
page_t page_new()
{
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
}

/* Look for count pages in continous memory */
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
    int i = (paddress / PAGE_SIZE) / 8;
    int j = (paddress / PAGE_SIZE) % 8;
    assert(i < MMU_LG && (MMU_BMP[i] & (1 << j)) != 0);
    kMMU.free_pages++;
    MMU_BMP[i] = MMU_BMP[i] & ~(1 << j);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

/* Free all used pages into a range of virtual addresses */
void page_sweep(mspace_t *mspace, size_t address, size_t length, bool clean)
{
    assert((address & (PAGE_SIZE - 1)) == 0);
    assert((length & (PAGE_SIZE - 1)) == 0);
    while (length) {
        if (clean)
            memset((void *)address, 0, PAGE_SIZE);
        page_t pg = mmu_drop(address);
        if (pg != 0)
            page_release(pg);
        address += PAGE_SIZE;
        length -= PAGE_SIZE;
    }
}

/* Resolve a page fault */
int page_fault(mspace_t *mspace, size_t address, int reason)
{
    int ret = 0;
    vma_t *vma = mspace_search_vma(mspace, address);
    if (vma == NULL) {
        kprintf(KLOG_PF, "\033[91m#PF\033[31m Forbidden address %p!\033[0m\n", address);
        return -1;
    }

    if (reason & PGFLT_WRITE && !(vma->flags & VMA_WRITE)
        && !(vma->flags & VMA_COPY_ON_WRITE)) {
        kprintf(KLOG_PF, "\033[91m#PF\033[31m Forbidden writing at %p!\033[0m\n",
                address);
        splock_unlock(&vma->mspace->lock);
        return -1; // Writing is forbidden
    }
    address = ALIGN_DW(address, PAGE_SIZE);
    if (reason & PGFLT_MISSING) {
        ++kMMU.soft_page_fault;
        ret = vma_resolve(vma, address, PAGE_SIZE);
    }
    if (ret != 0) {
        kprintf(KLOG_PF, "\033[91m#PF\033[31m Unable to resolve page at %p!\033[0m\n",
                address);
        splock_unlock(&vma->mspace->lock);
        return -1;
    }
    if (reason & PGFLT_WRITE && (vma->flags & VMA_COPY_ON_WRITE))
        ret = vma_copy_on_write(vma, address, PAGE_SIZE);
    if (ret != 0)
        kprintf(KLOG_PF, "\033[91m#PF\033[31m Error on copy and write at %p!\033[0m\n",
                address);
    kprintf(KLOG_PF, "\033[91m#PF\033[0m Page at %p!\033[0m\n", address);
    splock_unlock(&vma->mspace->lock);
    return 0;
}
