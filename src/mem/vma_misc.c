#include <kernel/stdc.h>
#include <kora/bbtree.h>
#include <kora/splock.h>
#include <kernel/memory.h>
#include <kernel/dlib.h>
#include <bits/atomic.h>
#include <errno.h>
#include <assert.h>


size_t vma_fetch_blank(vmsp_t *vmsp, vma_t *vma, xoff_t offset, bool blocking);
void vma_release_blank(vmsp_t *vmsp, vma_t *vma, xoff_t offset, size_t page);
void vma_resolve_blank(vmsp_t *vmsp, vma_t *vma, size_t vaddr, size_t page);
void vma_unmap_blank(vmsp_t *vmsp, vma_t *vma, size_t address);


char *vma_print_pipe(vma_t *vma, char *buf, size_t len)
{
    snprintf(buf, len, "[pipe.#%d]", -1); // vma->tsk ? vma->tsk->pid : -1);
    return buf;
}

vma_ops_t vma_ops_pipe = {
    .fetch = vma_fetch_blank,
    .release = vma_release_blank,
    .resolve = vma_resolve_blank,
    .unmap = vma_unmap_blank,
    .print = vma_print_pipe,
};

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

size_t vma_fetch_phys(vmsp_t *vmsp, vma_t *vma, xoff_t offset, bool blocking)
{
    return vma->offset == 0 ? page_new() : (size_t)offset;
}

void vma_resolve_phys(vmsp_t *vmsp, vma_t *vma, size_t vaddr, size_t page)
{
    int t = mmu_resolve(vaddr, page, vma->flags & (VM_UNCACHABLE | VM_RW));
    vmsp->t_size += t;
}

void vma_unmap_phys(vmsp_t *vmsp, vma_t *vma, size_t address)
{
    mmu_drop(address);
}

int vma_protect_phys(vmsp_t *vmsp, vma_t *vma, int flags)
{
    size_t length = vma->length;
    size_t address = vma->node.value_;
    vma->flags = flags & (VM_RW | VM_UNCACHABLE);
    while (length) {
        mmu_protect(address, vma->flags & (VM_UNCACHABLE | VM_RW));
        length -= PAGE_SIZE;
        address += PAGE_SIZE;
    }
    return 0;
}

char *vma_print_phys(vma_t *vma, char *buf, size_t len)
{
    snprintf(buf, len, "phys=%p", (void *)(size_t)vma->offset);
    return buf;
}

vma_ops_t vma_ops_phys = {
    .fetch = vma_fetch_phys,
    .resolve = vma_resolve_phys,
    .unmap = vma_unmap_phys,
    .print = vma_print_phys,

};
