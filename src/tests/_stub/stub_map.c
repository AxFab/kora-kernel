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
#include <kernel/vfs.h>
#include <kernel/device.h>


typedef struct map_page map_page_t;
typedef struct map_cache map_cache_t;

struct map_page {
    bbnode_t bnode;
    atomic32_t usage;
    page_t phys;
    bool dirty;
};

struct map_cache {
    inode_t *ino;
    size_t block;
    int(*read)(inode_t *, const *data, size_t, off_t);
    int(*write)(inode_t *, const void *data, size_t, off_t);
    bbtree_t tree;
    splock_t lock;
 };

map_cache_t *map_create(inode_t *ino, void *read, void *write);
void map_destroy(map_cache_t *cache);

page_t map_fetch(map_cache_t *cache, off_t off);
void map_sync(map_cache_t *cache, off_t off, page_t pg);
void map_release(map_cache_t *cache, off_t off, page_t pg);



map_cache_t *map_create(inode_t *ino, void *read, void *write)
{
    map_cache_t *cache = kalloc(sizeof(map_cache_t));
    cache->read = read;
    cache->write = write;
    cache->ino = ino;
    cache->block = ino->und.dev->block;
    splock_init(&cache->lock);
    bbtree_init(&cache->tree);
    return cache;
}

void map_destroy(map_cache_t *cache)
{
    assert(cache->tree.count_ == 0);
    kfree(cache);
}


page_t map_fetch(map_cache_t *cache, off_t off)
{
    assert(IS_ALIGNED(off, PAGE_SIZE));
    void *ptr = kmap(PAGE_SIZE, NULL, 0, VMA_PHYSIQ);
    cache->read(cache->ino, ptr, PAGE_SIZE, off);
    return (void*)ptr;
}

void map_sync(map_cache_t *cache, off_t off, page_t pg)
{
    assert(IS_ALIGNED(off, PAGE_SIZE));
    void *ptr = (void*)pg;
    cache->write(cache->ino, ptr, PAGE_SIZE, off);
}

void map_release(map_cache_t *cache, off_t off, page_t pg)
{
    assert(IS_ALIGNED(off, PAGE_SIZE));
    kunmap((void*)pg, PAGE_SIZE);
}

