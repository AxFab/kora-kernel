/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2021  <Fabien Bavent>
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
#include <kernel/stdc.h>
#include <kernel/vfs.h>
#include <kernel/core.h>
#include <errno.h>
#include <assert.h>

typedef struct block_file block_file_t;
typedef struct block_page block_page_t;

struct block_file {
    bbtree_t tree;
    splock_t lock;
    llhead_t llru;
};

struct block_page {
    bool ready;
    bbnode_t node;
    atomic_int rcu;
    page_t phys;
    bool dirty;
    mtx_t mtx;
    llnode_t nlru;
};

page_t mmu_read(size_t address);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

static void block_close_page(block_file_t *block, block_page_t *page)
{
    if (atomic_xadd(&page->rcu, -1) == 1) {
        splock_lock(&block->lock);
        if (page->rcu != 0) {
            splock_unlock(&block->lock);
            return;
        }

        // Put into LRU
        ll_enqueue(&block->llru, &page->nlru);
        splock_unlock(&block->lock);
    }
}

void block_scavenge(block_file_t *block, int max)
{
    splock_lock(&block->lock);
    while (max-- > 0) {
        block_page_t *page = ll_dequeue(&block->llru, block_page_t, nlru);
        if (page == NULL)
            break;
        int lba = page->node.value_;
        bbtree_remove(&block->tree, lba);
        page_release(page->phys);
        kfree(page);
    }
    splock_unlock(&block->lock);
}

page_t block_fetch(inode_t *ino, xoff_t off)
{
    char tmp[20];
    block_file_t *block = ino->fl_data;
    assert(IS_ALIGNED(off, PAGE_SIZE));
    splock_lock(&block->lock);

    assert(off >= 0 && off < (PAGE_SIZE * 0x100000000LL));
    size_t lba = off / PAGE_SIZE;
    block_page_t *page = bbtree_search_eq(&block->tree, lba, block_page_t, node);
    if (page == NULL) {
        page = kalloc(sizeof(block_page_t));
        mtx_init(&page->mtx, mtx_plain);
        page->rcu = 0;
        page->node.value_ = lba;
        bbtree_insert(&block->tree, &page->node);
    } else if (ll_contains(&block->llru, &page->nlru))
        ll_remove(&block->llru, &page->nlru);
    atomic_inc(&page->rcu);
    splock_unlock(&block->lock);

    if (!page->ready) {
        mtx_lock(&page->mtx);
        if (!page->ready) {
            void *ptr = kmap(PAGE_SIZE, NULL, 0, VM_RW | VM_RESOLVE | VM_PHYSIQ);
            assert(irq_ready());
            page->phys = mmu_read((size_t)ptr);
            kprintf(KL_BIO, "Alloc page %p for inode %s, read at %llx\n", page->phys, vfs_inokey(ino, tmp), off);
            int ret = ino->ops->read(ino, ptr, PAGE_SIZE, off, 0);
            if (ret != 0) {
                kprintf(-1, "\033[35mError while reading page: %s, pg:%d\033[0m\n", vfs_inokey(ino, tmp), off / PAGE_SIZE);
                mtx_unlock(&page->mtx);
                block_close_page(block, page);
                return 0;
            }
            assert(page->phys != 0);
            page->ready = true;
            // mmu_drop((size_t)ptr);
            kunmap(ptr, PAGE_SIZE);
        }
        mtx_unlock(&page->mtx);
    }

    return page->phys;
}

void block_release(inode_t *ino, xoff_t off, page_t pg, bool dirty)
{
    char tmp[20];
    block_file_t *block = ino->fl_data;
    assert(IS_ALIGNED(off, PAGE_SIZE));
    splock_lock(&block->lock);

    assert(off >= 0 && off < (PAGE_SIZE * 0x100000000LL));
    size_t lba = off / PAGE_SIZE;
    block_page_t *page = bbtree_search_eq(&block->tree, lba, block_page_t, node);
    assert(page != NULL);
    splock_unlock(&block->lock);

    if (dirty || page->dirty) { // TODO -- Check block is not RDONLY !?
        mtx_lock(&page->mtx);
        void *ptr = kmap(PAGE_SIZE, NULL, (xoff_t)page->phys, VM_RW | VM_RESOLVE | VM_PHYSIQ);
        assert(irq_ready());
        kprintf(KL_BIO, "Write back page %p for inode %s at %llx\n", page->phys, vfs_inokey(ino, tmp), off);
        int ret = ino->ops->write(ino, ptr, PAGE_SIZE, off, 0);
        if (ret != 0) {
            page->dirty = true;
            kprintf(-1, "\033[35mError while syncing page: %s, pg:%d\033[0m\n", vfs_inokey(ino, tmp), off / PAGE_SIZE);
        } else
            page->dirty = false;
        mtx_unlock(&page->mtx);
        kunmap(ptr, PAGE_SIZE);
    }

    block_close_page(block, page);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


int block_read(inode_t *ino, char *buf, size_t len, xoff_t off, int flags)
{
    if (off >= ino->length && ino->length != 0)
        return -1;
    xoff_t poff = -1;
    char *map = NULL;
    int bytes = 0;
    while (len > 0) {
        xoff_t po = ALIGN_DW(off, PAGE_SIZE);
        if (poff != po) {
            if (map != NULL)
                kunmap(map, PAGE_SIZE);
            poff = po;
            map = kmap(PAGE_SIZE, ino, poff, VM_RD | VM_RESOLVE);
            if (map == NULL)
                return -1;
        }
        size_t disp = (size_t)(off & (PAGE_SIZE - 1));
        int cap = MIN3((size_t)len, PAGE_SIZE - disp, (size_t)(ino->length - off));
        if (cap == 0)
            return bytes;
        memcpy(buf, map + disp, cap);
        len -= cap;
        off += cap;
        buf += cap;
        bytes += cap;
    }
    kunmap(map, PAGE_SIZE);
    return bytes;
}

int block_write(inode_t *ino, const char *buf, size_t len, xoff_t off, int flags)
{
    if (off >= ino->length && ino->length != 0)
        return -1;
    xoff_t poff = -1;
    char *map = NULL;
    int bytes = 0;
    while (len > 0) {
        xoff_t po = ALIGN_DW(off, PAGE_SIZE);
        if (poff != po) {
            if (map != NULL)
                kunmap(map, PAGE_SIZE);
            poff = po;
            map = kmap(PAGE_SIZE, ino, poff, VM_RW | VM_RESOLVE);
            if (map == NULL)
                return -1;
        }
        size_t disp = (size_t)(off & (PAGE_SIZE - 1));
        int cap = MIN3((size_t)len, PAGE_SIZE - disp, (size_t)(ino->length - off));
        if (cap == 0)
            return bytes;
        memcpy(map + disp, buf, cap);
        len -= cap;
        off += cap;
        buf += cap;
        bytes += cap;
    }
    kunmap(map, PAGE_SIZE);
    return bytes;
}

void block_destroy(inode_t *ino)
{
    char tmp[16];
    block_file_t *block = ino->fl_data;
    // TODO - Check bbtree is clean !!
    block_page_t *page = bbtree_first(&block->tree, block_page_t, node);
    while (page) {
        if (page->dirty)
            kprintf(-1, "Error: dirty page haven't been sync %s\n", vfs_inokey(ino, tmp));
        if (page->rcu != 0)
            kprintf(-1, "Error: page is still mapped\n", vfs_inokey(ino, tmp));
        kprintf(KL_BIO, "Release page %p for inode %s at %llx\n", page->phys, vfs_inokey(ino, tmp), (xoff_t)page->node.value_ * PAGE_SIZE);
        page_release(page->phys);
        bbtree_remove(&block->tree, page->node.value_);
        kfree(page);
        page = bbtree_first(&block->tree, block_page_t, node);
    }

    kfree(block);
}


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

fl_ops_t block_ops = {
    .read = block_read,
    .write = block_write,
    .fetch = block_fetch,
    .release = block_release,
    .destroy = block_destroy,
};

block_file_t *block_create()
{
    block_file_t *block = kalloc(sizeof(block_file_t));
    splock_init(&block->lock);
    bbtree_init(&block->tree);
    return block;
}
