/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015  <Fabien Bavent>
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
#include <kernel/core.h>
#include <kernel/vfs.h>
#include <kernel/task.h>
#include <kora/bbtree.h>
#include <kora/llist.h>
#include <kora/rwlock.h>
#include <kora/mcrs.h>
#include <string.h>
#include <errno.h>

#define IO_TIMEOUT 10000 // 10ms

typedef struct blkpage blkpage_t;
typedef struct blkcache blkcache_t;

struct blkpage {
    bbnode_t bnode;
    atomic32_t usage;
    page_t phys;
    inode_t *ino;
    bool dirty;
};

struct blkcache {
    bbtree_t tree;
    rwlock_t lock;
    llhead_t wlist; /* Waiting list for readers */
};

/* Request sync to driver */
static int ioblk_sync_(blkpage_t *page)
{
    void *map = kmap(PAGE_SIZE, NULL, page->phys, VMA_PHYSIQ);
    int ret = vfs_write(page->ino, map, PAGE_SIZE, (off_t)page->bnode.value_);
    assert((ret == 0) != (errno != 0));
    kunmap(map, PAGE_SIZE);
    if (ret == 0)
        page->dirty = false;
    return ret;
}


/* Request fetch to driver */
static int ioblk_fetch_(blkpage_t *page)
{
    void *map = kmap(PAGE_SIZE, NULL, 0, VMA_PHYSIQ);
    int ret = vfs_read(page->ino, map, PAGE_SIZE, (off_t)page->bnode.value_);
    assert((ret == 0) != (errno != 0));
    kunmap(map, PAGE_SIZE);
    if (ret == 0)
        page->phys = mmu_read((size_t)map);
    return ret;
}

static void ioblk_close(blkcache_t *cache, blkpage_t *page, bool write_lock)
{
    /* Decrement RCU */
    if (atomic32_xadd(&page->usage, -1) != 1)
        return;

    if (!write_lock) {
        /* Grab lock and recheck */
        rwlock_upgrade(&cache->lock);
        if (page->usage != 0) {
            rwlock_wrunlock(&cache->lock);
            rwlock_rdlock(&cache->lock);
            return;
        }
    }

    /* Synchronize if needed */
    if (page->dirty) {
        rwlock_wrunlock(&cache->lock);
        ioblk_sync_(page);
        rwlock_wrlock(&cache->lock);
        // TODO - Still ok for trashing!?
    }

    bbtree_remove(&cache->tree, page->bnode.value_);
    rwlock_wrunlock(&cache->lock);
    page_release(page->phys);
    kfree(page);
}

static blkpage_t *ioblk_search(inode_t *ino, off_t off)
{
    /* Do some checks */
    assert(off >= 0 && IS_ALIGNED(off, PAGE_SIZE));
    assert(S_ISREG(ino->mode) || S_ISBLK(ino->mode));
    if (off >= ino->length) {
        errno = EINVAL;
        return NULL;
    }

    /* Look for a known page */
    blkcache_t *cache = (blkcache_t *)ino->object;
    assert(true/* cache is locked in rd or wr */);
    blkpage_t *page = bbtree_search_eq(&cache->tree, (size_t)off, blkpage_t, bnode);
    if (page == NULL)
        return NULL;
    // TODO REMOVE FROM LRU
    atomic_inc(&page->usage);
    errno = 0;
    return page;
}


void ioblk_init(inode_t *ino)
{
    blkcache_t *cache = (blkcache_t *)kalloc(sizeof(blkcache_t));
    rwlock_init(&cache->lock);
    bbtree_init(&cache->tree);
    ino->object = cache;
}

void ioblk_sweep(inode_t *ino)
{
    blkcache_t *cache = (blkcache_t *)ino->object;
    rwlock_wrlock(&cache->lock);
    // REMOVE ALL PAGES
    assert(cache->tree.count_ == 0);
    rwlock_wrunlock(&cache->lock);
    kfree(cache);
}

void ioblk_release(inode_t *ino, off_t off)
{
    blkcache_t *cache = (blkcache_t *)ino->object;
    rwlock_rdlock(&cache->lock);
    blkpage_t *page = ioblk_search(ino, off);
    assert(page != NULL);
    ioblk_close(cache, page, false);
    ioblk_close(cache, page, false);
    rwlock_rdunlock(&cache->lock);
}

/* Find the page mapping the content of a block inode */
page_t ioblk_page(inode_t *ino, off_t off)
{
    blkcache_t *cache = (blkcache_t *)ino->object;
    rwlock_rdlock(&cache->lock);
    blkpage_t *page = ioblk_search(ino, off);

    /* If the page is referenced return aonce ready */
    if (page != NULL) {
        if (page->phys
            || advent_wait_rd(&cache->lock, &cache->wlist, IO_TIMEOUT) == 0) {
            rwlock_rdunlock(&cache->lock);
            errno = 0;
            return page->phys;
        }
        assert(errno != 0);
        int err = errno;
        ioblk_close(cache, page, false);
        rwlock_rdunlock(&cache->lock);
        errno = err;
        return 0;
    }

    /* Reference the page */
    page = (blkpage_t *)kalloc(sizeof(blkpage_t));
    page->ino = ino;
    page->bnode.value_ = (size_t)off;
    ++page->usage;
    rwlock_upgrade(&cache->lock);
    bbtree_insert(&cache->tree, &page->bnode);
    rwlock_wrunlock(&cache->lock);

    /* Request fetch to driver */
    int ret = ioblk_fetch_(page);
    rwlock_wrlock(&cache->lock);
    advent_awake(&cache->wlist, errno);
    if (ret != 0)
        ioblk_close(cache, page, true);
    rwlock_wrunlock(&cache->lock);
    return page->phys;
}

/* Synchronize a page mapping the content of a block inode */
int ioblk_sync(inode_t *ino, off_t off)
{
    blkcache_t *cache = (blkcache_t *)ino->object;
    rwlock_rdlock(&cache->lock);
    blkpage_t *page = ioblk_search(ino, off);
    assert(page != NULL);
    /* Request sync to driver */
    rwlock_rdunlock(&cache->lock);
    int ret = ioblk_sync_(page);
    rwlock_rdlock(&cache->lock);
    ioblk_close(cache, page, false);
    rwlock_rdunlock(&cache->lock);
    return ret;
}

void ioblk_dirty(inode_t *ino, off_t off)
{
    blkcache_t *cache = (blkcache_t *)ino->object;
    rwlock_rdlock(&cache->lock);
    blkpage_t *page = ioblk_search(ino, off);
    assert(page != NULL);
    page->dirty = true;
    // PUSH ON DIRTY LIST
    ioblk_close(cache, page, false);
    rwlock_rdunlock(&cache->lock);
}

int ioblk_read(inode_t *ino, char *buf, int len, off_t off)
{
    if (off >= ino->length)
        return 0;
    off_t poff = -1;
    char *map = NULL;
    int bytes = 0;
    while (len > 0) {
        off_t po = ALIGN_DW(off, PAGE_SIZE);
        if (poff != po) {
            if (map != NULL)
                kunmap(map, PAGE_SIZE);
            poff = po;
            map = kmap(PAGE_SIZE, ino, poff, VMA_FILE_RO | VMA_RESOLVE);
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
        bytes += cap;
    }
    kunmap(map, PAGE_SIZE);
    return bytes;
}


int ioblk_write(inode_t *ino, const char *buf, int len, off_t off)
{
    if (off >= ino->length)
        return 0;
    off_t poff = -1;
    char *map = NULL;
    int bytes = 0;
    while (len > 0) {
        off_t po = ALIGN_DW(off, PAGE_SIZE);
        if (poff != po) {
            if (map != NULL)
                kunmap(map, PAGE_SIZE);
            poff = po;
            map = kmap(PAGE_SIZE, ino, poff, VMA_FILE_RO | VMA_RESOLVE);
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
        bytes += cap;
    }
    kunmap(map, PAGE_SIZE);
    return bytes;
}

