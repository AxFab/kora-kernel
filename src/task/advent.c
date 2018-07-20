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
#include <kernel/core.h>
#include <kernel/cpu.h>
#include <kernel/task.h>
#include <kora/llist.h>
#include <errno.h>

typedef struct advent advent_t;

struct advent {
    llhead_t *list;
    time64_t timeout;
    llnode_t t_node;
    llnode_t l_node;
    task_t *task;
    splock_t *lock;
    int err;
};

llhead_t wlist = INIT_LLHEAD;
splock_t wlock = INIT_SPLOCK;
time64_t wtime = INT64_MAX;


static void advent_wakeup(advent_t *advent, int err)
{
    ll_remove(&wlist, &advent->t_node);
    if (advent->lock) {
        // WARNING RISK OF DEADLOCK !!!
        splock_lock(advent->lock);
        if (advent->list)
            ll_remove(advent->list, &advent->l_node);
        splock_unlock(advent->lock);
    }
    task_resume(advent->task);
}


void advent_awake(llhead_t *list, int err)
{
    // WARNING RISK OF DEADLOCK !!!
    splock_lock(&wlock);
    advent_t *advent = ll_first(&wlist, advent_t, l_node);
    advent_t *next;
    while (advent) {
        next = ll_next(&advent->l_node, advent_t, l_node);
        // advent_wakeup(advent, err); // CAN'T - DEADLOCK!
        // advent->err = err;
        advent = next;
    }
    splock_unlock(&wlock);
}

int advent_wait(splock_t *lock, llhead_t *list, long timeout_us)
{
    assert(kCPU.running != NULL);
    advent_t *advent = (advent_t *)kalloc(sizeof(advent_t));
    advent->lock = lock;
    advent->list = list;
    advent->task = kCPU.running;
    if (timeout_us > 0)
        advent->timeout = time64() + timeout_us;
    else
        advent->timeout = INT64_MAX;

    irq_disable();
    // WARNING RISK OF DEADLOCK !!!
    splock_lock(&wlock);
    ll_append(&wlist, &advent->t_node);
    if (wtime > advent->timeout)
        wtime = advent->timeout;
    if (list)
        ll_append(list, &advent->l_node);
    splock_unlock(&wlock);
    if (lock)
        splock_unlock(lock);
    /* Will put task into sleep mode and return here after! */
    task_switch(TS_BLOCKED, 0);
    int err = advent->err;
    kfree(advent);
    if (lock)
        splock_lock(lock);
    return err;
}

int advent_wait_rd(rwlock_t *lock, llhead_t *list, long timeout_us)
{
    assert(kCPU.running != NULL);
    advent_t *advent = (advent_t *)kalloc(sizeof(advent_t));
    // advent->lock = lock;
    advent->list = list;
    advent->task = kCPU.running;
    if (timeout_us > 0)
        advent->timeout = time64() + timeout_us;
    else
        advent->timeout = INT64_MAX;

    irq_disable();
    // WARNING RISK OF DEADLOCK !!!
    if (lock)
        rwlock_upgrade(lock);
    splock_lock(&wlock);
    ll_append(&wlist, &advent->t_node);
    if (wtime > advent->timeout)
        wtime = advent->timeout;
    if (list)
        ll_append(list, &advent->l_node);
    splock_unlock(&wlock);
    if (lock)
        rwlock_rdunlock(lock);
    /* Will put task into sleep mode and return here after! */
    task_switch(TS_BLOCKED, 0);
    kfree(advent);
    if (lock)
        rwlock_rdlock(lock);
    return advent->err;
}


void advent_timeout()
{
    time64_t now = time64();
    if (now < wtime)
        return;
    if (!splock_trylock(&wlock))
        return;

    advent_t *advent = ll_first(&wlist, advent_t, t_node);
    advent_t *next;
    time64_t later = INT64_MAX;
    while (advent) {
        next = ll_next(&advent->t_node, advent_t, t_node);
        if (now >= advent->timeout) {
            /* Wakup task and mark as timeout */
            advent_wakeup(advent, EAGAIN);
        } else if (later > advent->timeout)
            later = advent->timeout;
        advent = next;
    }

    wtime = later;
    splock_unlock(&wlock);
}

