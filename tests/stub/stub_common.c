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
#include <unistd.h>
#include <bits/atomic.h>
#include <bits/cdefs.h>
#include <kernel/core.h>
#include <kernel/arch.h>
#include <kernel/vfs.h>
#include <kernel/stdc.h>
#if defined(_WIN32)
#  include <Windows.h>
#endif



/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

#if !defined(_WIN32)
# define _valloc(s)  valloc(s)
# define _vfree(p)  free(p)
#else
# define _valloc(s)  _aligned_malloc(s, PAGE_SIZE)
# define _vfree(p)  _aligned_free(p)
#endif

int kallocCount = 0;
int kmapCount = 0;
//bool ktrack_init = false;
//bbtree_t ktrack_tree;
//splock_t ktrack_lock;
//struct ktrack
//{
//    void *ptr;
//    size_t len;
//    const char *msg;
//    bbnode_t bnode;
//};

// int kalloc_count()
// {
//     return kallocCount;
// }

// int kmap_count()
// {
//     return kmapCount;
// }

void *kalloc_(size_t len, const char *msg)
{
    //if (!ktrack_init) {
    //    bbtree_init(&ktrack_tree);
    //    splock_init(&ktrack_lock);
    //    ktrack_init = true;
    //}
    kallocCount++;
    void *ptr = calloc(len, 1);
    //splock_lock(&ktrack_lock);
    //struct ktrack *tr = malloc(sizeof(struct ktrack));
    //assert(tr != NULL);
    //tr->ptr = ptr;
    //tr->len = len;
    //tr->msg = msg;
    //tr->bnode.value_ = (size_t)ptr;
    //bbtree_insert(&ktrack_tree, &tr->bnode);
    //splock_unlock(&ktrack_lock);
    // kprintf(-1, "\033[96m+ alloc (%p, %d) %s\033[0m\n", ptr, len, msg);
    return ptr;
}

void kfree(void *ptr)
{
    kallocCount--;
    //splock_lock(&ktrack_lock);
    //struct ktrack *tr = bbtree_search_eq(&ktrack_tree, (size_t)ptr, struct ktrack, bnode);
    //assert(tr != NULL);
    //bbtree_remove(&ktrack_tree, (size_t)ptr);
    //splock_unlock(&ktrack_lock);
    //// kprintf(-1, "\033[96m- free (%p)\033[0m\n", ptr);
    free(ptr);
    //free(tr);
}

// void kalloc_check()
// {
//     splock_lock(&ktrack_lock);
//     struct ktrack *tr = bbtree_first(&ktrack_tree, struct ktrack, bnode);
//     while (tr) {
//         kprintf(-1, "- \033[31mMemory leak\033[0m at %p(%d) %s\n", tr->ptr, tr->len, tr->msg);
//         tr = bbtree_next(&tr->bnode, struct ktrack, bnode);
//     }

//     splock_unlock(&ktrack_lock);
// }

char *kstrdup(const char *str)
{
    int len = strlen(str) + 1;
    char *ptr = kalloc_(len, "strdup");
    strncpy(ptr, str, len);
    return ptr;
}

char *kstrndup(const char *str, size_t max)
{
    int len = strnlen(str, max) + 1;
    char *ptr = kalloc_(len, "strndup");
    strncpy(ptr, str, len);
    ptr[len] = '\0';
    return ptr;
}

int alloc_check()
{
    printf("Ending session: %d alloc, %d map\n", kallocCount, kmapCount);

    // struct ktrack *tr = bbtree_first(&ktrack_tree, struct ktrack, bnode);
    // while (tr) {
    //     printf("alloc leak: %p - %s\n", tr->ptr, tr->msg);
    //     tr = bbtree_next(&tr->bnode, struct ktrack, bnode);
    // }

    //map_page_t *mp = bbtree_first(&map_tree, map_page_t, node);
    //while (mp) {
    //    printf("kmap leak: %p - %o - %p\n", mp->ptr, mp->access, mp->ino);
    //    mp = bbtree_next(&mp->node, map_page_t, node);
    //}

    return kallocCount == 0 && kmapCount == 0 ? 0 : -1;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

splock_t kplock = INIT_SPLOCK;
char kpbuf[1024];
char kpbuf2[1024];
#define KL_NONE  ((char *)0xAC)

// Change kernel log settings
char *klog_lvl[KL_MAX] = {
    //[KL_FSA] = KL_NONE,
    [KL_FSA] = "\033[35m%s\033[0m",
    [KL_VMA] = "\033[93m%s\033[0m",
    [KL_BIO] = KL_NONE,
};


void kprintf(klog_t log, const char *msg, ...)
{
    char *pk = log >= 0 && log < KL_MAX ? klog_lvl[log] : NULL;
    if (pk == KL_NONE)
        return;
    splock_lock(&kplock);
    va_list ap;
    va_start(ap, msg);
    int lg = vsnprintf(kpbuf, 1024, msg, ap);
    va_end(ap);
    if (pk != NULL) {
        lg = snprintf(kpbuf2, 1024, pk, kpbuf);
        write(1, kpbuf2, lg);
    } else {
        write(1, kpbuf, lg);
    }
    splock_unlock(&kplock);
}

/* Return the current time in the 1-microsecond precision */
xtime_t xtime_read(xtime_name_t name)
{
#if defined(_WIN32)
    FILETIME tm;
    GetSystemTimePreciseAsFileTime(&tm); // 100-nanosecond
    uint64_t cl = (uint64_t)tm.dwHighDateTime << 32 | tm.dwLowDateTime;
    return cl / 10LL - SEC_TO_USEC(11644473600LL);
#else
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    xtime_t vl = TMSPEC_TO_USEC(now);
    return vl;
#endif
}

uint8_t rand8()
{
    return rand() & 0xff;
}

uint16_t rand16()
{
    return rand();
}

uint32_t rand32()
{
    return rand() | (rand() << 16);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

