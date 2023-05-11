#include <kernel/stdc.h>
#include <kora/bbtree.h>
#include <kora/splock.h>
#include <kernel/memory.h>
#include <kernel/dlib.h>
#include <bits/atomic.h>
#include <errno.h>
#include <assert.h>


size_t vma_fetch_blank(vmsp_t *vmsp, vma_t *vma, xoff_t offset, bool blocking)
{
    size_t page;
    if (vma->flags & VM_FAST_ALLOC) {
        page = page_new();
    } else {
        splock_unlock(&vmsp->lock);
        page = page_new();
        void *ptr = kmap(PAGE_SIZE, NULL, page, VMA_PHYS | VM_RW);
#ifdef KORA_KRN
        memset(ptr, 0, PAGE_SIZE);
#endif
        kunmap(ptr, PAGE_SIZE);
        splock_lock(&vmsp->lock);
    }
    return page;
}

void vma_release_blank(vmsp_t *vmsp, vma_t *vma, xoff_t offset, size_t page)
{
    page_release(page);
}

void vma_resolve_blank(vmsp_t *vmsp, vma_t *vma, size_t vaddr, size_t page)
{
    vmsp->p_size++;
    int t = mmu_resolve(vaddr, page, vma->flags & VM_RW);
    vmsp->t_size += t;
}

void vma_unmap_blank(vmsp_t *vmsp, vma_t *vma, size_t address)
{

    size_t pg = mmu_drop(address);
    if (pg != 0) {
        bool private = page_shared(vmsp->share, pg, 0);
        if (private) {
            vmsp->p_size--;
            page_release(pg);
        } else {
            vmsp->s_size--;
            if (page_shared(vmsp->share, pg, -1))
                page_release(pg);
        }
    }
}

int vma_protect_anon(vmsp_t *vmsp, vma_t *vma, int flags)
{
    size_t length = vma->length;
    size_t address = vma->node.value_;
    vma->flags = (vma->flags & ~VM_RWX) | (flags & VM_RWX);
    while (length) {
        size_t pg = mmu_read(address);
        if (pg != 0) {
            if (vma->flags & VMA_COW) {
                bool private = page_shared(vmsp->share, pg, 0);
                if (private)
                    mmu_protect(address, flags & VM_RWX);
                else
                    mmu_protect(address, flags & VM_RX);
            } else
                mmu_protect(address, flags & VM_RWX);
        }
        length -= PAGE_SIZE;
        address += PAGE_SIZE;
    }
    return 0;
}

void vma_split_anon(vma_t *va1, vma_t *va2)
{
    va2->flags = va1->flags;
}

char *vma_print_anon(vma_t *vma, char *buf, size_t len)
{
    strncpy(buf, "-", len);
    return buf;
}

vma_ops_t vma_ops_anon = {
    .fetch = vma_fetch_blank,
    .release = vma_release_blank,
    .resolve = vma_resolve_blank,
    .unmap = vma_unmap_blank,
    .protect = vma_protect_anon,
    .split = vma_split_anon,
    .print = vma_print_anon,
};

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=


char *vma_print_stack(vma_t *vma, char *buf, size_t len)
{
    snprintf(buf, len, "[stack.#%d]", 0);// vma->tsk->pid);
    return buf;
}

char *vma_print_heap(vma_t *vma, char *buf, size_t len)
{
    strncpy(buf, "[heap]", len);
    return buf;
}

vma_ops_t vma_ops_stack = {
    .fetch = vma_fetch_blank,
    .release = vma_release_blank,
    .resolve = vma_resolve_blank,
    .unmap = vma_unmap_blank,
    .print = vma_print_stack,

};

vma_ops_t vma_ops_heap = {
    .fetch = vma_fetch_blank,
    .release = vma_release_blank,
    .resolve = vma_resolve_blank,
    .unmap = vma_unmap_blank,
    .print = vma_print_heap,
};

