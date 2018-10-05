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
#include <kernel/core.h>
#include <kernel/memory.h>
#include <kora/mcrs.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "../check.h"

extern size_t __um_mspace_pages_count;
extern size_t __um_pages_available;


START_TEST(test_pages_001)
{
    __um_mspace_pages_count = 32; // Each memory space will be 32 pages
    __um_pages_available = 32; // We have a total of 64 pages available
    memory_initialize();
    ck_assert(kMMU.pages_amount == __um_pages_available);
    ck_assert(kMMU.free_pages < __um_pages_available);
    irq_reset(true);

    kprintf(KLOG_DBG, "Get page "FPTR"\n", page_new());
    kprintf(KLOG_DBG, "Get page "FPTR"\n", page_new());
    kprintf(KLOG_DBG, "Get 16 pages "FPTR"\n", page_get(1, 16));
    kprintf(KLOG_DBG, "Get page "FPTR"\n", page_new());
    kprintf(KLOG_DBG, "Get page "FPTR"\n", page_new());
    kprintf(KLOG_DBG, "Get page "FPTR"\n", page_new());
    kprintf(KLOG_DBG, "Get page "FPTR"\n", page_new());
    kprintf(KLOG_DBG, "Get page "FPTR"\n", page_new());
    kprintf(KLOG_DBG, "Get page "FPTR"\n", page_new());
    kprintf(KLOG_DBG, "Get page "FPTR"\n", page_new());
    kprintf(KLOG_DBG, "Get page "FPTR"\n", page_new());
    kprintf(KLOG_DBG, "Get page "FPTR"\n", page_new());

    memory_info();

    memory_sweep();
    // ck_assert(kMMU.free_pages == __um_pages_available);
}
END_TEST

START_TEST(test_pages_002)
{
    // page_initialize();
    // mspace_t *mspace = mspace_create();

    // void *vaddress = kmap(1 * _Mib_, NULL, 0xFE000, VMA_PHYS);
    // ck_assert(vaddress != NULL && errno == 0);

    // page_fault(mspace, (size_t)vaddress, PGFLT_MISSING);
    // page_t paddress = mmu_read((size_t)vaddress, false, false);
    // ck_assert(paddress == 0xFE000);

    // mspace_sweep(mspace);

}
END_TEST

void fixture_pages(Suite *s)
{
    TCase *tc;

    tc = tcase_create("Pagination");
    tcase_add_test(tc, test_pages_001);
    suite_add_tcase(s, tc);
}
