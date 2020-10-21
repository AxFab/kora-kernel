#include <kernel/core.h>
#include <kernel/memory.h>
#include <threads.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "check.h"

void futex_init();

void vfs_open_inode(inode_t *ino) {}
void vfs_close_inode(inode_t *ino) {}
void vfs_readlink(inode_t *ino) {}

_Noreturn void kpanic(const char *ms, ...)
{
    abort();
}

_Noreturn void task_fatal(const char *error, unsigned signum)
{
    abort();
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

TEST_CASE(tst_mem_01)
{
    kMMU.max_vma_size = 64 * PAGE_SIZE;
    mspace_t *msp = mspace_create();
    mspace_display(msp);

    // Map RW areas
    void *ptr1 = mspace_map(msp, 0, PAGE_SIZE, NULL, 0, VMA_ANON_RW);
    ck_ok(ptr1 != NULL);

    void *ptr2 = mspace_map(msp, 0, PAGE_SIZE, NULL, 0, VMA_ANON_RW);
    ck_ok(ptr2 != NULL);

    // Unmap area
    mspace_unmap(msp, (size_t)ptr1, PAGE_SIZE);

    // Change rights
    mspace_protect(msp, (size_t)ptr2, PAGE_SIZE, VMA_RO);

    // Map physical pages and resolve
    void *ptr3 = mspace_map(msp, 0, 2 * PAGE_SIZE, NULL, 0x4F000, VMA_PHYSIQ);
    ck_ok(ptr3 == NULL);
    kMMU.kspace = msp;
    ptr3 = mspace_map(msp, 0, 2 * PAGE_SIZE, NULL, 0x4F000, VMA_PHYSIQ);
    ck_ok(ptr3 != NULL);

    // Unmap physical pages
    mspace_unmap(msp, (size_t)ptr3, PAGE_SIZE);

    mspace_display(msp);
    mspace_close(msp);
}


TEST_CASE(tst_pg_01)
{
    memory_initialize();
    page_range(192 * PAGE_SIZE, 3 * PAGE_SIZE);

    page_t p1 = page_new();
    ck_ok(p1 != 0);

    page_t p2 = page_new();
    ck_ok(p2 != 0);

    page_t p3 = page_new();
    ck_ok(p3 != 0);

    page_release(p1);
    page_t p4 = page_new();
    ck_ok(p4 != 0 && p4 == p1);

    page_range(5 * PAGE_SIZE, 10 * PAGE_SIZE);
    page_range(16 * PAGE_SIZE, 128 * PAGE_SIZE);

    page_t p5 = page_new();
    ck_ok(p5 != 0);

    page_release(p1);
    page_release(p2);
    page_release(p3);
    // page_release(p4);
    page_release(p5);

    page_teardown();
}


jmp_buf __tcase_jump;
SRunner runner;

int main()
{
    kSYS.cpus = calloc(sizeof(struct kCpu), 0x500);
    // futex_init();

    suite_create("Memory");
    fixture_create("Virtual address space");
    tcase_create(tst_mem_01);

    fixture_create("Pages");
    tcase_create(tst_pg_01);

    free(kSYS.cpus);
    return summarize();
}


