#include <kernel/core.h>
#include <kernel/memory.h>
#include <kernel/mmu.h>
#include "opts.h"

#ifdef _WIN32
// Windows
#include <windows.h>

static inline void *__valloc(size_t len)
{
    return _aligned_malloc(len, PAGE_SIZE);
}

static inline void __vfree(void *ptr, size_t len)
{
    (void)len;
    _aligned_free(ptr);
}


#else
// Linux
#include <sys/mman.h>

static inline void *__valloc(size_t len)
{
    return mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
}

static inline void __vfree(void *ptr, size_t len)
{
    munmap(ptr, len);
}

#endif


void page_range(long long base, long long limit);

mspace_t kspace;

void mmu_setup()
{
    bbtree_init(&kspace.tree);
    kMMU.max_vma_size = 256 * PAGE_SIZE;
    kMMU.kspace = &kspace;
    page_range(5 * PAGE_SIZE, 5 * PAGE_SIZE);
    page_range(25 * PAGE_SIZE, 20 * PAGE_SIZE);
    // Initialize memory mapping
    void *ptr = __valloc(512 * PAGE_SIZE);
    ((long *)ptr)[0] = 0;
    kspace.lower_bound = (size_t)ptr;
    kspace.upper_bound = kspace.lower_bound + 512 * PAGE_SIZE;
    // uint16_t *tbl = kalloc(512 * sizeof(uint16_t));
    kprintf(-1, "Kernel heap at %p\n", ptr);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void mmu_context(mspace_t *ms) {}

void mmu_create_uspace(mspace_t *ms)
{
    void *ptr = __valloc(64 * PAGE_SIZE);
    ((long *)ptr)[0] = 0;
    ms->lower_bound = (size_t)ptr;
    ms->upper_bound = ms->lower_bound + 64 * PAGE_SIZE;
    // uint16_t *tbl = kalloc(64 * sizeof(uint16_t));
    kprintf(-1, "Create heap at %p\n", ptr);
}

void mmu_destroy_uspace(mspace_t *ms)
{
    __vfree((void *)ms->lower_bound, 64 * PAGE_SIZE);
}

void mmu_enable() {}
void mmu_leave() {}


int mmu_resolve(size_t vaddr, page_t phys, int flags)
{
    kprintf(-1, "mmu_resolve()\n");
    return -1;
}

page_t mmu_drop(size_t vaddr)
{
    kprintf(-1, "mmu_drop()\n");
    return 0;
}

page_t mmu_protect(size_t vaddr, int flags)
{
    kprintf(-1, "mmu_protect()\n");
    return 0;
}

page_t mmu_read(size_t vaddr)
{
    kprintf(-1, "mmu_read()\n");
    return 0;
}
