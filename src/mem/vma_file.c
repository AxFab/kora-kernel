#include <kernel/stdc.h>
#include <kora/bbtree.h>
#include <kora/splock.h>
#include <kernel/memory.h>
#include <kernel/vfs.h>
#include <bits/atomic.h>
#include <errno.h>
#include <assert.h>


void vma_resolve_filecpy(vmsp_t *vmsp, vma_t *vma, size_t vaddr, size_t page)
{
    vmsp->s_size++;
    mmu_resolve(vaddr, page, VM_RD);
}

int vma_shared_filecpy(vmsp_t *vmsp, vma_t *vma, size_t address, size_t page)
{
    xoff_t offset = vma->offset + (xoff_t)(address - vma->node.value_);
    size_t old = vfs_fetch_page(vma->ino, offset, false);
    if (old != 0)
        vfs_release_page(vma->ino, offset, old, false);
    if (old != 0 && old == page)
        return VPG_BACKEDUP; // This is a backedup page
    if (!page_shared(vmsp->share, page, 0))
        return VPG_SHARED; // This is a shared page
    return VPG_PRIVATE; // This is a private page
}

void vma_unmap_filecpy(vmsp_t *vmsp, vma_t *vma, size_t address)
{
    bool dirty = mmu_dirty(address);
    size_t pg = mmu_drop(address);
    if (pg != 0) {
        int status = vma_shared_filecpy(vmsp, vma, address, pg);
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
            vfs_release_page(vma->ino, offset, pg, dirty);
        }
    }
}

int vma_protect_filecpy(vmsp_t *vmsp, vma_t *vma, int flags)
{
    // TODO -- if (flags & VM_WR && !(flags & VM_CAN_WR)) return -1;
    size_t length = vma->length;
    size_t address = vma->node.value_;
    vma->flags = (vma->flags & ~VM_RWX) | (flags & VM_RW);
    while (length) {
        size_t pg = mmu_read(address);
        if (pg != 0) {
            int status = vma_shared_filecpy(vmsp, vma, address, pg);
            if (status == VPG_PRIVATE)
                mmu_protect(address, flags & VM_RWX);
            else // if (status == VPG_BACKEDUP)
                mmu_protect(address, flags & VM_RX);
        }
        length -= PAGE_SIZE;
        address += PAGE_SIZE;
    }
    return 0;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

size_t vma_fetch_file(vmsp_t *vmsp, vma_t *vma, xoff_t offset, bool blocking)
{
    if (!blocking)
        return vfs_fetch_page(vma->ino, offset, false);
    splock_unlock(&vmsp->lock);
    size_t page = vfs_fetch_page(vma->ino, offset, true);
    splock_lock(&vmsp->lock);
    return page;
}

void vma_release_file(vmsp_t *vmsp, vma_t *vma, xoff_t offset, size_t page)
{
    vfs_release_page(vma->ino, offset, page, false);
}

void vma_resolve_file(vmsp_t *vmsp, vma_t *vma, size_t vaddr, size_t page)
{
    vmsp->s_size++;
    mmu_resolve(vaddr, page, vma->flags & VM_RW);
}

void vma_unmap_file(vmsp_t *vmsp, vma_t *vma, size_t address)
{
    bool dirty = mmu_dirty(address);
    size_t pg = mmu_drop(address);
    if (pg != 0) {
        xoff_t offset = vma->offset + (xoff_t)(address - vma->node.value_);
        vfs_release_page(vma->ino, offset, pg, dirty);
        vmsp->s_size--;
    }
}

int vma_protect_file(vmsp_t *vmsp, vma_t *vma, int flags)
{
    // TODO -- if (flags & VM_WR && !(flags & VM_CAN_WR)) return -1;
    size_t length = vma->length;
    size_t address = vma->node.value_;
    vma->flags = (vma->flags & ~VM_RWX) | (flags & VM_RW);
    while (length) {
        size_t pg = mmu_read(address);
        if (pg != 0)
            mmu_protect(address, flags & VM_RWX);
        length -= PAGE_SIZE;
        address += PAGE_SIZE;
    }
    return 0;
}

void vma_split_file(vma_t *va1, vma_t *va2)
{
    va2->flags = va1->flags;
    va2->ino = vfs_open_inode(va1->ino);
    va2->offset = va1->offset + va1->length;
}


void vma_clone_file(vmsp_t *vmsp1, vmsp_t *vmsp2, vma_t *va1, vma_t *va2)
{
    va1->ino = vfs_open_inode(va2->ino);
    va1->offset = va2->offset;
}

void vma_close_file(vma_t *vma)
{
    vfs_close_inode(vma->ino);
}

char *vma_print_file(vma_t *vma, char *buf, size_t len)
{
    char tmp[32];
    snprintf(buf, len, "%s @%ld", vfs_inokey(vma->ino, tmp), (long)(vma->offset / PAGE_SIZE));
    return buf;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

vma_ops_t vma_ops_filecpy = {
    .fetch = vma_fetch_file,
    .release = vma_release_file,
    .resolve = vma_resolve_filecpy,
    .shared = vma_shared_filecpy,
    .unmap = vma_unmap_filecpy,
    .protect = vma_protect_filecpy,
    .split = vma_split_file,
    .clone = vma_clone_file,
    .close = vma_close_file,
    .print = vma_print_file,
};

vma_ops_t vma_ops_file = {
    .fetch = vma_fetch_file,
    .release = vma_release_file,
    .resolve = vma_resolve_file,
    .unmap = vma_unmap_file,
    .protect = vma_protect_file,
    .split = vma_split_file,
    .clone = vma_clone_file,
    .close = vma_close_file,
    .print = vma_print_file,
};

