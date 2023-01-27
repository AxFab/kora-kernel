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
#include <stdlib.h>
#include <stdio.h>
#include <bits/atomic.h>
#include <bits/cdefs.h>
#include <kernel/core.h>
#include <kernel/arch.h>
#include <kernel/vfs.h>
#include <kernel/stdc.h>
#if defined(_WIN32)
#  include <Windows.h>
#endif

typedef struct map_page map_page_t;
struct map_page {
    page_t pgs[16];
    char *ptr;
    size_t len;
    inode_t *ino;
    bbnode_t node;
    int access;
    xoff_t off;
    int usage;
};
bbtree_t map_tree;
bool map_init = false;

extern int kmapCount;

#if !defined(_WIN32)
# define _valloc(s)  valloc(s)
# define _vfree(p)  free(p)
#else
# define _valloc(s)  _aligned_malloc(s, PAGE_SIZE)
# define _vfree(p)  _aligned_free(p)
#endif


map_page_t *kmap_new(int access, void *ino, xoff_t off, size_t len, void *ptr)
{
    map_page_t *mp = malloc(sizeof(map_page_t));
    assert(mp != NULL);
    mp->usage = 1;
    mp->access = access;
    mp->ino = ino;
    mp->off = off;
    mp->len = len;
    mp->ptr = ptr;
    return mp;
}

map_page_t *kmap_search(void *ino, xoff_t off, size_t len, int mode)
{
    map_page_t *mp = bbtree_left(map_tree.root_, map_page_t, node);
    while (mp) {
        if (mp->ino == ino && mp->off == off && mp->len <= len) {
            mp->usage++;
            mp->access |= mode;
            return mp;
        }
        mp = bbtree_next(&mp->node, map_page_t, node);
    }

    return NULL;
}

// int kmap_check()
// {
//     //map_page_t *mp = bbtree_first(&map_tree, map_page_t, node);
//     //while (mp) {
//     //    printf("kmap leak: %p - %o - %p\n", mp->ptr, mp->access, mp->ino);
//     //    mp = bbtree_next(&mp->node, map_page_t, node);
//     //}
//     return kmapCount == 0 ? 0 : -1;
// }


void *kmap(size_t len, void *obj, xoff_t off, int flags)
{
    assert(len > 0);
    assert((len & (PAGE_SIZE - 1)) == 0);
    assert((off & (PAGE_SIZE - 1)) == 0);

    int type = flags & VMA_TYPE;
    int getter = 0;
    int access = flags & (VM_RW | VM_EX | VM_RESOLVE);
    if (type == 0 && obj != NULL)
        type = VMA_FILE;
    // assert((ino == NULL) != (type == VMA_FILE));
    switch (type) {
    case 0:
        type = VMA_ANON;
    case VMA_ANON:
        break;
    case VMA_PIPE:
        access = VM_RW;
        // data is pipe
        break;
    case VMA_STACK:
    case VMA_HEAP:
        access = VM_RW;
        break;
    case VMA_PHYS:
        getter = 4;
        access &= VM_RW;
        access |= VM_RESOLVE;
        break;
    case VMA_FILE:
        getter = 1;
        break;
    case VMA_CODE:
        access = VM_RD | VM_EX;
        getter = 2;
        break;
    }

    ++kmapCount;
    if (!map_init) {
        bbtree_init(&map_tree);
        map_init = true;
    }

    char tmp[16];
    map_page_t *mp = NULL;
    if (getter == 0) {
        mp = kmap_new(access | type, NULL, 0, len, _valloc(len));
    } else if (getter == 1) {
        assert(irq_ready());
        mp = kmap_search(obj, off, len, access & 7);
        if (mp != NULL)
            return mp->ptr;
        
        mp = kmap_new(access | type, obj, off, len, NULL);
        inode_t *ino = obj;
        assert(len == PAGE_SIZE);
        assert(ino != NULL);
        mp->ptr = (void *)vfs_fetch_page(ino, off);
        if (mp->ptr == NULL) {
            printf("Error on fetching page of %s\n", vfs_inokey(ino, tmp));
            return NULL;
        }

    } else if (getter == 4) {
        if (off == 0) {
            mp = kmap_new(access | type, NULL, 0, len, _valloc(len));
        } else {
            mp = kmap_search(obj, off, len, access & 7); // ino is null !?
            if (mp != NULL)
                return mp->ptr;
            mp = kmap_new(access | type, (void *)off, 0, len, (void*)off); // is off required?
        }
    } else { // if (getter == 3) {
        assert("No dlib supported" == NULL);
    }

    mp->node.value_ = (size_t)mp->ptr;
    bbtree_insert(&map_tree, &mp->node);
    return mp->ptr;
}

void kunmap(void *addr, size_t len)
{
    // assert(irq_ready());
    --kmapCount;
    map_page_t *mp = bbtree_search_eq(&map_tree, (size_t)addr, map_page_t, node);
    assert(mp != NULL);

    if (--mp->usage > 0)
        return;

    int getter = 0;
    switch (mp->access & VMA_TYPE) {
    case VMA_FILE:
        getter = 1;
        break;
    case VMA_CODE:
        getter = 2;
        break;
    case VMA_PHYS:
        getter = 4;
        break;
    }

    char tmp[16];
    bbtree_remove(&map_tree, (size_t)addr);
    if (getter == 0) {
        _vfree(addr);
    } else if (getter == 1) {
        bool dirty = mp->access & VM_WR ? true : false;
        size_t nx = 0;
        int ret = vfs_release_page(mp->ino, mp->off, (size_t)mp->ptr + nx, dirty);
        if (ret != 0) {
            printf("Error on releasing page of %s\n", vfs_inokey(mp->ino, tmp));
        }

    } else if (getter == 4) {

    } else { // if (getter == 3) {
        assert("No dlib supported" == NULL);
    }

    free(mp);
    // assert(irq_ready());
}

void page_release_kmap_stub(page_t page)
{
    _vfree((void *)page);
}

page_t mmu_read_kmap_stub(size_t address)
{
    if (!map_init) {
        bbtree_init(&map_tree);
        map_init = true;
    }
    map_page_t *mp = bbtree_search_le(&map_tree, address, map_page_t, node);
    if (mp == NULL || mp->node.value_ + mp->len < address)
        return 0;
    return (page_t)address;
}

const char *ksymbol(void *ip, char *buf, int lg)
{
    return NULL;
}
