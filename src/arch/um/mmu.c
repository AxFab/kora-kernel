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
#include <kernel/memory.h>
#include <kernel/core.h>
#include <kernel/sys/vma.h>
#include <skc/mcrs.h>
#include <assert.h>
#include <errno.h>
#include <string.h>



// UserSpace have 512 first Mb (-1) (131072 Pages - Table of 512 Kb)
// KernelSpace have 16 Mb after 1 Gb (4096 Pages - Table of 16 Kb)

uint8_t mmu_bmp[MMU_LG] = { 0 };

uint32_t ktable[(16 * _Kib_) / 4] = { 0 };
uint32_t *usertable;

/* - */
void mmu_detect_ram()
{
  page_range(0, 64 * _Kib_);
  page_range(2000 * _Kib_, 48 * _Kib_);
}

/* - */
int mmu_resolve(size_t vaddress, page_t paddress, int access, bool clean)
{
  uint32_t *dir;
  int page_no;
  if (vaddress >= kMMU.kspace->lower_bound && vaddress < kMMU.kspace->upper_bound) {
    dir = ktable;
    page_no = (vaddress - kMMU.kspace->lower_bound) / PAGE_SIZE;
  } else if (vaddress >= kMMU.uspace_lower_bound && vaddress < kMMU.uspace_upper_bound) {
    usertable = ktable;
    page_no = (vaddress - kMMU.uspace_lower_bound) / PAGE_SIZE;
  }

  assert(dir[page_no] == 0);
  dir[page_no] = paddress | (access & 7);
  return 0;
}

/* - */
page_t mmu_drop(size_t vaddress, bool clean)
{
  uint32_t *dir;
  int page_no;
  if (vaddress >= kMMU.kspace->lower_bound && vaddress < kMMU.kspace->upper_bound) {
    dir = ktable;
    page_no = (vaddress - kMMU.kspace->lower_bound) / PAGE_SIZE;
  } else if (vaddress >= kMMU.uspace_lower_bound && vaddress < kMMU.uspace_upper_bound) {
    dir = usertable;
    page_no = (vaddress - kMMU.uspace_lower_bound) / PAGE_SIZE;
  }

  page_t pg = dir[page_no] & ~(PAGE_SIZE - 1);
  dir[page_no] = 0;
  return pg;
}

page_t mmu_directory()
{
  usertable = kalloc(512 * _Kib_);
  page_t dir_page = (page_t)usertable;
  return dir_page;
}

void mmu_release_dir(page_t dir)
{
  uint32_t *table = (uint32_t*)dir;
  kfree(table);
}
