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
#include <kernel/sys/vma.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <check.h>

START_TEST(test_pages_001)
{
  page_initialize();
  mspace_t *mspace = memory_userspace();

  void *vaddress = kmap(1 * _Mib_, NULL, 0xFE000, VMA_PHYS);
  ck_assert(vaddress != NULL && errno == 0);

  page_fault(mspace, (size_t)vaddress, VMA_PF_NO_PAGE);
  page_t paddress = mmu_drop((size_t)vaddress, false);
  ck_assert(paddress == 0xFE000);

  memory_sweep(mspace);

} END_TEST

void fixture_pages(Suite *s)
{
  TCase *tc;

  tc = tcase_create("Pagination");
  tcase_add_test(tc, test_pages_001);
  suite_add_tcase(s, tc);
}
