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
#include <kernel/task.h>
#include <kora/llist.h>
#include <kora/splock.h>
#include <kora/rwlock.h>
#include <errno.h>

typedef struct advent advent_t;
typedef struct emitter emitter_t;

splock_t async_lock;
llhead_t async_queue = INIT_LLHEAD;
time64_t async_timeout = INT64_MAX;

struct emitter {
	llhead_t list;
	// TODO - Add lock as soon as we dont required global lock.
};

struct advent {
	emitter_t *emitter;
    time64_t timeout;
    llnode_t em_node;
    llnode_t qu_node;
    task_t *task;
    int err;
};

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

static advent_t *async_advent(emitter_t *emitter, long timeout_us)
{
	/* Create an advent structure to hold event-comimg info */
	advent_t *advent = (advent_t*)kalloc(sizeof(advent_t));
	advent->task = kCPU.running;
	advent->task->advent = advent;
	advent->timeout = timeout_us >= 0 ? time64() + timeout_us: INT64_MAX;
	advent->emitter = emitter;

	/* We use a system-wide lock for waiting queues */
	splock_lock(&async_lock);
	if (async_timeout > advent->timeout)
	    async_timeout = advent->timeout;
	if (emitter != NULL)
	    ll_enqueue(&emitter->list, &advent->em_node);
	ll_enqueue(&async_queue, &advent->qu_node);
	irq_disable();
	splock_unlock(&async_lock);
	return advent;
}

static void async_cancel(advent_t *advent, int err)
{
	assert(splock_locked(&async_lock));
	advent->err = err;
	if (advent->emitter != NULL)
	    ll_remove(&emitter->list, &advent->em_node);
	ll_remove(&async_queue, &advent->qu_node);
	advent->task->advent = NULL;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

/* Wait for an event to be emited */
int async_wait(splock_t *lock, emitter_t *emitter, long timeout_us)
{
	assert(kCPU.running != NULL);
	assert(kCPU.irq_semaphore == (lock == NULL ? 0 : 1));
	assert((lock == NULL) == (emitter == NULL));
	assert(emitter != NULL || timeout_us > 0);

	advent_t *advent = async_advent(emitter, timeout_us);
	if (lock != NULL)
	    splock_unlock(lock);
	/* We have no locks but IRQs are still off, we can switch task */
	task_switch(TS_BLOCKED, 0);

	/* We have been rescheduled */
	errno = advent->err;
	kfree(advent);
	if (lock != NULL)
	    splock_lock(lock);
	return lock == NULL || errno == 0 ? 0 : -1;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void async_cancel(task_t *task)
{
	splock_lock(&async_lock);
	async_cancel(task->advent, 0);
	splock_unlock(&async_lock);
}

void async_raise(emitter_t *emitter, int err)
{
	advent_t *advent;
	advent_t *next;
	/* We use a system-wide lock for waiting queues */
	splock_t(&async_lock);
	advent = ll_first(&emitter->list, advent_t, em_node);
	while (advent) {
		next = ll_next(&advent->em_node, advent_t, em_node);
		/* Wakeup task */
		async_cancel(advent, err);
		task_resume(advent->task);
		advent = next;
	}
	splock_unlock(&async_lock);
}


void async_timeout()
{
	advent_t *advent;
	advent_t *next;
	time64_t later = INT64_MAX;
	time64_t now = time64();
	if (now < async_next)
	    return;
	if (!splock_trylock(&async_lock))
	    return;

	advent = ll_first(&async_queue, advent_t, qu_node);
	while (advent) {
		next = ll_next(&advent->qu_node, advent_t, qu_node);
		if (now >= advent->timeout) {
		    /* Wakeup task */
		    async_cancel(advent, err);
		    task_resume(advent->task);
		} else if (later > advent->timeout) {
			later = advent->timeout;
		}
		advent = next;
	}
	async_timeout = later;
	splock_unlock(&async_lock);
}

