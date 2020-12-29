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
#include <kernel/stdc.h>
#include <kernel/vfs.h>
#include <kora/splock.h>
#include <kora/llist.h>
#include <kora/bbtree.h>
#include <string.h>
#include <time.h>
#include <kernel/tasks.h>
#include <kernel/input.h>
#include <kernel/irq.h>

typedef struct ftx ftx_t;

struct advent {
    llnode_t tm_node;
    xtime_t until;
    unsigned repeat;
    bool did_timeout;
    task_t *task;

    void *object;
    llnode_t node;

    void (*wake)(masterclock_t *, advent_t *);
    void (*dtor)(masterclock_t *, advent_t *);
};

struct ftx {
    splock_t lock;
    llhead_t queue;
    bbnode_t bnode;
    int *pointer;
    int rcu;
    int flags;
};

masterclock_t __clock;

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
extern int __irq_semaphore;

void advent_register(masterclock_t *clock, advent_t *advent, long timeout, long interval)
{
    if (timeout > 0) {
        advent->task = __current;
        // ll_append(&advent.task->alist, &advent.anode);
        advent->repeat = MAX(0, interval);
        advent->until = clock->monotonic + timeout;
        if (advent->until < clock->next_advent)
            clock->next_advent = advent->until;
        splock_lock(&clock->lock);
        ll_append(&clock->list, &advent->tm_node);
        splock_unlock(&clock->lock);
    } else
        advent->until = 0;
}

void advent_unregister(masterclock_t *clock, advent_t *advent)
{
    if (advent->until != 0) {
        // assert(splock_locked(&clock->lock));
        ll_remove(&clock->list, &advent->tm_node);
        advent->until = 0;
    }
}

void advent_pause_task(masterclock_t *clock, advent_t *advent, long timeout)
{
    assert(__irq_semaphore == 0);
    advent_register(clock, advent, MAX(1, timeout), 0);
    scheduler_switch(TS_BLOCKED);
    assert(__irq_semaphore == 0);
}

void advent_resume_task(masterclock_t *clock, advent_t *advent)
{
    advent_unregister(clock, advent);
    scheduler_add(&__scheduler, advent->task);
    // ll_remove(&advent->task->alist, &advent->anode);
}

//void advent_cleanup(masterclock_t* clock, task_t* task)
//{
//ll_remove(&advent->task->alist, &advent->anode);
//}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

static ftx_t *futex_open(masterclock_t *clock, int *addr, int flags)
{
    size_t phys = (size_t)addr; // TODO resolve physical address
    splock_lock(&clock->tree_lock);
    ftx_t *futex = bbtree_search_eq(&clock->tree, phys, ftx_t, bnode);
    if (futex == NULL && (flags & FUTEX_CREATE)) {
        futex = (ftx_t *)kalloc(sizeof(ftx_t));
        memset(futex, 0, sizeof(ftx_t));
        futex->flags = flags;
        if (flags & FUTEX_SHARED) {
            // futex->pointer = ADDR_OFF(kmap(PAGE_SIZE, NULL, ALIGN_DOWN(phys), VMA_PHYSIQ), phys & (PAGE_SIZE-1));
        } else
            futex->pointer = addr;
        futex->bnode.value_ = phys;
        bbtree_insert(&clock->tree, &futex->bnode);
        splock_init(&futex->lock);
        llist_init(&futex->queue);
    }
    if (futex != NULL)
        futex->rcu++;
    splock_unlock(&clock->tree_lock);
    return futex;
}

static void futex_close(masterclock_t *clock, ftx_t *futex)
{
    splock_lock(&clock->tree_lock);
    if (--futex->rcu != 0) {
        splock_unlock(&futex->lock);
        splock_unlock(&clock->tree_lock);
        return;
    }

    assert(futex->queue.count_ == 0);
    bbtree_remove(&clock->tree, futex->bnode.value_);
    if (futex->flags & FUTEX_SHARED) {
        // kunmap(PAGE_SIZE, ALIGN_DOWN(futex->pointer, PAGE_SIZE));
    }

    splock_unlock(&futex->lock);
    kfree(futex);
    splock_unlock(&clock->tree_lock);
}

static void futex_wake_advent(masterclock_t *clock, advent_t *advent)
{
    ftx_t *futex = advent->object;
    // splock_lock(&futex->lock);
    /* wake up the thread */
    ll_remove(&futex->queue, &advent->node);
    advent_resume_task(clock, advent);
    // splock_unlock(&futex->lock);
}

static void futex_dtor_advent(masterclock_t *clock, advent_t *advent)
{
    ftx_t *futex = advent->object;
    splock_lock(&futex->lock);
    // assert(advent.task->advent == NULL);
    // assert(advent.node.next_ == NULL && advent.node.prev_ == NULL);
    futex_close(clock, futex);
}

int futex_wait(int *addr, int val, long timeout, int flags)
{
    ftx_t *futex = futex_open(&__clock, addr, flags | FUTEX_CREATE);
    advent_t advent;
    memset(&advent, 0, sizeof(advent));
    advent.object = futex;
    advent.wake = futex_wake_advent;
    advent.dtor = futex_dtor_advent;

    splock_lock(&futex->lock);
    if (*futex->pointer == val) {
        ll_enqueue(&futex->queue, &advent.node);
        splock_unlock(&futex->lock);
        advent_pause_task(&__clock, &advent, timeout);
        __asm_mb;
        futex = advent.object;
        splock_lock(&futex->lock);
    }

    // assert(advent.task->advent == NULL);
    assert(advent.node.next_ == NULL && advent.node.prev_ == NULL);
    futex_close(&__clock, futex);
    return 0;
}

int futex_requeue(int *addr, int val, int val2, int *addr2, int flags)
{
    ftx_t *origin = futex_open(&__clock, addr, 0);
    if (origin == NULL)
        return 0;
    splock_lock(&origin->lock);
    advent_t *it = ll_first(&origin->queue, advent_t, node);
    while (it && val-- > 0) {
        advent_t *advent = it;
        it = ll_next(&it->node, advent_t, node);
        assert(advent->object == origin);
        futex_wake_advent(&__clock, advent);
    }

    if (val2 == 0 || addr2 == NULL) {
        futex_close(&__clock, origin);
        return 0;
    }

    ftx_t *target = futex_open(&__clock, addr2, flags | FUTEX_CREATE);
    splock_lock(&target->lock);
    while (it && val2-- > 0) {
        advent_t *advent = it;
        it = ll_next(&it->node, advent_t, node);
        /* move to next futex */
        assert(advent->object == origin);
        ll_remove(&origin->queue, &advent->node);
        ll_enqueue(&target->queue, &advent->node);
        origin->rcu--;
        target->rcu++;
        advent->object = target;
    }

    futex_close(&__clock, target);
    futex_close(&__clock, origin);
    return 0;
}

int futex_wake(int *addr, int val)
{
    return futex_requeue(addr, val, 0, NULL, 0);
}


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

static void itimer_wake_advent(masterclock_t *clock, advent_t *advent)
{
    evmsg_t msg = { 0 };
    msg.message = GFX_EV_TIMER;
    inode_t *ino = advent->object;
    vfs_write(ino, (void *)&msg, sizeof(msg), 0, IO_ATOMIC | IO_NOBLOCK);
}

static void itimer_dtor_advent(masterclock_t *clock, advent_t *advent)
{
    advent_unregister(clock, advent);
    kfree(advent);
}

void itimer_create(inode_t *ino, long delay, long interval)
{
    advent_t *advent = kalloc(sizeof(advent_t));
    advent->object = ino;
    advent->wake = itimer_wake_advent;
    advent->dtor = itimer_dtor_advent;

    // advent->task = kCPU.running;
    // ll_append(&advent->task->alist, &advent->anode);

    advent_register(&__clock, advent, delay, interval);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

xtime_t sleep_timer(long timeout)
{
    advent_t advent;
    memset(&advent, 0, sizeof(advent));
    advent.wake = advent_resume_task;
    advent.dtor = NULL;
    xtime_t until = __clock.monotonic + timeout;
    advent_pause_task(&__clock, &advent, timeout);
    __asm_mb;
    return advent.did_timeout ? 0 : until - __clock.monotonic;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void clock_handler ();

masterclock_t *clock_init(int irq, xtime_t now)
{
    // masterclock_t*clock = kalloc(sizeof(masterclock_t));
    splock_init(&__clock.lock);
    splock_init(&__clock.tree_lock);
    bbtree_init(&__clock.tree);
    llist_init(&__clock.list);
    __clock.wall = now;

    irq_register(irq, (irq_handler_t)clock_handler, &__clock);
    return &__clock;
}

void clock_adjtime(masterclock_t *clock, xtime_t now)
{
    xtime_t diff = now - clock->wall;
    clock->wall = now;
    clock->boot += diff;
    // TODO -- Use adjustement algorithm
}

xtime_t clock_read(masterclock_t *clock, xtime_name_t name)
{ // TODO -- Replace by xtime_read
    if (name == XTIME_CLOCK)
        return clock->wall;
    return 0;
}

void clock_ticks(masterclock_t *clock, unsigned elapsed)
{
    clock->wall += elapsed;
    clock->boot += elapsed;
    clock->monotonic += elapsed;

    if (clock->next_advent <= clock->monotonic) {
        splock_lock(&clock->lock);
        advent_t *it = ll_first(&clock->list, advent_t, tm_node);
        while (it) {
            advent_t *advent = it;
            it = ll_next(&it->tm_node, advent_t, tm_node);
            if (advent->until == 0 || advent->wake == NULL)
                continue;
            if (advent->until < clock->monotonic) {
                advent->did_timeout = true;
                advent->wake(clock, advent);
                if (advent->repeat > 0) {
                    advent->until += advent->repeat;
                    if (advent->until < clock->next_advent)
                        clock->next_advent = advent->until;
                }
            } else if (advent->until < clock->next_advent)
                clock->next_advent = advent->until;
        }
        splock_unlock(&clock->lock);
    }
}


void clock_handler(masterclock_t *clock)
{
    clock_ticks(clock, 10000);
    scheduler_switch(TS_READY);
}
