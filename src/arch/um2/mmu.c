#include <kernel/core.h>
#include <kernel/memory.h>
#include <kernel/asm/mmu.h>
#include <kora/mcrs.h>


unsigned char mmu_bmp[MMU_LG] = { 0 };

size_t __um_mspace_pages_count = 64;
size_t __um_pages_available = 64;


struct mmu_dir {
    page_t dir;
    page_t *tbl;
    int cnt;
};

struct mmu_dir *kdir;
struct mmu_dir *udir;

#define MMU_KRN_START  (0x10 * PAGE_SIZE)
#define MMU_USR_START  (0x10000000)

/* Request from kernel early code to enable paging */
void mmu_enable()
{
    // List available memory
    page_range(8 * PAGE_SIZE, __um_pages_available * PAGE_SIZE);

    // // Setup allocator initial area
    // setup_allocator((void *)(1536 * _Kib_), 512 * _Kib_);

    kdir = kalloc(sizeof(struct mmu_dir));
    kdir->dir = page_new();
    kdir->cnt = __um_mspace_pages_count;
    kdir->tbl = kalloc(sizeof(page_t) * kdir->cnt);

    kMMU.kspace->lower_bound = MMU_KRN_START;
    kMMU.kspace->upper_bound = MMU_KRN_START + __um_mspace_pages_count * PAGE_SIZE;
    kMMU.kspace->directory = (page_t)kdir;
    kMMU.kspace->a_size += PAGE_SIZE;
    // Active change
}

void mmu_leave()
{
    kfree(kdir->tbl);
    page_release(kdir->dir);
    kfree(kdir);
}

void mmu_context(mspace_t *mspace)
{
    udir = (struct mmu_dir *)mspace->directory;
}

void mmu_create_uspace(mspace_t *mspace)
{
    struct mmu_dir *udir = kalloc(sizeof(struct mmu_dir));
    udir->dir = page_new();
    udir->cnt = __um_mspace_pages_count;
    udir->tbl = kalloc(sizeof(page_t) * udir->cnt);

    mspace->lower_bound = MMU_USR_START;
    mspace->upper_bound = MMU_USR_START + __um_mspace_pages_count * PAGE_SIZE;
    mspace->directory = (page_t)udir;
    mspace->a_size += PAGE_SIZE;
}

void mmu_destroy_uspace(mspace_t *mspace)
{
    struct mmu_dir *udir = (struct mmu_dir *)mspace->directory;
    kfree(udir->tbl);
    page_release(udir->dir);
    kfree(udir);
}


page_t mmu_read(mspace_t *mspace, size_t address)
{
    if (address >= kMMU.kspace->lower_bound && address < kMMU.kspace->upper_bound) {
        int pno = (address - kMMU.kspace->lower_bound) / PAGE_SIZE;
        return kdir->tbl[pno] & (~(PAGE_SIZE - 1));
    } else if (address >= MMU_USR_START) {
        int pno = (address - MMU_USR_START) / PAGE_SIZE;
        return udir->tbl[pno] & (~(PAGE_SIZE - 1));
    }
    return 0;
}

page_t mmu_drop(mspace_t *mspace, size_t address, bool clean)
{
    if (address >= kMMU.kspace->lower_bound && address < kMMU.kspace->upper_bound) {
        int pno = (address - kMMU.kspace->lower_bound) / PAGE_SIZE;
        page_t pg = kdir->tbl[pno] & (~(PAGE_SIZE - 1));
        if (pg != 0) {
            kdir->tbl[pno] = 0;
            kMMU.kspace->p_size -= PAGE_SIZE;
            kMMU.kspace->a_size -= PAGE_SIZE;
        }
        return pg;
    } else if (address >= MMU_USR_START) {
        int pno = (address - MMU_USR_START) / PAGE_SIZE;
        page_t pg = udir->tbl[pno] & (~(PAGE_SIZE - 1));
        if (pg != 0) {
            udir->tbl[pno] = 0;
            // uspace->p_size -= PAGE_SIZE;
            // uspace->a_size -= PAGE_SIZE;
        }
        return pg;
    }
    return 0;
}

void mmu_protect(mspace_t *mspace, size_t address, size_t length, int access)
{
    length /= PAGE_SIZE;
    while (length-- > 0) {
        int pno = (address - kMMU.kspace->lower_bound) / PAGE_SIZE;
        kdir->tbl[pno] = (kdir->tbl[pno] & (~(PAGE_SIZE - 1))) | access;
        address += PAGE_SIZE;
    }
}

int mmu_resolve(mspace_t *mspace, size_t address, page_t phys, int access, bool clean)
{
    if (phys == 0)
        phys = page_new();
    if (address >= kMMU.kspace->lower_bound && address < kMMU.kspace->upper_bound) {
        kMMU.kspace->p_size += PAGE_SIZE;
        kMMU.kspace->a_size += PAGE_SIZE;
        int pno = (address - kMMU.kspace->lower_bound) / PAGE_SIZE;
        kdir->tbl[pno] = phys | access;
        return 0;
    }
    kpanic("Bad resolve\n");
    return -1;
}

