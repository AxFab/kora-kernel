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
#include <string.h>
#include <errno.h>
#include <kora/allocator.h>
#include <kora/mcrs.h>
#include <kernel/mmu.h>

static int __empty_arena = 0;
static int __arena_option = HEAP_CHECK;
static size_t __arena_size = (16 * _Mib_);
static size_t __arena_chunk_size_limit = (16 * _Kib_);
static llhead_t __arenas;
static heap_arena_t __firstArena;

static heap_arena_t *new_arena(size_t length)
{
    ++__empty_arena;
    heap_arena_t *arena = (heap_arena_t *)_PRT(malloc)(sizeof(heap_arena_t));
    void *map = heap_map(length);
    if (map == NULL) {
        __FAIL(ENOMEM, ""); /* TODO */
        return NULL;
    }
    setup_arena(arena, (size_t)map, length, __arena_chunk_size_limit,
                __arena_option);
    ll_append(&__arenas, &arena->node_);
    return arena;
}

static heap_arena_t *find_arena(size_t ptr)
{
    heap_arena_t *arena;
    // printf(" FREE - Look for 0x%zx\n", ptr);
    for ll_each(&__arenas, arena, heap_arena_t, node_) {
        // printf("   |-> Arean 0x%zx - 0x%zx\n", arena->address_, arena->address_ + arena->length_);
        if (arena->address_ > ptr || arena->address_ + arena->length_ <= ptr) {
            continue;
        }
        return arena;
    }
    return NULL;
}

/* Dynamicaly allocate a page align memory block */
void *_PRT(valloc)(size_t size)
{
    heap_arena_t *arena = (heap_arena_t *)_PRT(malloc)(sizeof(heap_arena_t));
    void *map = heap_map(ALIGN_UP(size, PAGE_SIZE));
    if (map == NULL) {
        __FAIL(ENOMEM, ""); /* TODO */
        return NULL;
    }
    arena->address_ = (size_t)map;
    arena->length_ = size;
    arena->flags_ = HEAP_MAPPED;
    ll_append(&__arenas, &arena->node_);
    /* TODO CREATE ARENA(HEAP_MAPPED) PUSH ON BBTREE */
    return map;
}

/* Dynamicaly allocate a page align memory block */
void *_PRT(pvalloc)(size_t size)
{
    return _PRT(valloc)(ALIGN_UP(size, PAGE_SIZE));
}

/* Dynamicaly allocate a block of memory on the address space */
void *_PRT(malloc)(size_t size)
{
    void *ptr = NULL;
    heap_arena_t *arena = NULL;

    if (size > __arena_chunk_size_limit) {
        return _PRT(valloc)(size);
    }

    for ll_each(&__arenas, arena, heap_arena_t, node_) {
        if (arena->length_ - arena->used_ < size) {
            continue;
        }

        if (arena->used_ == 0) {
            --__empty_arena;
        }

        ptr = malloc_r(arena, size);
        if (ptr != NULL) {
            break;
        }
    }

    if (ptr == NULL) {
        arena = new_arena(__arena_size);
        if (arena == NULL) {
            __FAIL(ENOMEM, "");
            return NULL;
        }
        ptr = malloc_r(arena, size);
    }

    if (__empty_arena == 0) {
        arena = new_arena(__arena_size);
        if (arena == NULL)
            __FAIL(ENOMEM, "");
        __empty_arena++;
    }

    return ptr;
}

/* Dynamicaly allocate a page align memory block */
void *_PRT(calloc)(size_t nmemb, size_t size)
{
    void *ptr;
    size *= nmemb;
    if (size > __arena_chunk_size_limit) {
        return _PRT(valloc)(size);
    }

    ptr = _PRT(malloc)(size);
    memset(ptr, 0, size);
    return ptr;
}

/* Re-allocate a block memory */
void *_PRT(realloc)(void *ptr, size_t size)
{
    size_t lg = 0; /* Get chunk size */
    void *buf;
    if (lg > size) {
        return ptr;
    }
    /* TODO try to grab next item! */
    buf = _PRT(malloc)(size);
    memcpy(buf, ptr, MIN(lg, size));
    _PRT(free)(ptr);
    return buf;
}

/* Release a block of memory previously allocated dynamicaly */
void _PRT(free)(void *ptr)
{
    heap_arena_t *arena = find_arena((size_t)ptr); /* TODO bbtree GET */
    if (arena == NULL) {
        __FAIL(-1, ""); /* TODO */
        return;
    }

    if (arena->flags_ & HEAP_MAPPED) {
        if ((size_t)ptr == arena->address_) {
            heap_unmap((void *)arena->address_, arena->length_);
        } else {
            __FAIL(-1, ""); /* TODO */
            return;
        }
        /* Remove from bbtree and free */
        // } else if ((size_t)ptr >= arena->address_ && (size_t)ptr < arena->address_ + arena->length_) {
    } else {
        free_r(arena, ptr);
        // __FAIL(-1, ""); /* TODO */
    }
}

/*  */
void setup_allocator(void *base, size_t len)
{
    __empty_arena = 0;
    memset(&__arenas, 0, sizeof(__arenas));
    memset(&__firstArena, 0, sizeof(__firstArena));
    setup_arena(&__firstArena, (size_t)base, len, __arena_chunk_size_limit,
                __arena_option);
    ll_append(&__arenas, &__firstArena.node_);
    __empty_arena = 1;
}

void sweep_allocator()
{
    heap_arena_t *arena = ll_first(&__arenas, heap_arena_t, node_);
    while (arena) {
        heap_arena_t *next = ll_next(&arena->node_, heap_arena_t, node_);
        if (arena->used_ == 0 && arena != &__firstArena) {
            heap_unmap((void *)arena->address_, arena->length_);
            _PRT(free)(arena);
            ll_remove(&__arenas, &arena->node_);
        }
        arena = next;
    }
}
