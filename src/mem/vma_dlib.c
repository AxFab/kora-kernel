#include <kernel/stdc.h>
#include <kora/bbtree.h>
#include <kora/splock.h>
#include <kernel/memory.h>
#include <kernel/dlib.h>
#include <bits/atomic.h>
#include <errno.h>
#include <assert.h>


size_t vma_fetch_dlib(vmsp_t *vmsp, vma_t *vma, xoff_t offset, bool blocking)
{
    if (!blocking)
        return dlib_fetch_page(vma->lib, offset, false);
    splock_unlock(&vmsp->lock);
    size_t page = dlib_fetch_page(vma->lib, offset, true);
    splock_lock(&vmsp->lock);
    return page;
}

void vma_release_dlib(vmsp_t *vmsp, vma_t *vma, xoff_t offset, size_t page)
{
    dlib_release_page(vma->lib, offset, page);
}

void vma_resolve_dlib(vmsp_t *vmsp, vma_t *vma, size_t vaddr, size_t page)
{
    vmsp->s_size++;
    int t = mmu_resolve(vaddr, page, VM_RD);
    vmsp->t_size += t;
}

int vma_shared_dlib(vmsp_t *vmsp, vma_t *vma, size_t address, size_t page)
{
    xoff_t offset = vma->offset + (xoff_t)(address - vma->node.value_);
    size_t old = dlib_fetch_page(vma->lib, offset, false);
    if (old != 0)
        dlib_release_page(vma->lib, offset, old);
    if (old != 0 && old == page)
        return VPG_BACKEDUP; // This is a backedup page
    if (!page_shared(vmsp->share, page, 0))
        return VPG_SHARED; // This is a shared page
    return VPG_PRIVATE; // This is a private page
}

void vma_unmap_dlib(vmsp_t *vmsp, vma_t *vma, size_t address)
{
    size_t pg = mmu_drop(address);
    if (pg != 0) {
        int status = vma_shared_dlib(vmsp, vma, address, pg);
        if (status == VPG_PRIVATE) {
            vmsp->p_size--;
            page_release(pg);
        } else if (status == VPG_SHARED) {
            vmsp->s_size--;
            if (page_shared(vmsp->share, pg, -1))
                page_release(pg);
        } else {
            xoff_t offset = vma->offset + (xoff_t)(address - vma->node.value_);
            vmsp->s_size--;
            dlib_release_page(vma->lib, offset, pg);
        }
    }
}


int vma_protect_dlib(vmsp_t *vmsp, vma_t *vma, int flags)
{
    if (vma->flags != 0)
        return -1;
    if (flags != VM_RX && flags != VM_RW && flags != VM_RD)
        return -1;
    vma->flags = flags | VMA_COW | VMA_BACKEDUP;
    return 0;
}

void vma_split_dlib(vma_t *va1, vma_t *va2)
{
    va2->flags = va1->flags;
    va2->lib = va1->lib;
    va2->offset = va1->offset + va1->length;
}

void vma_clone_dlib(vmsp_t *vmsp1, vmsp_t *vmsp2, vma_t *va1, vma_t *va2)
{
    va1->lib = va2->lib;
    va1->offset = va2->offset;
}

char *vma_print_dlib(vma_t *vma, char *buf, size_t len)
{
    char tmp[32];
    snprintf(buf, len, "<%s>", dlib_name(vma->lib, tmp, 32));
    return buf;
}


vma_ops_t vma_ops_dlib = {
    .fetch = vma_fetch_dlib,
    .release = vma_release_dlib,
    .resolve = vma_resolve_dlib,
    .shared = vma_shared_dlib,
    .unmap = vma_unmap_dlib,
    .protect = vma_protect_dlib,
    .split = vma_split_dlib,
    .clone = vma_clone_dlib,
    .print = vma_print_dlib,

};
