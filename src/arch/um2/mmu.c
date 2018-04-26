#include <kernel/core.h>
#include <kernel/memory.h>
#include <kernel/asm/mmu.h>
#include <kora/mcrs.h>


unsigned char mmu_bmp[MMU_LG] = { 0 };

/* Request from kernel early code to enable paging */
void mmu_enable()
{
    // List available memory
    page_range(12 * _Kib_, 64 * _Kib_);

    // // Setup allocator initial area
    // setup_allocator((void *)(1536 * _Kib_), 512 * _Kib_);

    // // Prepare Kernel pages
    // page_t *dir = (page_t *)KRN_PG_DIR;
    // page_t *tbl = (page_t *)KRN_PG_TBL;
    // memset(dir, 0, PAGE_SIZE);
    // memset(tbl, 0, PAGE_SIZE);
    // dir[1022] = KRN_PG_DIR | MMU_K_RW;
    // dir[1023] = KRN_PG_DIR | MMU_K_RW;
    // dir[0] = KRN_PG_TBL | MMU_K_RW;
    // for (i = 0; i < 512; ++i) {
    //     tbl[i] = (i * PAGE_SIZE) | MMU_K_RW;
    // }

    // kMMU.kspace->lower_bound = MMU_KSPACE_LOWER;
    // kMMU.kspace->upper_bound = MMU_KSPACE_UPPER;

    // Active change
}


void mmu_create_uspace(mspace_t *mspace)
{
    mspace->lower_bound = 4 * PAGE_SIZE;
    mspace->upper_bound = 128 * PAGE_SIZE;
}

void mmu_destroy_uspace(mspace_t *mspace)
{
}


page_t mmu_read(size_t address, bool drop, bool clean)
{
    return 0;
}

int mmu_resolve(size_t address, page_t phys, int access, bool clean)
{
    return 0;
}



