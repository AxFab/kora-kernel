/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2018  <Fabien Bavent>
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
#include <kora/bbtree.h>
#include <kora/llist.h>
#include <kora/splock.h>
#include <kora/mcrs.h>
#include <string.h>
#include <threads.h>
#include <errno.h>

#if 0
typedef struct map_ops map_ops_t;
typedef struct map_cache map_cache_t;
typedef struct map_page map_page_t;
typedef page_t (*map_fetch_t)(inode_t *ino, off_t offset);
typedef void (*map_sync_t)(inode_t *ino, off_t offset, page_t page);
typedef void (*map_release_t)(inode_t *ino, off_t offset, page_t page);

struct map_ops {
    map_fetch_t fetch;
    map_sync_t sync;
    map_release_t release;
};

struct map_cache {
    bbtree_t tree;
    splock_t lock;
    inode_t *ino;
    map_ops_t *ops;
};

struct map_page {
    bbnode_t node;
    atomic32_t rcu;
    llnode_t lru;
    page_t phys;
    cnd_t ready;
    bool dirty;
};

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

map_page_t *map_create_(map_cache_t *cache, size_t no)
{
    map_page_t *page = kalloc(sizeof(map_page_t));
    page->node.value_ = no;
    cnd_init(&page->ready);
    bbtree_insert(&cache->tree, &page->node);
    splock_unlock(&cache->lock);
    assert(kCPU.irq_semaphore == 0);
    page->phys = cache->ops->fetch(cache->ino, no);
    cnd_broadcast(&page->ready);
    splock_lock(&cache->lock);
    return page;
}

void map_sync_(map_cache_t *cache, map_page_t *page)
{

}

void map_close_(map_cache_t *cache, map_page_t *page)
{
    assert(page != NULL);
    if (atomic_xadd(&page->rcu, -1) > 1)
        return;
    ll_append(&cache->tree, &page->lru);
    splock_unlock(&cache->lock);
}

void map_clean_(map_cache_t *cache, map_page_t *page)
{
    bbtree_remove(&cache->tree, &page->node);
    cache->ops->release(cache->ino, page->node.value_ * PAGE_SIZE, page->phys);
    kfree(page);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

map_cache_t *map_create(inode_t *ino)
{
    // assert(ino->map_ops != NULL);
    map_cache_t *cache = kalloc(sizeof(map_cache_t));
    bbtree_init(&cache->tree);
    splock_init(&cache->lock);
    cache->ino = ino;
    cache->ops = NULL;//ino->map_ops;
    return cache;
}

void map_destroy(map_cache_t *cache)
{
    map_page_t *page = bbtree_first(&cache->tree, map_page_t, node);
    while (page) {

        if (page->dirty)
            map_sync_(cache, page);
        map_page_t *last = page;
        page = bbtree_next(&page->node, map_page_t, node);
        map_clean_(cache, last);
    }
    kfree(cache);
}

page_t map_fetch(map_cache_t *cache, off_t offset)
{
    size_t no = offset / PAGE_SIZE;
    assert(IS_ALIGNED(offset, PAGE_SIZE));
    splock_lock(&cache->lock);
    map_page_t *page = bbtree_search_eq(&cache->tree, no, map_page_t, node);
    if (page == NULL)
        page = map_create_(cache, no);
    else if (page->rcu == 0)
        ll_remove(&cache->tree, &page->lru);
    cnd_wait(&page->ready, NULL);
    if (page->phys == 0) {
        map_close(cache, offset, 0);
        splock_unlock(&cache->lock);
        return 0;
    }
    atomic_inc(&page->rcu);
    splock_unlock(&cache->lock);
    return page->phys;
}

void map_close(map_cache_t *cache, off_t offset, bool dirty)
{
    size_t no = offset / PAGE_SIZE;
    assert(IS_ALIGNED(offset, PAGE_SIZE));
    splock_lock(&cache->lock);
    map_page_t *page = bbtree_search_eq(&cache->tree, no, map_page_t, node);
    if (dirty)
        page->dirty = true;
    map_close_(cache, page);
}

void map_sync(map_cache_t *cache)
{
    map_page_t *page = NULL;
    for (;;) {
        splock_lock(&cache->lock);
        if (page == NULL)
            page = bbtree_first(&cache->tree, map_page_t, node);
        else
            map_close_(cache, page);
        while (page != NULL && !page->dirty)
            page = bbtree_next(&page->node, map_page_t, node);
        if (page == NULL)
            return;
        if (page->rcu == 0)
            ll_remove(&cache->tree, &page->lru);
        atomic_inc(&page->rcu);
        splock_unlock(&cache->lock);
        map_sync_(cache, page);
    }
}

#endif
