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

typedef struct bio bio_t;
typedef struct bio_req bio_req_t;
typedef struct bio_algo bio_algo_t;

typedef struct block_file block_file_t;
typedef struct block_page block_page_t;

struct block_file {
    // Page cache
    bbtree_t tree;
    splock_t lock;
    llhead_t llru;

    bool async;
};

struct block_page {
    bool ready;
    bool dirty;
    bool in_ops;
    bbnode_t node;
    atomic_int rcu;
    size_t phys;
    mtx_t mtx;
    cnd_t cnd;
    llnode_t nlru;
};

size_t mmu_read(size_t address);

bio_t *bio_create(inode_t *ino);
void bio_request(bio_t *bio, block_page_t *page, size_t lba, size_t cnt, int flags);
void bio_push(bio_t *bio);
int bio_wait(bio_t *bio);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void block_scavenge(block_file_t *block, int max)
{
    splock_lock(&block->lock);
    while (max-- > 0) {
        block_page_t *page = ll_dequeue(&block->llru, block_page_t, nlru);
        if (page == NULL)
            break;
        int lba = page->node.value_;
        // TODO -- Race condition, is page_mutex released !?
        bbtree_remove(&block->tree, lba);
        page_release(page->phys);
        kfree(page);
    }
    splock_unlock(&block->lock);
}

static int block_fill(inode_t *ino, block_page_t *page)
{
    int ret;
    char tmp[20];
    might_sleep();

    mtx_lock(&page->mtx);
    if (page->ready && !page->in_ops) {
        mtx_unlock(&page->mtx);
        return 0;
    }
    
    while (page->in_ops) {
        // Wait for end of operaton
        cnd_wait(&page->cnd, &page->mtx);
    }

    if (page->ready) {
        mtx_unlock(&page->mtx);
        return 0;
    }

    block_file_t *block = ino->fl_data;
    if (block->async) {
        // Asynchronous read
        int bpp = PAGE_SIZE / ino->dev->block;
        size_t lba = page->node.value_ * bpp;
        bio_t *bio = bio_create(ino);
        bio_request(bio, page, lba, bpp, VM_RD);
        bio_push(bio);
        ret = bio_wait(bio);
    } else {
        // Synchronous read
        // page->phys = page_new(); TODO -- ISSUE ON CLI_VFS OR FOR DMA DRIVERS...
        assert(page->phys == 0);
        void *ptr = kmap(PAGE_SIZE, NULL, page->phys, VM_RW | VMA_PHYS);
        assert(ptr != NULL);
        page->phys = mmu_read((size_t)ptr);
        xoff_t off = page->node.value_ * PAGE_SIZE;
        kprintf(KL_BIO, "Alloc page %p for inode %s, read at %llx\n", page->phys, vfs_inokey(ino, tmp), off);
        ret = ino->ops->read(ino, ptr, PAGE_SIZE, off, 0);
        kunmap(ptr, PAGE_SIZE);
    }
    if (ret != 0) {
        kprintf(-1, "\033[35mError while reading page: %s, pg:%d\033[0m\n", vfs_inokey(ino, tmp), page->node.value_);
        mtx_unlock(&page->mtx);
        return -1;
    }
    page->ready = true;

    mtx_unlock(&page->mtx);
    return 0;
}

static int block_writeback(inode_t *ino, block_page_t *page)
{
    char tmp[20];
    block_file_t *block = ino->fl_data;
    if (block->async) {
        // Asynchronous write
        int bpp = PAGE_SIZE / ino->dev->block;
        size_t lba = page->node.value_ * bpp;
        page->in_ops = true;
        bio_t *bio = bio_create(ino);
        bio_request(bio, page, lba, bpp, VM_WR);
        bio_push(bio);
    } else {
        // Synchronous write
        void *ptr = kmap(PAGE_SIZE, NULL, (xoff_t)page->phys, VM_RW | VMA_PHYS);
        assert(ptr != NULL);
        xoff_t off = page->node.value_ * PAGE_SIZE;
        kprintf(KL_BIO, "Write back page %p for inode %s at %llx\n", page->phys, vfs_inokey(ino, tmp), off);

        int len = PAGE_SIZE;
        assert(off < ino->length);
        if (off + len > ino->length)
            len = ino->length - off;

        int ret = ino->ops->write(ino, ptr, len, off, 0);
        kunmap(ptr, PAGE_SIZE);
        if (ret != 0) {
            kprintf(-1, "\033[35mError while syncing page: %s, pg:%d\033[0m\n", vfs_inokey(ino, tmp), off / PAGE_SIZE);
            return -1;
        }

        page->dirty = false;
    }

    return 0;
}

static block_page_t *block_get(inode_t *ino, xoff_t off, bool create)
{
    assert(IS_ALIGNED(off, PAGE_SIZE));
    block_file_t *block = ino->fl_data;
    splock_lock(&block->lock);

    size_t lba = (size_t)(off / PAGE_SIZE);
    block_page_t *page = bbtree_search_eq(&block->tree, lba, block_page_t, node);
    if (page == NULL) {
        if (!create) {
            splock_unlock(&block->lock);
            return NULL;
        }
        page = kalloc(sizeof(block_page_t));
        mtx_init(&page->mtx, mtx_plain);
        cnd_init(&page->cnd);
        page->rcu = 0;
        page->node.value_ = lba;
        bbtree_insert(&block->tree, &page->node);
    } else if (ll_contains(&block->llru, &page->nlru))
        ll_remove(&block->llru, &page->nlru);
    // TODO -- Can be locked !?

    atomic_inc(&page->rcu);
    splock_unlock(&block->lock);
    return page;
}

static void block_rel(inode_t *ino, block_page_t *page)
{
    might_sleep();
    block_file_t *block = ino->fl_data;
    if (atomic_xadd(&page->rcu, -1) != 1)
        return;

    mtx_lock(&page->mtx);
    if (page->rcu == 0) {
        if (page->dirty) {
            // Try to sync
            block_writeback(ino, page);
            // TODO -- if write back might fail !?
        }

        if (!page->dirty) {
            // Put into LRU
            splock_lock(&block->lock);
            ll_enqueue(&block->llru, &page->nlru);
            splock_unlock(&block->lock);
        }
    }
    mtx_unlock(&page->mtx);
}


size_t block_fetch(inode_t *ino, xoff_t off, bool blocking)
{
    char tmp[20];
    // Check parameters
    if (off < 0 || off / PAGE_SIZE >= (xoff_t)__SSIZE_MAX) {
        kprintf(-1, "\033[35mSettings %lld / %lld / ...\033[0m\n", (xoff_t)PAGE_SIZE, (xoff_t)__SSIZE_MAX);
        kprintf(-1, "\033[35mError while reading page: %s, pg:%d. bad offset\033[0m\n", vfs_inokey(ino, tmp), off / PAGE_SIZE);
        return 0;
    }

    block_page_t *page = block_get(ino, off, blocking);
    if (page == NULL)
        return 0;

    int ret = -1;
    for (int i = 0; ret != 0 && i < 3; ++i)
        ret = block_fill(ino, page);
    
    if (ret != 0) {
        block_rel(ino, page);
        return -1; // Unable to get the page
    }

    return page->phys;
}

int block_release(inode_t *ino, xoff_t off, size_t pg, bool dirty)
{
    char tmp[20];
    // Check parameters
    if (off < 0 || off / PAGE_SIZE >= (xoff_t)__SSIZE_MAX) {
        kprintf(-1, "\033[35mError while syncing page: %s, pg:%d. bad offset\033[0m\n", vfs_inokey(ino, tmp), off / PAGE_SIZE);
        return -1;
    }

    block_page_t *page = block_get(ino, off, false);
    assert(page != NULL);
    if (dirty) {
        mtx_lock(&page->mtx);
        page->dirty = true;
        mtx_unlock(&page->mtx);
    }
    block_rel(ino, page);
    block_rel(ino, page);
    return 0;

}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int block_read(inode_t *ino, char *buf, size_t len, xoff_t off, int flags)
{
    // TODO -- Should we do only one big mapping !!?
    might_sleep();
    if (off > ino->length) {
        errno = EINVAL;
        return -1;
    }

    char *map = NULL;
    int bytes = 0;
    while (len > 0) {
        xoff_t po = ALIGN_DW(off, PAGE_SIZE);
        xoff_t mo = off - po;
        size_t cap = (size_t)MIN3((xoff_t)len, PAGE_SIZE - mo, ino->length - off);
        if (cap == 0)
            break;
        size_t page = block_fetch(ino, po, true);
        map = kmap(PAGE_SIZE, NULL, page, VM_RD | VMA_PHYS);
        memcpy(buf, &map[mo], cap);
        kunmap(map, PAGE_SIZE);
        block_release(ino, po, page, false);
        len -= cap;
        off += cap;
        buf += cap;
        bytes += cap;
    }

    return bytes;
}

int block_write(inode_t *ino, const char *buf, size_t len, xoff_t off, int flags)
{
    might_sleep();
    if (ino->length < off + (xoff_t)len) {
        int ret = vfs_truncate(ino, off + (xoff_t)len) != 0;
        if (ret != 0)
            return -1;
    }

    char *map = NULL;
    int bytes = 0;
    while (len > 0) {
        xoff_t po = ALIGN_DW(off, PAGE_SIZE);
        xoff_t mo = off - po;
        size_t cap = (size_t)MIN3((xoff_t)len, PAGE_SIZE - mo, ino->length - off);
        if (cap == 0)
            break;
        size_t page = block_fetch(ino, po, true);
        map = kmap(PAGE_SIZE, NULL, page, VM_RW | VMA_PHYS);
        memcpy(&map[mo], buf, cap);
        kunmap(map, PAGE_SIZE);
        block_release(ino, po, page, true); // TODO -- Can we mark only part of the page as dirty !?
        len -= cap;
        off += cap;
        buf += cap;
        bytes += cap;
    }

    assert(len == 0);
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
    .destroy = block_destroy,
};

block_file_t *block_create()
{
    block_file_t *block = kalloc(sizeof(block_file_t));
    splock_init(&block->lock);
    bbtree_init(&block->tree);
    block->async = false;
    return block;
}
