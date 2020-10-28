/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2019  <Fabien Bavent>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   - - - - - - - - - - - - - - -
 */
#include <kernel/memory.h>
#include <kernel/vfs.h>
// #include <kora/bbtree.h>
// #include <kora/splock.h>
// #include <stdatomic.h>
#include <string.h>
#include <assert.h>
#include <errno.h>


#define MSP_NAME(p,m,s) ((m) == kMMU.kspace ? p"Krn"s : p"Usr"s)

/* Helper to print VMA info into syslogs */
char *vma_print(char *buf, int len, vma_t *vma)
{
    static char *rights[] = { "---", "--x", "-w-", "-wx", "r--", "r-x", "rw-", "rwx"};
    char *nam;
    int i = 0;
    char sh = vma->flags & VMA_COW ? (vma->flags & VMA_SHARED ? 'W' : 'w')
              : (vma->flags & VMA_SHARED ? 'S' : 'p');
    i += snprintf(&buf[i], len - i, "%p-%p %s %s%c  {%04x} <%s> ",
                  (void *)vma->node.value_, (void *)(vma->node.value_ + vma->length),
                  rights[(vma->flags >> 4) & 7], rights[vma->flags & 7], sh,
                  vma->flags, sztoa(vma->length));
    switch (vma->flags & VMA_TYPE) {
    case VMA_HEAP:
        i += snprintf(&buf[i], len - i, "[heap]");
        break;
    case VMA_STACK:
        i += snprintf(&buf[i], len - i, "[stack]");
        break;
    case VMA_PIPE:
        i += snprintf(&buf[i], len - i, "[pipe]");
        break;
    case VMA_PHYS:
        i += snprintf(&buf[i], len - i, "PHYS=%p", (void *)((size_t)vma->offset));
        break;
    case VMA_ANON:
        break;
    case VMA_FILE:
        nam = kalloc(4096);
        vfs_inokey(vma->ino, nam);
        i += snprintf(&buf[i], len - i, "%s @%x", nam, vma->offset);
        kfree(nam);
        break;
    default:
        i += snprintf(&buf[i], len - i, "-");
        break;
    }
    return buf;
}

/* - */
static void vma_log(const char *prefix, vma_t *vma)
{
    // int len = 4096;
    // char *buf = kalloc(len);
    // kprintf(KLOG_MEM, "%s%s\n", prefix, vma_print(buf, len, vma));
    // kfree(buf);
}

/* - */
vma_t *vma_create(mspace_t *mspace, size_t address, size_t length, inode_t *ino,
                  xoff_t offset, int flags)
{
    vma_t *vma = (vma_t *)kalloc(sizeof(vma_t));
    vma->mspace = mspace;
    vma->node.value_ = address;
    vma->length = length;
    vma->ino = vfs_open_inode(ino);
    vma->offset = offset;
    vma->flags = flags;
    bbtree_insert(&mspace->tree, &vma->node);
    mspace->v_size += length;

    vma_log(MSP_NAME(" ", mspace, " ADD :: "), vma);
    return vma;
}

/* - */
vma_t *vma_clone(mspace_t *mspace, vma_t *model)
{
    vma_t *vma = (vma_t *)kalloc(sizeof(vma_t));
    vma->mspace = mspace;
    vma->node.value_ = model->node.value_;
    vma->length = model->length;
    vma->ino = vfs_open_inode(model->ino);
    vma->offset = model->offset;
    vma->flags = model->flags;
    bbtree_insert(&mspace->tree, &vma->node);
    mspace->v_size += model->length;

    if (vma->flags & VMA_SHARED) {

    } else {
        //if (vma->flags & VMA_CAN_WRITE) {
        //    // Set as READ ONLY / COW !
        //}
    }

    return NULL;
}

/* Split one VMA into two.
 * The VMA is cut after the length provided. A new VMA is add that will map
 * the rest of the VMA serving the exact same data.
 */
vma_t *vma_split(mspace_t *mspace, vma_t *area, size_t length)
{
    assert(splock_locked(&mspace->lock));
    if (area->length < length)
        return NULL;
    assert(area->length > length);

    /* Alloc a second one */
    vma_t *vma = (vma_t *)kalloc(sizeof(vma_t));
    vma->mspace = mspace;
    vma->node.value_ = area->node.value_ + length;
    vma->length = area->length - length;
    vma->ino = area->ino ? vfs_open_inode(area->ino) : NULL;
    vma->offset = area->offset + length;
    vma->flags = area->flags;

    /* Update size */
    area->length = length;

    /* Insert new one */
    bbtree_insert(&mspace->tree, &vma->node);
    return vma;
}

/* Close a VMA and release private pages. */
int vma_close(mspace_t *mspace, vma_t *vma, int arg)
{
    (void)arg;
    size_t off;
    assert(splock_locked(&mspace->lock));
    int type = vma->flags & VMA_TYPE;
    switch (type) {
    case VMA_PHYS:
        for (off = 0; off < vma->length; off += PAGE_SIZE)
            mmu_drop(vma->node.value_ + off);
        break;
    case VMA_FILE:
        for (off = 0; off < vma->length; off += PAGE_SIZE) {
            // TODO -- Look if page is dirty
            page_t pg = mmu_read(vma->node.value_ + off);
            if (pg != 0) {
                mmu_drop(vma->node.value_ + off);
                assert(vma->ino && vma->ino->ops);
                if (vma->ino->ops->release)
                    vma->ino->ops->release(vma->ino, vma->offset + off, pg, false);
            }
        }
        break;
    default:
        page_sweep(mspace, vma->node.value_, vma->length, true);
        break;
    }

    if (vma->ino)
        vfs_close_inode(vma->ino);
    bbtree_remove(&mspace->tree, vma->node.value_);

    vma_log(MSP_NAME(" ", mspace, " DEL :: "), vma);
    mspace->v_size -= vma->length;
    kfree(vma);
    return 0;
}

/* Change the flags of a VMA. */
int vma_protect(mspace_t *mspace, vma_t *vma, int flags)
{
    assert(splock_locked(&mspace->lock));
    /* Check permission */
    if ((flags & ((vma->flags >> 4) & VM_RWX)) != flags) {
        errno = EPERM;
        return -1;
    }

    /* Change access flags */
    size_t off;
    for (off = 0; off < vma->length; off += PAGE_SIZE)
        mmu_protect(vma->node.value_ + off, flags);
    vma->flags &= ~VM_RWX;
    vma->flags |= flags;

    vma_log(MSP_NAME(" ", mspace, " PRT :: "), vma);
    return 0;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int vma_resolve(vma_t *vma, size_t address, size_t length)
{
    assert(splock_locked(&vma->mspace->lock));
    assert((address & (PAGE_SIZE - 1)) == 0);
    assert((length & (PAGE_SIZE - 1)) == 0);
    assert(address + length <= vma->node.value_ + vma->length);

    int type = vma->flags & VMA_TYPE;
    xoff_t offset = vma->offset + (address - vma->node.value_);
    switch (type) {
    case VMA_PHYS:
        while (length > 0) {
            vma->mspace->p_size += mmu_resolve(address, (page_t)offset, vma->flags);
            length -= PAGE_SIZE;
            address += PAGE_SIZE;
            offset += PAGE_SIZE;
        }
        break;
    case VMA_HEAP:
    case VMA_STACK:
    case VMA_PIPE:
    case VMA_ANON:
    case VMA_EXEC:
        while (length > 0) {
            vma->mspace->p_size += mmu_resolve(address, 0, vma->flags);
            memset((void *)address, 0, PAGE_SIZE);
            length -= PAGE_SIZE;
            address += PAGE_SIZE;
        }
        break;
    case VMA_FILE:
        while (length > 0) {
            inode_t *ino = vma->ino;
            if (ino->fops->fetch == NULL) {
                errno = ENOSYS;
                kprintf(KL_ERR, "Unable to resolve already mapped file\n");
                return -1;
            }
            splock_unlock(&vma->mspace->lock);
            page_t pg = ino->fops->fetch(ino, offset); // CAN SLEEP!
            splock_lock(&vma->mspace->lock);
            if (pg == 0)
                return -1;
            // TODO Check we still have a valid VMA !
            vma->mspace->p_size += mmu_resolve(address, pg, vma->flags);
            length -= PAGE_SIZE;
            address += PAGE_SIZE;
            offset += PAGE_SIZE;
        }
        break;
    default:
        return -1;
    }
    return 0;
}



int vma_copy_on_write(vma_t *vma, size_t address, size_t length)
{
    assert(splock_locked(&vma->mspace->lock));
    assert((address & (PAGE_SIZE - 1)) == 0);
    assert((length & (PAGE_SIZE - 1)) == 0);
    assert(address + length <= vma->node.value_ + vma->length);

    vma->flags &= ~VMA_COW;
    vma->flags |= VM_WR;

    char *buf = kmap(length, NULL, 0, VM_RW);
    memcpy(buf, (void *)address, length);
    void *ptr = buf;
    size_t len = length;
    while (length > 0) {
        // TODO -- Clear or not ! release or decrement counter!
        page_t pg = mmu_drop(address);
        (void)pg;
        page_t copy = mmu_read((size_t)buf);
        vma->mspace->p_size += mmu_resolve(address, copy, vma->flags);
        address += PAGE_SIZE;
        length -= PAGE_SIZE;
        buf += PAGE_SIZE;
    }
    kunmap(ptr, len);
    return 0;
}
