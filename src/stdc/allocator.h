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
#ifndef _KORA_ALLOCATOR_H
#define _KORA_ALLOCATOR_H 1

#include <stddef.h>
#include <kora/splock.h>
#include <kora/llist.h>
#include <kora/mcrs.h>

#define HEAP_PARANO  (1 << 0)
#define HEAP_CHECK  (1 << 1)
#define HEAP_CORRUPTED  (1 << 2)
#define HEAP_ARENA  (1 << 3)
#define HEAP_MAPPED  (1 << 4)
#define HEAP_OPTIONS  (HEAP_PARANO | HEAP_CHECK)

typedef struct heap_arena heap_arena_t;
typedef struct heap_chunk heap_chunk_t;

struct heap_arena {
    size_t address_;
    size_t length_;
    size_t used_;
    size_t max_chunk;
    int flags_;
    splock_t lock_;
    heap_chunk_t *free_;
    llnode_t node_;
};


/* Initialize a memory area to serve as allocator arena */
void setup_arena(heap_arena_t *arena, size_t address, size_t length,
                 size_t max, int option);
/* Allocate a block of memory into an arena */
void *malloc_r(heap_arena_t *arena, size_t size);
/* Release a block of memory previously allocated on the same arena */
void free_r(heap_arena_t *arena, void *ptr);

/* Dynamicaly allocate a block of memory on the address space */
void *_PRT(malloc)(size_t size);
/* Dynamicaly allocate a page align memory block */
void *_PRT(valloc)(size_t size);
/* Dynamicaly allocate a page align memory block */
void *_PRT(calloc)(size_t nmemb, size_t size);
/* Re-allocate a block memory */
void *_PRT(realloc)(void *ptr, size_t size);
/* Release a block of memory previously allocated dynamicaly */
void _PRT(free)(void *ptr);
/*  */
void setup_allocator(void *address, size_t length);
void sweep_allocator();



#endif /* _KORA_ALLOCATOR_H */
