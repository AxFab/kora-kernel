#include <kernel/core.h>
#include <sys/mman.h>
#include "opts.h"

void page_range(long long base, long long limit);

void mmu_setup()
{
    page_range(5 * PAGE_SIZE, 5 * PAGE_SIZE);
    page_range(25 * PAGE_SIZE, 20 * PAGE_SIZE);
    // Initialize memory mapping
    void *ptr = mmap(NULL, 512 * PAGE_SIZE, PROT_NONE, MAP_ANON | MAP_PRIVATE, -1, 0);
    uint16_t *tbl = kalloc(512 * sizeof(uint16_t));
    kprintf(-1, "Kernel heap at %p\n", ptr);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int mmu_context;
int mmu_create_uspace;
int mmu_destroy_uspace;
int mmu_drop;
int mmu_enable;
int mmu_leave;
int mmu_protect;
int mmu_resolve;

page_t mmu_read(size_t vaddr)
{
    return 0;
}
