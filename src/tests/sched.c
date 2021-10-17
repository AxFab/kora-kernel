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
#include <kernel/task.h>
#include <kernel/core.h>
#include <kora/splock.h>

splock_t scheduler_lock;
llhead_t scheduler_list;

void usleep(long us);

void scheduler_add(task_t *task)
{
    splock_lock(&scheduler_lock);
    task->status = TS_READY;
    ll_append(&scheduler_list, &task->node);
    splock_unlock(&scheduler_lock);
}

void scheduler_rm(task_t *task, int status)
{
    splock_lock(&scheduler_lock);
    task->status = status;
    ll_remove(&scheduler_list, &task->node);
    splock_unlock(&scheduler_lock);
}

void scheduler_switch(int status, int code)
{
    if (status == TS_READY) {
        splock_lock(&scheduler_lock);
        ll_append(&scheduler_list, &kCPU.running->node);
        splock_unlock(&scheduler_lock);
    }
    for (;;) {
        splock_lock(&scheduler_lock);
        task_t *task;
        for ll_each(&scheduler_list, task, task_t, node) {
            if (task->pid == cpu_no()) {
                ll_remove(&scheduler_list, &task->node);
                splock_unlock(&scheduler_lock);
                return;
            }
        }
        splock_unlock(&scheduler_lock);
        usleep(MSEC_TO_USEC(50));
    }
}

void scheduler_init()
{
    splock_init(&scheduler_lock);
    llist_init(&scheduler_list);
}
