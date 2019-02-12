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
#include <kernel/core.h>
#include <kernel/task.h>
#include <threads.h>

task_t *anchor = NULL;

splock_t task_lock;
llhead_t task_list;

void scheduler_add(task_t *task)
{
    splock_lock(&task_lock);
    ll_enqueue(&task_list, &task->node);
    task->status = TS_READY;
    // kprintf(-1, "Scheduler add #%d\n", task->pid);
    splock_unlock(&task_lock);
}

void scheduler_rm(task_t *task, int status)
{
    splock_lock(&task_lock);
    ll_remove(&task_list, &task->node);
    // kprintf(-1, "Scheduler remove #%d\n", task->pid);
    task->status = status;
    splock_unlock(&task_lock);
}

// void scheduler_ticks()
// {
//     // TODO -- Count quanta -- time update...
//     scheduler_next();
// }

task_t *scheduler_next()
{
    splock_lock(&task_lock);
    task_t *curs;
    task_t *task = NULL;
    for ll_each(&task_list, curs, task_t, node) {
        if (curs->pid == cpu_no()) {
            task = curs;
            ll_remove(&task_list, &task->node);
            break;
        }
    }

    if (task != NULL) {
        task->status = TS_RUNNING;
        // kprintf(-1, "Scheduler next #%d\n", task->pid);
    }
    splock_unlock(&task_lock);
    return task;
}


__thread bool can_quit = false;
/* */
void scheduler_switch(int status, int retcode)
{
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 1000000;
    for (;;) {
        assert(kCPU.irq_semaphore == 1 || kCPU.irq_semaphore == 0);
        assert(status >= TS_ZOMBIE && status <= TS_READY);
        irq_reset(false);
        task_t *task = kCPU.running;
        if (task) {
            // kprintf(-1, "Leaving Task %d\n", task->pid);
            splock_lock(&task->lock);
            task->retcode = retcode;
            if (setjmp(task->state.jbuf) != 0)
                return;
            // kprintf(-1, "Saved Task %d\n", task->pid);

            // TODO Stop task chrono
            if (task->status == TS_ABORTED) {
                if (status == TS_BLOCKED) {
                    // TODO - We have advent structure to clean
                }
                status = TS_ZOMBIE;
            }
            if (status == TS_ZOMBIE) {
                /* Quit the task */
                async_raise(&task->wlist, 0);
                // task_zombie(task);
            } else if (status == TS_READY)
                scheduler_add(task);
            task->status = status;
            splock_unlock(&task->lock);
        }

        task = scheduler_next();
        kCPU.running = task;
        if (task == NULL) {
            task = task_search(cpu_no());
            if (can_quit && (task == NULL || task->status == TS_ZOMBIE)) {
                if (task)
                    task_close(task);
                thrd_exit(0);
            }
            task_close(task);
            nanosleep(&ts, NULL);
            kCPU.running = NULL;
            irq_disable();
            continue;
        }
        // TODO Start task chrono!
        if (task->usmem)
            mmu_context(task->usmem);
        // kprintf(-1, "Start Task %d\n", task->pid)

        if (task->pid == cpu_no()) {
            can_quit = true;
            longjmp(task->state.jbuf, 1);
        }

        nanosleep(&ts, NULL);
        splock_lock(&task->lock);
        task->status = TS_READY;
        scheduler_add(task);
        splock_unlock(&task->lock);
        kCPU.running = NULL;
        irq_disable();
    }
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


extern __thread int __cpu_no;

int new_cpu_thread(task_t *task)
{
    __cpu_no = task->pid;
    kCPU.running = NULL;
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 1000000;
    int val = setjmp(task->state.jbuf);
    switch (val) {
    case 0:
        for (;;) {
            nanosleep(&ts, NULL);
            irq_enter(0);
        }
    case 1:
        task->state.entry(task->state.param);
        task_stop(task, 0);
        break;
    }
    return 0;
}


void cpu_stack(task_t *task, size_t entry, size_t param)
{
    thrd_t thr;
    task->state.entry = (entry_t)entry;
    task->state.param = param;
    thrd_create(&thr, (thrd_start_t)new_cpu_thread, task);
}
