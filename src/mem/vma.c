///*
// *      This file is part of the KoraOS project.
// *  Copyright (C) 2015-2021  <Fabien Bavent>
// *
// *  This program is free software: you can redistribute it and/or modify
// *  it under the terms of the GNU Affero General Public License as
// *  published by the Free Software Foundation, either version 3 of the
// *  License, or (at your option) any later version.
// *
// *  This program is distributed in the hope that it will be useful,
// *  but WITHOUT ANY WARRANTY; without even the implied warranty of
// *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// *  GNU Affero General Public License for more details.
// *
// *  You should have received a copy of the GNU Affero General Public License
// *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
// *
// *   - - - - - - - - - - - - - - -
// */
//#include <kernel/memory.h>
//#include <kernel/vfs.h>
//#include <kernel/dlib.h>
//#include <string.h>
//#include <assert.h>
//#include <errno.h>
//
//#define VMS_NAME(m) ((m) == __mmu.kspace ? "Krn" : "Usr")
//
//
///* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
//
//int vma_no_protect(vma_t *vma, int flags)
//{
//    errno = EPERM;
//    return -1;
//}
//
//int vma_std_protect(vma_t *vma, int flags)
//{
//    int type = vma->flags & VMA_TYPE;
//    int cap = vma->flags >> 3 & VM_RWX;
//    if ((cap & flags) != flags) {
//        errno = EPERM;
//        return -1;
//    }
//    int opts = vma->flags & (VM_SHARED | VM_UNCACHABLE); 
//    vma->flags = type | opts | (flags & cap) | (cap << 3) | VM_RD;
//    return 0;
//}
//
//
//
//int vma_anon_open(vma_t *vma, int access, inode_t *ino, xoff_t offset)
//{
//    // No Shared support yet
//    if (access & VM_SHARED)
//        return -1;
//    int mask = ((access & VM_EX) ? VM_RX : VM_RW);
//    vma->flags = VMA_ANON | (access & mask);
//    vma->flags |= mask << 3;
//    return 0;
//}
//
//int vma_anon_resolve(vma_t *vma, size_t address)
//{
//    mmu_resolve(address, 0, vma->flags);
//    // TODO -- memset((void *)address, 0, PAGE_SIZE);
//    return 0;
//}
//
//int vma_anon_cow(vma_t *vma, size_t address)
//{
//    return 0;
//}
//
//void vma_anon_close(vma_t *vma)
//{
//    size_t length = vma->length;
//    size_t address = vma->node.value_;
//    while (length) {
//        if (vma->flags & VMA_CLEAN && mmu_read(address) != 0)
//            memset((void *)address, 0, PAGE_SIZE);
//        size_t pg = mmu_drop(address);
//        if (pg != 0) {
//            vma->mspace->p_size--;
//            page_release(pg);
//        }
//        address += PAGE_SIZE;
//        length -= PAGE_SIZE;
//    }
//}
//
//int vma_anon_clone(vma_t *vma, vma_t *model)
//{
//    // If private open info for COW
//    // If shared should already be mapped
//    return 0;
//}
//
//char *vma_anon_print(vma_t *vma, char *buf, int len)
//{
//    *buf = '\0';
//    return buf;
//}
//
//vma_ops_t vma_anon_ops = {
//    .open = vma_anon_open,
//    .resolve = vma_anon_resolve,
//    .cow = vma_anon_cow,
//    .close = vma_anon_close,
//    .print = vma_anon_print,
//    .protect = vma_std_protect,
//    .split = NULL,
//    .clone = vma_anon_clone,
//};
//
//
//
//int vma_heap_open(vma_t *vma, int access, inode_t *ino, xoff_t offset)
//{
//    int type = vma->flags & VMA_TYPE;
//    vma->flags = type | VM_RW;
//    return 0;
//}
//
////int vma_heap_resolve(vma_t *vma, size_t address)
////{
////    if (vma->cow_reg) {
////        int idx = (address - vma->node.value_ + vma->offset) / PAGE_SIZE;
////        size_t page = vma->cow_reg->pages[idx];
////        if (page != 0) {
////            mmu_resolve(address, page, vma->flags & ~VM_WR);
////            return 0;
////        }
////    }
////
////    mmu_resolve(address, 0, vma->flags);
////    // TODO -- memset((void *)address, 0, PAGE_SIZE);
////    return 0;
////}
//
//int vma_heap_cow(vma_t *vma, size_t address)
//{
//    // Check we are using the common page
//    size_t page = mmu_read(address);
//    int idx = (address - vma->node.value_ + vma->offset) / PAGE_SIZE;
//    if (page != vma->cow_reg->pages[idx])
//        return -1;
//
//    // Copy the page
//    void *des = kmap(PAGE_SIZE, NULL, 0, VMA_PHYS | VM_RW);
//    void *src = kmap(PAGE_SIZE, NULL, page, VMA_PHYS | VM_RW);
//    page = mmu_read((size_t)des);
//    memcpy(des, src, PAGE_SIZE);
//    kunmap(des, PAGE_SIZE);
//
//    // Map the new private page
//    mmu_resolve(address, page, vma->flags);
//    return 0;
//}
//
//void vma_heap_cow_destroy(vma_cow_reg_t *reg)
//{
//    for (unsigned i = 0; i < reg->pages_count; ++i) {
//        size_t pg = reg->pages[i];
//        if (pg != 0) {
//            page_release(pg);
//        }
//    }
//    kfree(reg);
//}
//
////void vma_heap_close(vma_t *vma)
////{
////    size_t length = vma->length;
////    size_t address = vma->node.value_;
////    while (length) {
////        if (vma->flags & VMA_CLEAN && mmu_read(address) != 0)
////            memset((void *)address, 0, PAGE_SIZE);
////        size_t pg = mmu_drop(address);
////        if (pg != 0) {
////            page_release(pg);
////        }
////        address += PAGE_SIZE;
////        length -= PAGE_SIZE;
////    }
////
////    vma_cow_reg_t *old = vma->cow_reg;
////    if (old && atomic_xadd(&old->rcu, -1) == 1)
////        vma_heap_cow_destroy(old);
////}
//
//int vma_heap_clone(vma_t *vma, vma_t *model)
//{
//    size_t pages = vma->length / PAGE_SIZE;
//    if (model->cow_reg)
//        pages = model->cow_reg->pages_count;
//    vma_cow_reg_t *reg = kalloc(sizeof(vma_cow_reg_t) + pages * sizeof(size_t));
//    reg->rcu = 2;
//    vma->cow_reg = reg;
//    if (model->cow_reg) {
//        reg->parent = model->cow_reg;
//        vma->offset = model->offset;
//    }
//
//    size_t address = model->node.value_;
//    size_t length = model->length;
//    size_t idx = model->offset / PAGE_SIZE;
//
//    for (size_t off = 0; off < length; off += PAGE_SIZE) {
//        size_t page = mmu_read(address + off);
//        if (page == 0 && model->cow_reg)
//            page = model->cow_reg->pages[idx];
//        reg->pages[idx] = page;
//        if (page != 0) {
//            mmu_protect(address, VMA_COW | VM_RD);
//            // Page info should be already set
//        }
//    }
//
//    vma_cow_reg_t *old = model->cow_reg;
//    model->cow_reg = reg;
//    if (old && atomic_xadd(&old->rcu, -1) == 1)
//        vma_heap_cow_destroy(old);
//
//    return 0;
//}
//
//char *vma_heap_print(vma_t *vma, char *buf, int len)
//{
//    int type = vma->flags & VMA_TYPE;
//    if (type == VMA_HEAP)
//        strncpy(buf, "[heap]", len);
//    else if (type == VMA_STACK)
//        strncpy(buf, "[stack]", len);
//    return buf;
//}
//
//vma_ops_t vma_heap_ops = {
//    .open = vma_heap_open,
//    .resolve = vma_anon_resolve,
//    .cow = vma_heap_cow,
//    .close = vma_anon_close,
//    .print = vma_heap_print,
//    .protect = vma_no_protect,
//    .split = NULL,
//    .clone = vma_heap_clone,
//};
//
//
//int vma_pipe_open(vma_t *vma, int access, inode_t *ino, xoff_t offset)
//{
//    if (vma->mspace != __mmu.kspace) {
//        errno = EPERM;
//        return -1;
//    }
//    vma->ino = ino;
//    vma->flags = VMA_PIPE | VM_RW;
//    return 0;
//}
//
//char *vma_pipe_print(vma_t *vma, char *buf, int len)
//{
//    // int type = vma->flags & VMA_TYPE;
//    snprintf(buf, len, "[pipe.%03x]", (int)((size_t)vma->ino));
//    return buf;
//}
//
//vma_ops_t vma_pipe_ops = {
//    .open = vma_pipe_open,
//    .resolve = vma_anon_resolve,
//    .cow = NULL,
//    .close = vma_anon_close,
//    .print = vma_pipe_print,
//    .protect = vma_no_protect,
//    .split = NULL,
//    .clone = NULL,
//};
//
//
//int vma_phys_open(vma_t *vma, int access, inode_t *ino, xoff_t offset)
//{
//    if (vma->mspace != __mmu.kspace || access & VM_SHARED) {
//        errno = EPERM;
//        return -1;
//    }
//    vma->offset = offset;
//    vma->flags = VMA_PHYS | access | (VM_RW << 3);
//    return 0;
//}
//
//int vma_phys_resolve(vma_t *vma, size_t address)
//{
//    xoff_t offset = vma->offset != 0 ? vma->offset + (address - vma->node.value_) : 0;
//    mmu_resolve(address, (page_t)offset, vma->flags);
//    if (offset == 0)
//        vma->mspace->a_size++;
//    return 0;
//}
//
//void vma_phys_close(vma_t *vma)
//{
//    size_t length = vma->length;
//    size_t address = vma->node.value_;
//    while (length) {
//        size_t pg = mmu_drop(address);
//        if (pg && vma->offset == 0)
//            vma->mspace->a_size--;
//        address += PAGE_SIZE;
//        length -= PAGE_SIZE;
//    }
//}
//
//char *vma_phys_print(vma_t *vma, char *buf, int len)
//{
//    snprintf(buf, len, "PHYS=%p", (void *)((size_t)vma->offset));
//    return buf;
//}
//
//void vma_phy_split(vma_t *vma, vma_t *area, size_t length)
//{
//    vma->offset = area->offset != 0 ? area->offset + length : 0;
//}
//
//
//vma_ops_t vma_phys_ops = {
//    .open = vma_phys_open,
//    .resolve = vma_phys_resolve,
//    .cow = NULL,
//    .close = vma_phys_close,
//    .print = vma_phys_print,
//    .protect = vma_std_protect,
//    .split = vma_phy_split,
//    .clone = NULL,
//};
//
//
//int vma_file_open(vma_t *vma, int access, inode_t *ino, xoff_t offset)
//{
//    if (ino == NULL) // TODO -- Check can be mapped
//        return -1;
//    vma->ino = vfs_open_inode(ino);
//    vma->offset = offset;
//    // TODO -- Check length or not !?
//    // TODO -- Can be SHARED or COW !?
//    vma->flags = VMA_FILE | VM_SHARED | (access & VM_RWX) | (VM_RW << 3);
//    return 0;
//}
//
//int vma_file_resolve(vma_t *vma, size_t address)
//{
//    xoff_t offset = vma->offset + (address - vma->node.value_);
//    // MUTEX ON VMA !?
//    size_t pg = vfs_fetch_page(vma->ino, offset);
//    if (pg == 0)
//        return -1;
//    mmu_resolve(address, pg, vma->flags);
//    return 0;
//}
//
//int vma_file_cow(vma_t *vma, size_t address)
//{
//    return 0;
//}
//
//void vma_file_close(vma_t *vma)
//{
//    size_t length = vma->length;
//    size_t address = vma->node.value_;
//    xoff_t offset = vma->offset;
//    while (length) {
//        bool isdirty = mmu_dirty(address);
//        size_t pg = mmu_drop(address);
//        if (pg != 0) {
//            vfs_release_page(vma->ino, offset, pg, isdirty);
//        }
//        address += PAGE_SIZE;
//        offset += PAGE_SIZE;
//        length -= PAGE_SIZE;
//    }
//    vfs_close_inode(vma->ino);
//}
//
//int vma_file_clone(vma_t *vma, vma_t *model)
//{
//    // Should not be anything to do...
//    return 0;
//}
//
//char *vma_file_print(vma_t *vma, char *buf, int len)
//{
//    char tmp[16];
//    snprintf(buf, len, "%s @"XOFF_F, vfs_inokey(vma->ino, tmp), vma->offset);
//    return buf;
//}
//
//void vma_file_split(vma_t *vma, vma_t *area, size_t length)
//{
//    vma->ino = vfs_open_inode(area->ino);
//    vma->offset = area->offset + length; 
//}
//
//
//vma_ops_t vma_file_ops = {
//    .open = vma_file_open,
//    .resolve = vma_file_resolve,
//    .cow = vma_file_cow,
//    .close = vma_file_close,
//    .print = vma_file_print,
//    .protect = vma_std_protect,
//    .split = vma_file_split,
//    .clone = vma_file_clone,
//};
//
//int vma_exec_open(vma_t *vma, int access, inode_t *lib, xoff_t offset)
//{
//    if (lib == NULL) // TODO -- Is ready?
//        return -1;
//    vma->offset = offset;
//    vma->ino = lib;
//    vma->flags = VMA_CODE | (access & VM_RWX);
//    return 0;
//}
//
//int vma_exec_resolve(vma_t *vma, size_t address)
//{
//    xoff_t offset = vma->offset + (address - vma->node.value_);
//    size_t pg = dlib_fetch_page((dlib_t *)vma->ino, offset);
//    if (pg == 0)
//        return -1;
//    mmu_resolve(address, pg, vma->flags & ~VM_RX);
//    return 0;
//}
//
//int vma_exec_cow(vma_t *vma, size_t address)
//{
//    page_t pg_shared = mmu_read(address);
//    void *src = kmap(PAGE_SIZE, NULL, pg_shared, VMA_PHYS | VM_RW);
//    void *trg = kmap(PAGE_SIZE, NULL, 0, VMA_PHYS | VM_RW);
//    memcpy(trg, src, PAGE_SIZE);
//    page_t pg_priv = mmu_read((size_t)trg);
//    mmu_resolve(address, pg_priv, vma->flags);
//    return 0;
//}
//
//void vma_exec_close(vma_t *vma)
//{
//    size_t length = vma->length;
//    size_t address = vma->node.value_;
//    xoff_t offset = vma->offset;
//    while (length) {
//        size_t pg = mmu_drop(address);
//        if (pg != 0) {
//            size_t ac = dlib_fetch_page((dlib_t *)vma->ino, offset);
//            if (pg == ac) {
//                dlib_release_page((dlib_t *)vma->ino, offset, pg);
//                vma->mspace->s_size--;
//            } else {
//                page_release(pg);
//                vma->mspace->p_size--;
//            }
//        }
//        address += PAGE_SIZE;
//        offset += PAGE_SIZE;
//        length -= PAGE_SIZE;
//    }
//
//    // dlib_close(vma->ino);
//}
//
//int vma_exec_clone(vma_t *vma, vma_t *model)
//{
//    // Should not be anything to do...
//    return 0;
//}
//
//char *vma_exec_print(vma_t *vma, char *buf, int len)
//{
//    char *typ = "";
//    int access = vma->flags & VM_RWX;
//    if (access == VM_RX)
//        typ = "<code>";
//    else if (access == VM_RW)
//        typ = "<data>";
//    else if (access == VM_RD)
//        typ = "<rodata>";
//    else
//        typ = "<.?>";
//
//    char tmp[64];
//    snprintf(buf, len, "%s %s", typ, dlib_name((dlib_t *)vma->ino, tmp, 64));
//    return buf;
//}
//
//int vma_exec_protect(vma_t *vma, int flags)
//{
//    if ((vma->flags & VM_RWX) != VM_RWX)
//        return -1;
//    vma->flags = VMA_CODE | VMA_COW | (flags & VM_RWX);
//    //for (size_t off = 0; off < vma->length; off += PAGE_SIZE)
//    //    mmu_protect(vma->node.value_ + off, vma->flags);
//    return 0;
//}
//
//void vma_exec_split(vma_t *vma, vma_t *area, size_t length)
//{
//    vma->ino = area->ino;
//    vma->offset = area->offset + length;
//}
//
//
//vma_ops_t vma_exec_ops = {
//    .open = vma_exec_open,
//    .resolve = vma_exec_resolve,
//    .cow = vma_exec_cow,
//    .close = vma_exec_close,
//    .print = vma_exec_print,
//    .protect = vma_exec_protect,
//    .split = vma_exec_split,
//};
//
//
///* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
//
///* Helper to print VMA info into syslogs */
//char *vma_print(char *buf, int len, vma_t *vma)
//{
//    static char *rights[] = { "---", "--x", "-w-", "-wx", "r--", "r-x", "rw-", "rwx" };
//    char sh = vma->flags & VMA_COW ? (vma->flags & VM_SHARED ? 'W' : 'w')
//        : (vma->flags & VM_SHARED ? 'S' : 'p');
//    int i = snprintf(buf, len, "%p-%p %s%c %012"XOFF_FX" {%04x} <%s>  ",
//        (void *)vma->node.value_, (void *)(vma->node.value_ + vma->length),
//        rights[vma->flags & 7], sh, vma->offset, vma->flags, sztoa(vma->length));
//
//    vma->ops->print(vma, &buf[i], len - i);
//    return buf;
//}
//
///* - */
//vma_t *vma_create(mspace_t *mspace, size_t address, size_t length, inode_t *ino, xoff_t offset, int flags)
//{
//    char tmp[32];
//    assert(splock_locked(&mspace->lock));
//    assert((address & (PAGE_SIZE - 1)) == 0);
//    assert((length & (PAGE_SIZE - 1)) == 0);
//    assert((offset & (PAGE_SIZE - 1)) == 0);
//
//    int type = flags & VMA_TYPE;
//    int access = flags & (VM_RW | VM_EX | VM_RESOLVE | VM_SHARED | VM_UNCACHABLE);
//
//    vma_t *vma = (vma_t *)kalloc(sizeof(vma_t));
//    vma->mspace = mspace;
//    vma->node.value_ = address;
//    vma->length = length;
//    vma->flags = type | (flags & (VMA_CLEAN));
//
//    switch (type) {
//    case VMA_ANON:
//        vma->ops = &vma_anon_ops;
//        break;
//    case VMA_STACK:
//        if (mspace == __mmu.kspace)
//            access |= VM_RESOLVE;
//        // fall through
//    case VMA_HEAP:
//        vma->ops = &vma_heap_ops;
//        break;
//    case VMA_PIPE:
//        vma->ops = &vma_pipe_ops;
//        break;
//    case VMA_PHYS:
//        access |= VM_RESOLVE;
//        vma->ops = &vma_phys_ops;
//        break;
//    case VMA_FILE:
//        vma->ops = &vma_file_ops;
//        break;
//    case VMA_CODE:
//        vma->ops = &vma_exec_ops;
//        break;
//    default:
//        errno = EINVAL;
//        kfree(vma);
//        return NULL;
//    }
//
//    vma->offset = offset;
//    int ret = vma->ops->open(vma, access, ino, offset);
//    if (ret != 0) {
//        kfree(vma);
//        return NULL;
//    }
//    bbtree_insert(&mspace->tree, &vma->node);
//    mspace->v_size += length / PAGE_SIZE;
//    kprintf(KL_VMA, "On %s%p, add vma %s\n", VMS_NAME(mspace), mspace, vma->ops->print(vma, tmp, 32));
//
//    if (access & VM_RESOLVE)
//        vma_resolve(vma, address, length);
//
//    return vma;
//}
//
///* - */
//vma_t *vma_clone(mspace_t *mspace, vma_t *model)
//{
//    char tmp[32];
//    // assert(splock_locked(&mspace->lock));
//    
//    vma_t *vma = (vma_t *)kalloc(sizeof(vma_t));
//    vma->mspace = mspace;
//    vma->node.value_ = model->node.value_;
//    vma->length = model->length;
//    vma->flags = model->flags;
//    vma->ops = model->ops;
//
//    int ret = vma->ops->clone(vma, model);
//    if (ret == -1)
//        return NULL;
//    bbtree_insert(&mspace->tree, &vma->node);
//    mspace->v_size += vma->length / PAGE_SIZE;
//    kprintf(KL_VMA, "On %s%p, clone vma %s from\n", VMS_NAME(mspace), mspace, model->ops->print(model, tmp, 32));
//    return vma;
//}
//
///* Split one VMA into two.
// * The VMA is cut after the length provided. A new VMA is add that will map
// * the rest of the VMA serving the exact same data.
// */
//vma_t *vma_split(mspace_t *mspace, vma_t *area, size_t length)
//{
//    char tmp[32];
//    assert((length & (PAGE_SIZE - 1)) == 0);
//    assert(splock_locked(&mspace->lock));
//    assert(area->length > length);
//    kprintf(KL_VMA, "On %s%p, split vma %s\n", VMS_NAME(mspace), mspace, area->ops->print(area, tmp, 32));
//
//    // Alloc a second one
//    vma_t *vma = (vma_t *)kalloc(sizeof(vma_t));
//    vma->mspace = mspace;
//    vma->node.value_ = area->node.value_ + length;
//    vma->flags = area->flags;
//    vma->length = area->length - length;
//    vma->ops = area->ops;
//    if (vma->ops->split)
//        vma->ops->split(vma, area, length);
//    // Update size
//    area->length = length;
//    // Insert new one
//    bbtree_insert(&mspace->tree, &vma->node);
//    return vma;
//}
//
///* Close a VMA and release private pages. */
//int vma_close(mspace_t *mspace, vma_t *vma, int arg)
//{
//    char tmp[32];
//    (void)arg;
//    assert(splock_locked(&mspace->lock));
//    bbtree_remove(&mspace->tree, vma->node.value_);
//    kprintf(KL_VMA, "On %s%p, close vma %s\n", VMS_NAME(mspace), mspace, vma->ops->print(vma, tmp, 32));
//    vma->ops->close(vma);
//    mspace->v_size -= vma->length / PAGE_SIZE;
//    kfree(vma);
//    return 0;
//}
//
///* Change the flags of a VMA. */
//int vma_protect(mspace_t *mspace, vma_t *vma, int flags)
//{
//    char tmp[32];
//    assert(splock_locked(&mspace->lock));
//    // Check permission
//    if (vma->ops->protect(vma, flags) != 0) {
//        errno = EPERM;
//        return -1;
//    }
//
//    // Change access flags
//    size_t off;
//    for (off = 0; off < vma->length; off += PAGE_SIZE)
//        mmu_protect(vma->node.value_ + off, vma->flags);
//
//    kprintf(KL_VMA, "On %s%p, change protect vma %s\n", VMS_NAME(mspace), mspace, vma->ops->print(vma, tmp, 32));
//    return 0;
//}
//
///* Resolve missing page */
//int vma_resolve(vma_t *vma, size_t address, size_t length)
//{
//    assert(splock_locked(&vma->mspace->lock));
//    assert((address & (PAGE_SIZE - 1)) == 0);
//    assert((length & (PAGE_SIZE - 1)) == 0);
//    assert(address >= vma->node.value_);
//    assert(address + length <= vma->node.value_ + vma->length);
//    int ret = 0;
//    while (length > 0) {
//        if (vma->ops->resolve(vma, address) != 0)
//            ret = -1;
//        address += PAGE_SIZE;
//        length -= PAGE_SIZE;
//    }
//    return ret;
//}
//
//
///* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
//
//int vma_copy_on_write(vma_t *vma, size_t address, size_t length)
//{
//    assert(splock_locked(&vma->mspace->lock));
//    assert((address & (PAGE_SIZE - 1)) == 0);
//    assert((length & (PAGE_SIZE - 1)) == 0);
//    assert(address >= vma->node.value_);
//    assert(address + length <= vma->node.value_ + vma->length);
//    int ret = 0;
//    if (vma->ops->cow == NULL)
//        return -1;
//    while (length > 0) {
//        if (vma->ops->cow(vma, address) != 0)
//            ret = -1;
//        address += PAGE_SIZE;
//        length -= PAGE_SIZE;
//    }
//    return ret;
//}
//
//
//void vma_debug(vma_t *vma)
//{
//    char rgs[4];
//    // char buf[128];
//    // vma_print(buf, 128, vma);
//    // kprintf(-1, buf);
//    int sz = vma->length / PAGE_SIZE;
//    size_t address = vma->node.value_;
//    rgs[3] = '\0';
//    for (int i = 0; i < sz; ++i) {
//        if ((i % 8) == 0)
//            kprintf(-1, "\n  ");
//        size_t pg = mmu_read(address + i * PAGE_SIZE);
//        int flg = mmu_read_flags(address + i * PAGE_SIZE);
//        rgs[0] = (flg & VM_RD ? 'r' : '-');
//        rgs[1] = (flg & VM_WR ? 'w' : '-');
//        rgs[2] = (flg & VM_EX ? 'x' : '-');
//        if (pg == 0)
//            kprintf(-1, "  .........");
//        else
//            kprintf(-1, "  %05x.%s", pg >> 12, rgs);
//    }
//    kprintf(-1, "\n");
//}
#include <kernel/stdc.h>
#include <kora/bbtree.h>
#include <kora/splock.h>
#include <kernel/memory.h>
#include <kernel/vfs.h>
#include <kernel/dlib.h>
#include <bits/atomic.h>
#include <errno.h>
#include <assert.h>


//
//
//void vma_clone(vmsp_t *vmsp1, vmsp_t *vmsp2, vma_t *va1, vma_t *va2)
//{
//    va1->ops->clone(vmsp1, vmsp2, va1, va2);
//    //// DLIB
//    //va1->lib = va2->lib;
//    //va1->offset = va2->offset;
//
//    //// FILECPY | FILE
//    //va1->ino = vfs_open_inode(va2->ino);
//    //va1->offset = va2->offset;
//    
//    //// ANON...
//
//    if (va1->flags & VMA_COW) {
//        size_t length = va1->length;
//        size_t address = va1->node.value_;
//        while (length > 0) {
//            size_t page = mmu_read(address);
//            if (page == 0)
//                continue;
//
//            int status = 0;
//            if (va1->flags & VMA_BACKEDUP)
//                status = va1->ops->shared(va1, address, page);
//            else {
//                status = page_shared(page, 0) ? VPG_PRIVATE : VPG_SHARED;
//            }
//
//            mmu_set(vmsp1->directory, address, page, va1->flags & VM_RX);
//            vmsp1->s_size++;
//            if (status == VPG_PRIVATE) {
//                mmu_protect(address, va1->flags & VM_RX);
//                vmsp2->s_size++;
//                vmsp2->p_size--;
//                page_shared(page, 2);
//            } else if (status == VPG_SHARED) {
//                page_shared(page, 1);
//            }
//            // Else nothing to do, need to fetch anyway...
//
//            length -= PAGE_SIZE;
//            address += PAGE_SIZE;
//        }
//    }
//}
//




