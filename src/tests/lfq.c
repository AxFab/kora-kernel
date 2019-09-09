
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


