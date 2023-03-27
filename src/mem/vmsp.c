#include <kernel/stdc.h>
#include <kora/bbtree.h>
#include <kora/splock.h>
#include <kernel/memory.h>
#include <kernel/dlib.h>
#include <bits/atomic.h>
#include <errno.h>
#include <assert.h>


int vma_resolve(vmsp_t *vmsp, vma_t *vma, size_t vaddr, bool missing, bool write);
void vma_unmap(vmsp_t *vmsp, vma_t *vma);

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

extern vma_ops_t vma_ops_anon;
extern vma_ops_t vma_ops_stack;
extern vma_ops_t vma_ops_heap;
extern vma_ops_t vma_ops_pipe;
extern vma_ops_t vma_ops_phys;
extern vma_ops_t vma_ops_filecpy;
extern vma_ops_t vma_ops_file;
extern vma_ops_t vma_ops_dlib;

static vma_t *vma_create(vmsp_t *vmsp, size_t address, size_t length, void *ptr, xoff_t offset, int flags)
{
    assert(splock_locked(&vmsp->lock));
    vma_t *vma = kalloc(sizeof(vma_t));
    vma->node.value_ = address;
    vma->length = length;
    vma->usage = 1;
    int type = flags & VMA_TYPE;
    int mask = ((flags & VM_EX) ? VM_RX : VM_RW);
    int opts = vmsp == __mmu.kspace ? VM_FAST_ALLOC : VMA_CLEAN;

    switch (type) {
    case VMA_ANON:
        vma->flags = opts | (flags & mask) | VMA_COW;
        vma->ops = &vma_ops_anon;
        break;
    //case VMA_SHDANON:
    //    vma_open_anon(vmsp, vma, ptr, offset, flags);
    //    vma->flags |= VM_SHARED; // TODO
    //    break;
    case VMA_STACK:
        vma->flags = opts | VM_RW;
        vma->tsk = ptr;
        vma->ops = &vma_ops_stack;
        break;
    case VMA_HEAP:
        vma->flags = opts | VM_RW;
        vma->ops = &vma_ops_heap;
        break;
    case VMA_PIPE:
        assert(vmsp == __mmu.kspace);
        // TODO -- assert(ptr != NULL);
        vma->flags = VM_FAST_ALLOC | VM_RW;
        vma->ino = ptr;
        vma->ops = &vma_ops_pipe;
        break;
    case VMA_PHYS:
        assert(vmsp == __mmu.kspace);
        flags |= VM_RESOLVE;
        vma->flags = (flags & (VM_RW | VM_UNCACHABLE));
        vma->offset = offset;
        vma->ops = &vma_ops_phys;
        break;
    case VMA_FILECPY:
        assert(ptr != NULL);
        vma->flags = (flags & VM_RW) | VMA_COW | VMA_BACKEDUP;
        vma->ino = vfs_open_inode(ptr);
        vma->offset = offset;
        vma->ops = &vma_ops_filecpy;
        break;
    case VMA_FILE:
        assert(ptr != NULL);
        vma->flags = (flags & VM_RW) | VMA_BACKEDUP;
        vma->ino = vfs_open_inode(ptr);
        vma->offset = offset;
        vma->ops = &vma_ops_file;
        break;
    case VMA_DLIB:
        assert(ptr != NULL);
        vma->flags = 0;
        vma->lib = ptr;
        vma->offset = offset;
        vma->ops = &vma_ops_dlib;
        break;
    default:
        assert(false);
        break;
    }

    bbtree_insert(&vmsp->tree, &vma->node);
    vmsp->v_size += length / PAGE_SIZE;
    // kprintf(KL_VMA, "On %s%p, add vma %s\n", VMS_NAME(vmsp), vmsp, vma->ops->print(vma, tmp, 32));

    if (flags & VM_RESOLVE) {
        while (length > 0) {
            int ret = vma_resolve(vmsp, vma, address, true, false);
            if (ret != 0) {
                // TODO -- Cancel and unmap !?
                break;
            }
            address += PAGE_SIZE;
            length -= PAGE_SIZE;
        }
    }

    return vma;
}


int vma_resolve(vmsp_t *vmsp, vma_t *vma, size_t vaddr, bool missing, bool write)
{
    // We should check vmsp is locked, but irq_semaphore == 1 !

    xoff_t offset = vma->offset + (xoff_t)(vaddr - vma->node.value_);
    if (missing) {
        // Look for page
        size_t page = vma->ops->fetch(vmsp, vma, offset, false);
        if (page == 0) {
            page = vma->ops->fetch(vmsp, vma, offset, true);

            if (unlikely(vma->flags & VM_UNMAPED)) {
                vma->ops->release(vmsp, vma, offset, page);
                vma_unmap(vmsp, vma);
                kprintf(KL_PF, PF_ERR"Mapping removed at this address '%p'\n", (void *)vaddr);
                return -1;
            }

            if (page == 0) {
                kprintf(KL_PF, PF_ERR"Mapping can't retrieve the page at '%p'\n", (void *)vaddr);
                return -1;
            }
        }

        // Resolve the page
        vma->ops->resolve(vmsp, vma, vaddr, page);
    }

    if ((!missing || (vma->flags & VMA_BACKEDUP)) && write && (vma->flags & VMA_COW)) {
        assert(vma->flags & VM_WR);

        // Copy the page
        splock_unlock(&vmsp->lock);
        size_t page = page_new();
        void *ptr = kmap(PAGE_SIZE, NULL, page, VMA_PHYS | VM_RW);
#ifdef KORA_KRN
        memcpy(ptr, (void *)vaddr, PAGE_SIZE);
#endif
        kunmap(ptr, PAGE_SIZE);
        splock_lock(&vmsp->lock);

        if (vma->flags & VM_UNMAPED) {
            vma->ops->release(vmsp, vma, offset, page);
            vma_unmap(vmsp, vma);
            kprintf(KL_PF, PF_ERR"Mapping removed at this address '%p'\n", (void *)vaddr);
            return -1;
        }

        // Release previous one
        size_t old = mmu_read(vaddr);
        int status = VPG_SHARED;
        if (vma->flags & VMA_BACKEDUP)
            status = vma->ops->shared(vmsp, vma, vaddr, old);
        if (status == VPG_BACKEDUP)
            vma->ops->release(vmsp, vma, offset, old);
        else {
            if (page_shared(vmsp->share, old, -1))
                page_release(old);
        }

        // Map new page
        vmsp->s_size--;
        vmsp->p_size++;
        mmu_resolve(vaddr, page, VM_RW);
    }

    return 0;
}

void vma_unmap(vmsp_t *vmsp, vma_t *vma)
{
    // char tmp[32];
    assert(splock_locked(&vmsp->lock));
    if ((vma->flags & VM_UNMAPED) == 0) {
        bbtree_remove(&vmsp->tree, vma->node.value_);
        // kprintf(KL_VMA, "On %s%p, close vma %s\n", VMS_NAME(vmsp), vmsp, vma->ops->print(vma, tmp, 32));
        // Unmap pages
        vmsp->v_size -= vma->length / PAGE_SIZE;
        size_t length = vma->length;
        size_t address = vma->node.value_;
        while (length > 0) {
#if _KORA_KRN
            if (vma->flags & VMA_CLEAN && mmu_read(address) != 0)
                memset((void *)address, 0, PAGE_SIZE);
#endif
            vma->ops->unmap(vmsp, vma, address);
            length -= PAGE_SIZE;
            address += PAGE_SIZE;
        }
        
        vma->flags |= VM_UNMAPED;
    }
    if (atomic_xadd(&vma->usage, -1) != 1)
        return;
    if (vma->ops->close)
        vma->ops->close(vma);
    // Close
    kfree(vma);
}

vma_t *vma_clone(vmsp_t *vmsp1, vmsp_t *vmsp2, vma_t *vma)
{
    // char tmp[32];
    vma_t *cpy = kalloc(sizeof(vma_t));
    cpy->node.value_ = vma->node.value_;
    cpy->length = vma->length;
    cpy->usage = 1;
    cpy->flags = vma->flags;
    cpy->ops = vma->ops;
    if (vma->ops->clone)
        vma->ops->clone(vmsp1, vmsp2, cpy, vma);
   
    if (vma->flags & VMA_COW) {
        size_t length = vma->length;
        size_t address = vma->node.value_;
        while (length > 0) {
            size_t page = mmu_read(address);
            if (page != 0) {
                int status = 0;
                if (vma->flags & VMA_BACKEDUP)
                    status = vma->ops->shared(vmsp1, vma, address, page);
                else {
                    status = page_shared(vmsp1->share, page, 0) ? VPG_PRIVATE : VPG_SHARED;
                }

                if (status == VPG_PRIVATE) {
                    mmu_set(vmsp1->directory, address, page, vma->flags & VM_RX);
                    vmsp1->s_size++;
                    mmu_protect(address, vma->flags & VM_RX);
                    vmsp2->s_size++;
                    vmsp2->p_size--;
                    page_shared(vmsp1->share, page, 2);
                } else if (status == VPG_SHARED) {
                    mmu_set(vmsp1->directory, address, page, vma->flags & VM_RX);
                    vmsp1->s_size++;
                    page_shared(vmsp1->share, page, 1);
                }
                // Else nothing to do, need to fetch anyway...
            }
            length -= PAGE_SIZE;
            address += PAGE_SIZE;
        }
    }

    bbtree_insert(&vmsp1->tree, &cpy->node);
    vmsp1->v_size += vma->length / PAGE_SIZE;
    // kprintf(KL_VMA, "On %s%p, clone vma %s from\n", VMS_NAME(vmsp1), vmsp1, cpy->ops->print(cpy, tmp, 32));
    return vma;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

static size_t vmsp_find_slot(vmsp_t *vmsp, size_t length)
{
    assert(splock_locked(&vmsp->lock));
    vma_t *vma = bbtree_first(&vmsp->tree, vma_t, node);
    if (vma == NULL || vmsp->lower_bound + length <= vma->node.value_)
        return vmsp->lower_bound;

    for (;;) {
        if (vma->node.value_ + vma->length + length > vmsp->upper_bound) {
            errno = ENOMEM;
            return 0;
        }

        vma_t *next = bbtree_next(&vma->node, vma_t, node);
        if (next == NULL || vma->node.value_ + vma->length + length <= next->node.value_) {
            return vma->node.value_ + vma->length;
        }
        vma = next;   
    }
}

static size_t vmsp_slot_address(vmsp_t *vmsp, size_t address, size_t length)
{
    if (address < vmsp->lower_bound || address + length > vmsp->upper_bound)
        return 0;
    vma_t *vma = bbtree_search_le(&vmsp->tree, address, vma_t, node);
    if (vma == NULL) {
        vma = bbtree_first(&vmsp->tree, vma_t, node);
        if (vma != NULL && address + length > vma->node.value_)
        	return 0;
    } else {
        if (vma->node.value_ + vma->length > address)
        	return false;
        vma = bbtree_next(&vma->node, vma_t, node);
        if (vma != NULL && address + length > vma->node.value_)
        	return 0;
    }
    return address;
}

vma_t *vmsp_find_area(vmsp_t *vmsp, size_t address)
{
    assert(splock_locked(&vmsp->lock));
    if (address < vmsp->lower_bound || address >= vmsp->upper_bound)
        return NULL;
    vma_t *vma = bbtree_search_le(&vmsp->tree, address, vma_t, node);
    if (vma == NULL || vma->node.value_ + vma->length <= address)
        return NULL;
    return vma;
}

size_t vmsp_map(vmsp_t *vmsp, size_t address, size_t length, void *ptr, xoff_t offset, int flags)
{
    // Check parameters
    if (length == 0 || (length & (PAGE_SIZE - 1)) != 0 || (offset & (PAGE_SIZE - 1)) != 0) {
        errno = EINVAL;
        return 0;
    } else if (length > vmsp->max_size) {
        errno = E2BIG;
        return 0;
    }

    // Look for a slot
    splock_lock(&vmsp->lock);

    // If we have an address, check availability
    size_t base = 0;
    if (address != 0) {
        base = vmsp_slot_address(vmsp, address, length);
        if (base == 0 && flags & VMA_FIXED) {
            errno = ERANGE;
            splock_unlock(&vmsp->lock);
            return 0;
        }
    }

    // If we don't have address yet, look for a new spot
    if (base == 0)
        base = vmsp_find_slot(vmsp, length);

    if (base == 0) {
        errno = ENOMEM;
        splock_unlock(&vmsp->lock);
        return 0;
    }

    // Create the VMA
    vma_t *vma = vma_create(vmsp, base, length, ptr, offset, flags);
    if (vma == NULL) {
        splock_unlock(&vmsp->lock);
        return 0;
    }

    errno = 0;
    splock_unlock(&vmsp->lock);
    return base;
}

static vma_t *vma_check_range(vmsp_t *vmsp, vma_t *vma, size_t base, size_t length)
{
    // char tmp1[32], tmp2[32];
    assert(splock_locked(&vmsp->lock));
    if (vma->node.value_ > base || vma->ops->split == NULL)
        return NULL;

    size_t limit = vma->node.value_ + vma->length;
    vma_t *nx = vma;
    while (base + length > limit) {
        nx = bbtree_next(&nx->node, vma_t, node);
        if (nx == NULL || nx->node.value_ != limit || nx->ops != vma->ops || nx->lib != vma->lib)
            return NULL;
        limit = nx->node.value_ + nx->length;
    }

    if (vma->node.value_ != base) {
        assert(base > vma->node.value_);
        size_t nlen = base - vma->node.value_;
        vma_t *sec = kalloc(sizeof(vma_t));
        sec->usage = 1;
        sec->node.value_ = base;
        sec->length = vma->length - nlen;
        vma->length = nlen;
        sec->ops = vma->ops;
        vma->ops->split(vma, sec);

        bbtree_insert(&vmsp->tree, &sec->node);
        // kprintf(KL_VMA, "On %s%p, split vma %s/%s\n", VMS_NAME(vmsp), vmsp, vma->ops->print(vma, tmp1, 32), sec->ops->print(sec, tmp2, 32));
        vma = sec;
    }

    assert(vma->node.value_ == base);
    return vma;
}

static vma_t *vma_check_limit(vmsp_t *vmsp, vma_t *vma, size_t base, size_t length)
{
    // char tmp1[32], tmp2[32];
    size_t limit = base + length;
    assert(limit >= vma->node.value_);
    length = limit - vma->node.value_;

    if (length == 0)
        return NULL;

    if (length < vma->length) {
        vma_t *sec = kalloc(sizeof(vma_t));
        sec->usage = 1;
        sec->node.value_ = base + length;
        sec->length = vma->length - length;
        vma->length = length;
        sec->ops = vma->ops;
        vma->ops->split(vma, sec);

        bbtree_insert(&vmsp->tree, &sec->node);
        // kprintf(KL_VMA, "On %s%p, split vma %s/%s\n", VMS_NAME(vmsp), vmsp, vma->ops->print(vma, tmp1, 32), sec->ops->print(sec, tmp2, 32));
    }

    return vma;
}

int vmsp_unmap(vmsp_t *vmsp, size_t base, size_t length)
{
    splock_lock(&vmsp->lock);
    vma_t *vma = vmsp_find_area(vmsp, base);
    if (vma == NULL) {
        splock_unlock(&vmsp->lock);
        errno = EINVAL;
        return -1;
    }

    if (vma->node.value_ == base && vma->length == length) {
        vma_unmap(vmsp, vma);
        splock_unlock(&vmsp->lock);
        return 0;
    }

    vma_t *cur = vma_check_range(vmsp, vma, base, length);
    if (cur == NULL) {
        splock_unlock(&vmsp->lock);
        errno = EINVAL;
        return -1;
    }

    for (;;) {
        cur = vma_check_limit(vmsp, cur, base, length);
        if (cur == NULL)
            break;
        vma_t *next = bbtree_next(&cur->node, vma_t, node);
        vma_unmap(vmsp, cur);
        // TODO -- Can we merge front, can we merge back if last one close it
        if (next == NULL)
            break;
        cur = next;
    }

    splock_unlock(&vmsp->lock);
    return 0;
}

int vmsp_protect(vmsp_t *vmsp, size_t base, size_t length, int flags)
{
    splock_lock(&vmsp->lock);
    vma_t *vma = vmsp_find_area(vmsp, base);
    if (vma == NULL || vma->ops->protect == NULL) {
        errno = EINVAL;
        splock_unlock(&vmsp->lock);
        return -1;
    }

    if (vma->node.value_ == base && vma->length == length) {
        int ret = vma->ops->protect(vmsp, vma, flags);
        splock_unlock(&vmsp->lock);
        return ret;
    }

    vma_t *cur = vma_check_range(vmsp, vma, base, length);
    if (cur == NULL) {
        splock_unlock(&vmsp->lock);
        errno = EINVAL;
        return -1;
    }

    int ret = 0;
    for (;;) {
        cur = vma_check_limit(vmsp, cur, base, length);
        if (cur == NULL)
            break;
        ret |= cur->ops->protect(vmsp, cur, flags);
        // TODO -- Can we merge front, can we merge back
        cur = bbtree_next(&cur->node, vma_t, node);
        if (cur == NULL)
            break;
    }

    splock_unlock(&vmsp->lock);
    return ret;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

static vmsp_t *vmsp_build()
{
    vmsp_t *vmsp = kalloc(sizeof(vmsp_t));
    bbtree_init(&vmsp->tree);
    splock_init(&vmsp->lock);
    vmsp->usage = 1;
    vmsp->max_size = VMSP_MAX_SIZE; // TODO -- configurable
    mmu_create_uspace(vmsp);
    return vmsp;
}

vmsp_t *vmsp_create()
{
    vmsp_t *vmsp = vmsp_build();
    vmsp->share = kalloc(sizeof(page_sharing_t));
    vmsp->share->usage = 1;
    hmp_init(&vmsp->share->map, 16);
    return vmsp;
}

vmsp_t *vmsp_open(vmsp_t *vmsp)
{
    assert(vmsp != __mmu.kspace);
    atomic_inc(&vmsp->usage);
    return vmsp;
}

vmsp_t *vmsp_clone(vmsp_t *vmsp)
{
    assert(vmsp != __mmu.kspace);
    splock_lock(&vmsp->lock);
    vmsp_t *copy = vmsp_build();
    assert(vmsp->lower_bound == copy->lower_bound);
    assert(vmsp->upper_bound == copy->upper_bound);

    copy->share = vmsp->share;
    atomic_inc(&copy->share->usage);
    vma_t *vma = bbtree_first(&vmsp->tree, vma_t, node);
    while (vma != NULL) {
    	vma_clone(copy, vmsp, vma);
        vma = bbtree_next(&vma->node, vma_t, node);
    }
    splock_unlock(&vmsp->lock);
    return copy;
}

/* Release all VMAs */
void vmsp_sweep(vmsp_t *vmsp)
{
	splock_lock(&vmsp->lock);
	vma_t *vma = bbtree_first(&vmsp->tree, vma_t, node);
	while (vma != NULL) {
        vma_unmap(vmsp, vma);
		vma = bbtree_first(&vmsp->tree, vma_t, node);
	}
	splock_unlock(&vmsp->lock);
}

void vmsp_close(vmsp_t *vmsp)
{
    assert(vmsp != __mmu.kspace);
    if (atomic_xadd(&vmsp->usage, -1) != 1)
        return;
    vmsp_sweep(vmsp);
    if (atomic_xadd(&vmsp->share->usage, -1) == 1) {
        assert(vmsp->share->map.count == 0);
        hmp_destroy(&vmsp->share->map);
        kfree(vmsp->share);
    }
    mmu_destroy_uspace(vmsp);
    if (vmsp->proc)
        dlib_destroy(vmsp->proc);
    kfree(vmsp);
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

int vmsp_fault(const char *message, size_t address)
{
    kprintf(KL_PF, message, (void *)address);
    return -1;
}

int vmsp_resolve(vmsp_t *vmsp, size_t address, bool missing, bool write)
{
    if (vmsp == NULL)
        return vmsp_fault(PF_ERR"Address is outside of addressable space '%p'\n", address);

    splock_lock(&vmsp->lock);
    vma_t *vma = vmsp_find_area(vmsp, address);
    if (vma == NULL) {
        splock_unlock(&vmsp->lock);
        return vmsp_fault(PF_ERR"No mapping at this address '%p'\n", address);
    }

    if (write && !(vma->flags & VM_WR)) {
        splock_unlock(&vmsp->lock);
        return vmsp_fault(PF_ERR"Can't write on read-only memory at '%p'\n", address);
    }

    errno = 0;
    size_t vaddr = ALIGN_DW(address, PAGE_SIZE);
    int ret = vma_resolve(vmsp, vma, vaddr, missing, write);
    splock_unlock(&vmsp->lock);
    return ret;
}

char *vma_print(vma_t *vma, char *buf, int len)
{
    static char *rights[] = { "---", "--x", "-w-", "-wx", "r--", "r-x", "rw-", "rwx" };
    char sh = vma->flags & VMA_COW ? (vma->flags & VM_SHARED ? 'W' : 'w')
        : (vma->flags & VM_SHARED ? 'S' : 'p');
    // TODO -- Issue with xoff_t on print !
    int i = snprintf(buf, len, "%p-%p %s%c %012"XOFF_FX" {%04x} <%s>  ",
        (void *)vma->node.value_, (void *)(vma->node.value_ + vma->length),
        rights[vma->flags & 7], sh, vma->offset, vma->flags, sztoa(vma->length));
    vma->ops->print(vma, &buf[i], len - i);
    return buf;
}

void vma_debug(vma_t *vma)
{
    char rgs[4];
    int sz = vma->length / PAGE_SIZE;
    size_t address = vma->node.value_;
    rgs[3] = '\0';
    for (int i = 0; i < sz; ++i) {
        if ((i % 8) == 0)
            kprintf(-1, "\n  ");
        size_t pg = mmu_read(address + i * PAGE_SIZE);
        int flg = mmu_read_flags(address + i * PAGE_SIZE);
        rgs[0] = (flg & VM_RD ? 'r' : '-');
        rgs[1] = (flg & VM_WR ? 'w' : '-');
        rgs[2] = (flg & VM_EX ? 'x' : '-');
        if (pg == 0)
            kprintf(-1, "  .........");
        else
            kprintf(-1, "  %05x.%s", pg >> 12, rgs);
    }
    kprintf(-1, "\n");
}

void vmsp_display(vmsp_t *vmsp)
{
    splock_lock(&vmsp->lock);
    kprintf(KL_DBG, "------------------------------------------------\n");
    kprintf(KL_DBG,
        "%p-%p virtual: %d KB   private: %d KB   shared: %d KB   table: %d KB\n",
        vmsp->lower_bound, vmsp->upper_bound,
        vmsp->v_size * KB, vmsp->p_size * KB, vmsp->s_size * KB, vmsp->t_size * KB);
    kprintf(KL_DBG, "------------------------------------------------\n");
    char *buf = kalloc(512);
    vma_t *vma = bbtree_first(&vmsp->tree, vma_t, node);
    while (vma) {
    vma_print(vma, buf, 512);
    if (vma->length <= 128 * _Kib_) {
        kprintf(KL_DBG, "%s", buf);
        vma_debug(vma);
    } else {
        kprintf(KL_DBG, "%s\n", buf);
    }
    vma = bbtree_next(&vma->node, vma_t, node);
    }
    kfree(buf);
    splock_unlock(&vmsp->lock);
}

