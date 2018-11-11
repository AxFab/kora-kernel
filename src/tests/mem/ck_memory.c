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
#include <kora/mcrs.h>
#include <kernel/memory.h>
#include <kernel/vfs.h>
#include "../check.h"

extern size_t __um_mspace_pages_count;
extern size_t __um_pages_available;

page_t test_mem_fetch(inode_t *ino, off_t off) {
    return 1 * PAGE_SIZE;
}

void test_mem_sync(inode_t *ino, off_t off, page_t pg) {
}

void test_mem_release(inode_t *ino, off_t off, page_t pg) {
}

ino_ops_t __test_blk_ops = {
    .fetch = test_mem_fetch,
    .sync = test_mem_sync,
    .release = test_mem_release,
};

/* Various dummy tests with kernel space only */
START_TEST(test_01)
{
    inode_t *ino = vfs_inode(1, FL_BLK, NULL);
    ino->ops = &__test_blk_ops;
    __um_mspace_pages_count = 32; // Each memory space will be 32 pages
    __um_pages_available = 64; // We have a total of 64 pages available
    memory_initialize();
    ck_assert(kMMU.pages_amount == __um_pages_available);
    ck_assert(kMMU.free_pages < __um_pages_available);
    irq_reset(true);

    // Map RW arena
    void *ad0 = mspace_map(kMMU.kspace, 0, 4 * PAGE_SIZE, NULL, 0, 0, VMA_HEAP_RW);
    ck_assert(ad0 != NULL && errno == 0);
    ck_assert(mmu_read((size_t)ad0) == 0);

    // Map RO arena

    void *ad1 = mspace_map(kMMU.kspace, 0, 2 * PAGE_SIZE, ino, PAGE_SIZE, 0,
                           VMA_FILE_RO);
    ck_assert(ad1 != NULL && errno == 0);
    ck_assert(mmu_read((size_t)ad1) == 0);

    // Page fault
    page_fault(NULL, (size_t)ad0, PGFLT_MISSING);
    ck_assert(mmu_read((size_t)ad0) != 0);

    void *ad2 = mspace_map(kMMU.kspace, 0, 1 * PAGE_SIZE, NULL, 0, 0, VMA_STACK_RW);
    ck_assert(ad2 != NULL && errno == 0);
    ck_assert(mmu_read((size_t)ad2) == 0);

    // Map physical memory, and resolve alls pages
    size_t avail = kMMU.free_pages;
    void *ad3 = mspace_map(kMMU.kspace, 0, 8 * PAGE_SIZE, NULL, 0xFC00000, 0,
                           VMA_PHYSIQ);
    ck_assert(ad3 != NULL && errno == 0);
    ck_assert(mmu_read((size_t)ad3) != 0);
    ck_assert(avail == kMMU.free_pages); // Only true as we don't have tables

    // Map a file RW but private
    void *ad4 = mspace_map(kMMU.kspace, 0, 2 * PAGE_SIZE, ino, 0, 0, VMA_FILE_WP);
    ck_assert(ad4 != NULL && errno == 0);
    ck_assert(mmu_read((size_t)ad4) == 0);

    // Page fault that will cause a file read
    page_fault(NULL, (size_t)ad4, PGFLT_MISSING);
    // Page fault that will cause a copy-on-write
    // page_fault(NULL, (size_t)ad4, PGFLT_WRITE_DENY);

    mspace_unmap(kMMU.kspace, (size_t)ad1, PAGE_SIZE);
    page_fault(NULL, (size_t)ad1 + PAGE_SIZE, PGFLT_MISSING);
    ck_assert(mmu_read((size_t)ad1 + PAGE_SIZE) != 0);
    mspace_unmap(kMMU.kspace, (size_t)ad1 + PAGE_SIZE, PAGE_SIZE);

    ad1 = mspace_map(kMMU.kspace, (size_t)ad1, PAGE_SIZE, NULL, 0, 0,
                     VMA_ANON | VMA_RX);
    ck_assert(ad1 != NULL && errno == 0);


    int ret = mspace_protect(kMMU.kspace, (size_t)ad2, PAGE_SIZE, VMA_RX);
    ck_assert(ret == -1 && errno == EPERM);
    ret = mspace_protect(kMMU.kspace, (size_t)ad2, PAGE_SIZE, VMA_RO & 7);
    ck_assert(ret == 0 && errno == 0);

    mspace_display(kMMU.kspace);
    kprintf(-1, "\n");
    memory_info();

    memory_sweep();
    ck_assert(kMMU.free_pages == __um_pages_available);
    vfs_close(ino);
}
END_TEST

/* Dummy test with several user spaces */
START_TEST(test_02)
{
    __um_mspace_pages_count = 32; // Each memory space will be 32 pages
    __um_pages_available = 64; // We have a total of 64 pages available
    memory_initialize();
    ck_assert(kMMU.pages_amount == __um_pages_available);
    ck_assert(kMMU.free_pages < __um_pages_available);
    irq_reset(true);

    mspace_t *m1 = mspace_create();
    mspace_t *m2 = mspace_open(m1);
    mmu_context(m1);



    mspace_map(m1, 0x10000000 + 6 * PAGE_SIZE, PAGE_SIZE, NULL, 0, 0, VMA_ANON_RW);


    mspace_display(kMMU.kspace);
    kprintf(-1, "\n");
    memory_info();

    mspace_t *m3 = mspace_clone(m1);


    mspace_close(m3);
    mspace_close(m2);
    mspace_close(m1);

    memory_sweep();
    ck_assert(kMMU.free_pages == __um_pages_available);
}
END_TEST

/* Copy-on-write advanced tests */
START_TEST(test_03)
{
    // kprintf (-1, "\n\e[94m  MEM #1 - <<<>>>\e[0m\n");
}
END_TEST

void fixture_mspace(Suite *s)
{
    TCase *tc = tcase_create("Address space");
    tcase_add_test(tc, test_01);
    tcase_add_test(tc, test_02);
    tcase_add_test(tc, test_03);
    suite_add_tcase(s, tc);
}
