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
#include <errno.h>
#include <check.h>

extern int mmu_uspace_size;
void mmu_disable();

/* Basic get and release address intervals */
START_TEST(test_memory_001)
{
    page_initialize(); // We can do anything without this!

    mmu_uspace_size = 24; // Set size of mspace
    mspace_t *mspace = mspace_create();

    // We have 24 pages available
    void *a1 = mspace_map(mspace, 0, 4 * PAGE_SIZE, NULL, 0, 0, VMA_RW | VMA_HEAP);
    ck_assert(a1 != NULL && errno == 0);
    void *a2 = mspace_map(mspace, 0, 4 * PAGE_SIZE, NULL, 0, 0, VMA_RW | VMA_HEAP);
    ck_assert(a2 != NULL && errno == 0);
    void *a3 = mspace_map(mspace, 0, 4 * PAGE_SIZE, NULL, 0, 0, VMA_RW | VMA_HEAP);
    ck_assert(a3 != NULL && errno == 0);
    void *a4 = mspace_map(mspace, 0, 4 * PAGE_SIZE, NULL, 0, 0, VMA_RW | VMA_HEAP);
    ck_assert(a4 != NULL && errno == 0);
    void *a5 = mspace_map(mspace, 0, 4 * PAGE_SIZE, NULL, 0, 0, VMA_RW | VMA_HEAP);
    ck_assert(a5 != NULL && errno == 0);
    void *a6 = mspace_map(mspace, 0, 4 * PAGE_SIZE, NULL, 0, 0, VMA_RW | VMA_HEAP);
    ck_assert(a6 != NULL && errno == 0);
    void *a7 = mspace_map(mspace, 0, 4 * PAGE_SIZE, NULL, 0, 0, VMA_RW | VMA_HEAP);
    ck_assert(a7 == NULL && errno == ENOMEM);

    ck_assert(mspace_protect(mspace, (size_t)a2, 4 * PAGE_SIZE, VMA_READ) == 0
              && errno == 0); // Full VMA
    ck_assert(mspace_protect(mspace, (size_t)a3, 2 * PAGE_SIZE, VMA_READ) == 0
              && errno == 0); // Low Half VMA
    ck_assert(mspace_protect(mspace, (size_t)a4 + 2 * PAGE_SIZE, 2 * PAGE_SIZE,
                             VMA_READ) == 0 && errno == 0); // High Half VMA

    ck_assert(mspace_protect(mspace, (size_t)a2 + 3 * PAGE_SIZE, 2 * PAGE_SIZE,
                             VMA_WRITE) == 0 && errno == 0); // 2 Part VMA

    // Unmap
    ck_assert(mspace_unmap(mspace, (size_t)a1, 4 * PAGE_SIZE) == 0 && errno == 0);

    ck_assert(mspace_protect(mspace, (size_t)a2, 12 * PAGE_SIZE, VMA_DEAD) == 0
              && errno == 0);
    mspace_scavenge(mspace);

    mspace_display(0, mspace);  /* Should write on a file... */

    mspace_sweep(mspace);
    mmu_disable();
}
END_TEST

/* Special get address intervals */
START_TEST(test_memory_002)
{
    page_initialize(); // We can do anything without this!

    mmu_uspace_size = 16; // Set size of mspace
    mspace_t *mspace = mspace_create();

    void *a1 = mspace_map(mspace, 0, 2 * PAGE_SIZE, NULL, 0, 0, VMA_RW | VMA_HEAP);
    ck_assert(a1 != NULL && errno == 0);

    void *a2 = mspace_map(mspace, (size_t)a1 + 4 * PAGE_SIZE, 2 * PAGE_SIZE, NULL,
                          0, 0, VMA_RO | VMA_HEAP | VMA_MAP_FIXED);
    ck_assert(a2 != NULL && errno == 0);

    void *a0 = mspace_map(mspace, (size_t)a1 + 4 * PAGE_SIZE, 1 * PAGE_SIZE, NULL,
                          0, 0, VMA_RW | VMA_HEAP | VMA_MAP_FIXED);
    ck_assert(a0 == NULL && errno == ERANGE);

    void *a3 = mspace_map(mspace, (size_t)a1 + 4 * PAGE_SIZE, 6 * PAGE_SIZE, NULL,
                          0, 0, VMA_RW | VMA_HEAP);
    ck_assert(a3 != NULL && errno == 0);

    mspace_display(0, mspace);  /* Should write on a file... */

    mspace_sweep(mspace);
    mmu_disable();
}
END_TEST

/* Special case for protect */
START_TEST(test_memory_003)
{
    page_initialize(); // We can do anything without this!

    mmu_uspace_size = 16; // Set size of mspace
    mspace_t *mspace = mspace_create();



    mspace_display(0, mspace);  /* Should write on a file... */

    mspace_sweep(mspace);
    mmu_disable();
}
END_TEST

/* Test release of pages */
START_TEST(test_memory_004)
{
    page_initialize(); // We can do anything without this!

    mmu_uspace_size = 20; // Set size of mspace
    mspace_t *mspace = mspace_create();

    void *a1 = mspace_map(mspace, 0, 6 * PAGE_SIZE, NULL, 0, 0, VMA_RW | VMA_HEAP);
    ck_assert(a1 != NULL && errno == 0);

    void *a2 = mspace_map(mspace, 0, 6 * PAGE_SIZE, NULL, 0, 0, VMA_RO | VMA_HEAP);
    ck_assert(a2 != NULL && errno == 0);

    void *a3 = mspace_map(mspace, 0, 6 * PAGE_SIZE, NULL, 0, 0xCAFE0000,
                          VMA_RW | VMA_PHYS);
    ck_assert(a3 != NULL && errno == 0);

    int pages0 = kMMU.free_pages;
    page_fault(mspace, (size_t)a1 + 0x134, PGFLT_MISSING);
    ck_assert(kMMU.free_pages != pages0);

    mspace_display(0, mspace);  /* Should write on a file... */

    mspace_sweep(mspace);

    ck_assert(kMMU.free_pages == pages0);

    mmu_disable();
}
END_TEST

void fixture_memory(Suite *s)
{
    TCase *tc = tcase_create("Memory");
    tcase_add_test(tc, test_memory_001);
    tcase_add_test(tc, test_memory_002);
    tcase_add_test(tc, test_memory_003);
    tcase_add_test(tc, test_memory_004);
    suite_add_tcase(s, tc);
}
