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
#include <kernel/core.h>
#include <kernel/vfs.h>
#include <kernel/device.h>
#include <kernel/files.h>
#include <threads.h>

struct map_page {
    bbnode_t bnode;
    int rcu;
    page_t phys;
    bool dirty;
    cnd_t cond;
};

struct map_cache {
    inode_t *ino;
    size_t block;
    int(*read)(inode_t *, char *data, size_t, off_t);
    int(*write)(inode_t *, const char *data, size_t, off_t);
    bbtree_t tree;
    splock_t lock;
};


map_cache_t *map_create(inode_t *ino, void *read, void *write)
{
    map_cache_t *cache = kalloc(sizeof(map_cache_t));
    cache->read = read;
    cache->write = write;
    cache->ino = ino;
    cache->block = ino->dev->block;
    splock_init(&cache->lock);
    bbtree_init(&cache->tree);
    return cache;
}

void map_destroy(map_cache_t *cache)
{
    // assert(cache->tree.count_ == 0);
    kfree(cache);
}

static void map_close(map_cache_t *cache, map_page_t *page)
{
    if (--page->rcu == 0) {
        // IF DIRTY SYNC
        // TODO push on LRU
        // kSYS.map_pages_lru
    }
}

/*
int map_scavenge(int count, int min)
{
    int del = 0;
    while (count-- > 0 && kSYS.map_pages_lru.count_ > min) {

        map_page_t *page = map_pages_lru.first;
        map_cache_t *cache;
        while (splock_trylock(&cache->lock) != 0) {
            // TODO -- don't touch MIN lasts items!
            page = llnext(&page->node, map_cache_t, node);
        }

        ++del;
        // TODO pop from on LRU
        // kSYS.map_pages_lru
        bbtree_remove(&cache->tree, page->bnode.value_);
        page_release(page->phys);
        splock_unlock(&cache->lock);
   }
   return del;
} */



page_t map_fetch(map_cache_t *cache, off_t off)
{
    // kprintf(-1, "FETCH page: %p, n%d\033[0m\n", cache->ino, off / PAGE_SIZE);
    if (cache->ino->length != 0 && off > cache->ino->length)
        kprintf(-1, "Warning - fetching page %x outside of block inode!\n", off);

    assert(kCPU.irq_semaphore == 0);
    assert(IS_ALIGNED(off, PAGE_SIZE));
    splock_lock(&cache->lock);
    map_page_t *page = bbtree_search_eq(&cache->tree, off / PAGE_SIZE, map_page_t, bnode);
    if (page != NULL) {
        // TODO - ensure not in LRU
        page->rcu++;
        splock_unlock(&cache->lock);
        // cnd_wait(&page->cond, NULL); SLEEP UNTIL PHYS == NULL
        return page->phys;
    }

    page = kalloc(sizeof(map_page_t));
    cnd_init(&page->cond);
    page->rcu = 1;
    page->bnode.value_ = off / PAGE_SIZE;
    bbtree_insert(&cache->tree, &page->bnode);
    splock_unlock(&cache->lock);
    assert(kCPU.irq_semaphore == 0);

    void *ptr = kmap(PAGE_SIZE, NULL, 0, VMA_PHYSIQ);
    assert(kCPU.irq_semaphore == 0);
    if (cache->read(cache->ino, ptr, PAGE_SIZE, off) != 0) {
        kprintf(-1, "\033[35mError while reading page: %p, n%d\033[0m\n", cache->ino, off / PAGE_SIZE);
        // TODO -- Handle bad page or retry  !?
    }
    assert(kCPU.irq_semaphore == 0);
    page_t pg = mmu_read((size_t)ptr);
    assert(pg != 0);
    kunmap(ptr, PAGE_SIZE);
    page->phys = pg;
    cnd_broadcast(&page->cond);
    return pg;
}

void map_sync(map_cache_t *cache, off_t off, page_t pg)
{
    assert(kCPU.irq_semaphore == 0);
    assert(IS_ALIGNED(off, PAGE_SIZE));
    splock_lock(&cache->lock);
    map_page_t *page = bbtree_search_eq(&cache->tree, off / PAGE_SIZE, map_page_t, bnode);
    assert(page != NULL);
    if (!page->dirty) {
        splock_unlock(&cache->lock);
        return;
    }

    assert(pg == page->phys);
    // TODO - async io, using CoW !?
    splock_unlock(&cache->lock);

    void *ptr = kmap(PAGE_SIZE, NULL, (off_t)pg, VMA_PHYSIQ);
    assert(kCPU.irq_semaphore == 0);
    cache->write(cache->ino, ptr, PAGE_SIZE, off);
    assert(kCPU.irq_semaphore == 0);
    kunmap(ptr, PAGE_SIZE);
}

void map_release(map_cache_t *cache, off_t off, page_t pg)
{
    // kprintf(-1, "RELEASE page: %p, n%d\033[0m\n", cache->ino, off / PAGE_SIZE);
    assert(IS_ALIGNED(off, PAGE_SIZE));
    splock_lock(&cache->lock);
    map_page_t *page = bbtree_search_eq(&cache->tree, off / PAGE_SIZE, map_page_t, bnode);
    assert(page != NULL);
    map_close(cache, page);
    splock_unlock(&cache->lock);
}

