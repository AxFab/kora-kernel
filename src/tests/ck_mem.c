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
#include "check.h"

extern size_t __um_mspace_pages_count;
extern size_t __um_pages_available;

struct inode {
    int fd;
    int rcu;
};

inode_t *vfs_inode()
{
    inode_t *ino = (inode_t *)kalloc(sizeof(inode_t));
    ino->rcu = 1;
    return ino;
}
inode_t *vfs_open(inode_t *ino)
{
    if (ino != NULL)
        ino->rcu++;
    return ino;
}

void vfs_close(inode_t *ino)
{
    if (--ino->rcu == 0)
        kfree(ino);
}

/* Find the page mapping the content of a block inode */
page_t ioblk_page(inode_t *ino, off_t off)
{
    (void)ino;
    assert(IS_ALIGNED(off, PAGE_SIZE));
    return 0x4000 + off;
}

int vfs_readlink(inode_t *ino, char *buf, int len, int flags)
{
    buf[0] = 'a';
    buf[1] = '\0';
    return 0;
}

void vfs_read(inode_t *ino, char *buf, size_t len, off_t off)
{

}


/* Various dummy tests with kernel space only */
START_TEST(test_01)
{
    inode_t *ino = 0;// vfs_inode(0, )
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

START_TEST(test_page_01)
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

int main ()
{
    kprintf(-1, "\e[1;97mKora MEM check - " __ARCH " - v" _VTAG_ "\e[0m\n");

    ck_case(test_page_01);
    ck_case(test_01);
    ck_case(test_02);
    ck_end();
    return 0;
}

