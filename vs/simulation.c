/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2018  <Fabien Bavent>
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
#include <windows.h>

task_t *anchor = NULL;

splock_t task_lock;
llhead_t task_list;

void scheduler_add(task_t *task)
{
    splock_lock(&task_lock);
    ll_enqueue(&task_list, &task->node);
    splock_unlock(&task_lock);
}

void scheduler_rm(task_t *task)
{
    splock_lock(&task_lock);
    ll_remove(&task_list, &task->node);
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
    if (task == NULL)
        task = ll_dequeue(&task_list, task_t, node);
    splock_unlock(&task_lock);
    return task;
}

/* */
void scheduler_switch(int status, int retcode)
{
    for (;;) {
        assert(kCPU.irq_semaphore == 1);
        assert(status >= TS_ZOMBIE && status <= TS_READY);
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
        irq_reset(false);
        if (task == NULL) {
            Sleep(2);
            kCPU.running = NULL;
            irq_disable();
            continue;
        }
        // TODO Start task chrono!
        if (task->usmem)
            mmu_context(task->usmem);
        // kprintf(-1, "Start Task %d\n", task->pid)

        if (task->pid == cpu_no())
            longjmp(task->state.jbuf, 1);

        Sleep(2);
        splock_lock(&task->lock);
        task->status = TS_READY;
        scheduler_add(task);
        splock_unlock(&task->lock);
        kCPU.running = NULL;
        irq_disable();
    }
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


extern __declspec(thread) int __cpu_no;

DWORD WINAPI new_cpu_thread(LPVOID lpParameter)
{
    task_t *task = (task_t *)lpParameter;
    __cpu_no = task->pid;
    kCPU.running = NULL;

    int val = setjmp(task->state.jbuf);
    switch (val) {
    case 0:
        for (;;) {
            Sleep(10);
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
    task->state.entry = (entry_t)entry;
    task->state.param = param;
    DWORD myThreadId;
    HANDLE myHandle = CreateThread(0, 0, new_cpu_thread, task, 0, &myThreadId);
}
