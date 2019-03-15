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

task_t *anchor = NULL;

splock_t task_lock;
llhead_t task_list;

void scheduler_add(task_t *task)
{
    splock_lock(&task_lock);
    ll_enqueue(&task_list, &task->node);
    task->status = TS_READY;
    splock_unlock(&task_lock);
}

void scheduler_rm(task_t *task, int status)
{
    assert(status != TS_READY);
    splock_lock(&task_lock);
    ll_remove(&task_list, &task->node);
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
    task_t *task = ll_dequeue(&task_list, task_t, node);
    splock_unlock(&task_lock);
    return task;
}

/* */
void scheduler_switch(int status, int retcode)
{
    assert(kCPU.irq_semaphore == 1 || kCPU.irq_semaphore == 0);
    assert(status >= TS_ZOMBIE && status <= TS_READY);
    irq_reset(false);
    task_t *task = kCPU.running;
    if (task) {
        // kprintf(-1, "Leaving Task %d\n", task->pid);
        splock_lock(&task->lock);
        task->retcode = retcode;
        if (cpu_save(task->state) != 0)
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
        clock_elapsed(CPU_IDLE);
        cpu_halt();
    }
    // TODO Start task chrono!
    if (task->usmem)
        mmu_context(task->usmem);
    clock_elapsed(CPU_USER);
    // kprintf(-1, "-> Task %d\n", task->pid);
    cpu_restore(task->state);
}
