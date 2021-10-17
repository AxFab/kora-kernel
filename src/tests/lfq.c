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

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-= */

typedef volatile uint16_t atomic16_t;

static inline void atomic16_inc(atomic16_t *ptr)
{
    asm volatile("lock insw %0" : "=m"(*ptt));
}

// ...
//

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-= */
/* Lock-free queue */

typedef struct lfnode lfnode_t;
typedef struct lfqueue lfqueue_t;

struct lfnode {
    lfnode_t *next;
};

struct lfqueue {
    lfnode_t *head;
    lfnode_t *tail;
    lfnode_t anchor;
};

static inline int lfq_init(lfqueue_t *queue)
{
    queue->head = queue->tail = &queue->anchor;
    queue->anchor.next = NULL;
}

static inline bool lfq_empty(lfqueue_t *queue)
{
    return queue->tail == queue->head;
}

static inline void lfq_enqueue(lfqueue_t *queue, lfnode_t *node)
{
    node->next = NULL;
    for (;;) {
        lnode_t *t = queue->tail;
        if (atomic_cmpset(&t->next, NULL, node)) {
            atomic_cmpset(&queue->tail, t, node);
            return;
        }
    }
}


static inline lfnode_t *lfq_dequeue_(lfqueue_t queue)
{
    for (;;) {
        lfnode_t *top = queue->head;
        if (queue->tail == top)
            return NULL;
        lfnode_t *node = top->next;
        if (atomic_cmpset(&queue->head->next, node, node->next)) {
            atomic_cmpset(&queue->tail, node, &queue->anchor);
            node->next = NULL;
            return node;
        }
    }
}

#define lfq_dequeue(q,t,m)  (itemof((lfq_dequeue_(q)),t,m))


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-= */

#include <pthread.h>
#include <stdlib.h>
#include <time.h>


typedef struct n {
    lfnode_t node;
    unsigned long long value;
} nint;

lfqueue_t queue;

void unpredictable_delay()
{
    struct timespec dl;
    dl.tv_sec = 0;
    dl.tv_nsec = rand() & 0xfff;
    nanosleep(&dl, NULL);
}

void writer_thread()
{
