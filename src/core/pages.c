/*
 *      This file is part of the SmokeOS project.
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
#include <kernel/sys/vma.h>
#include <skc/mcrs.h>
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
  while (i < MMU_LG && MMU_BMP[i] == 0xff) ++i;
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
    page_t pg = mmu_drop(address, clean);
    if (pg != 0) {
      page_release(pg);
    }
    address += PAGE_SIZE;
    length -= PAGE_SIZE;
  }
}

/* Resolve a page fault */
int page_fault(mspace_t *mspace, size_t address, int reason)
{
  vma_t *vma;
  kprintf(-1, "Page fault at: %x (%d) - ", address, reason);
  address = ALIGN_DW(address, PAGE_SIZE);
  if (address >= kMMU.kheap_lower_bound && address < kMMU.kheap_upper_bound) {
    vma = bbtree_search_le(&kMMU.kspace->tree, address, vma_t, node);
  } else if (mspace != NULL && address >= kMMU.uspace_lower_bound && address < kMMU.uspace_upper_bound) {
    vma = bbtree_search_le(&mspace->tree, address, vma_t, node);
  } else {
    kprintf(-1, "Outside of address spaces.\n");
    return -1;
  }

  if (vma == NULL || vma->node.value_ + vma->length <= address) {
    kprintf(-1, "No associated VMA.\n");
    return -1;
  }

  bool zeroed = false;
  page_t page = 0;
  // if (reason == VMA_PF_NO_PAGE) {
    switch (vma->flags & VMA_TYPE) {
      case VMA_HEAP:
      case VMA_STACK:
      case VMA_ANON:
        kprintf(-1, "EMPTY.\n");
        zeroed = true;
        page = page_new();
        break;
      case VMA_FILE:
        // page = vma->ino->map_page(vma->ino, vma->offset + vma->node.value_ - address);
        kprintf(-1, "FILE %x.\n", page);
        break;
      case VMA_PHYS:
        page = vma->offset + vma->node.value_ - address;
        kprintf(-1, "PHYS %x.\n", page);
        break;
    }

    assert(page != 0);
    return mmu_resolve(address, page, vma->flags, zeroed);
  // } else if (reason == VMA_PF_WRITE) {
  //   if (vma->flags & VMA_CAN_WRITE) {
  //     return -1;
  //   }

  //   // TODO Copy-on-write

  // } else {
  //   // TODO Disk-Swap
  // }
}

