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
    task_t *task = ll_dequeue(&task_list, task_t, node);
    splock_unlock(&task_lock);
    return task;
}

