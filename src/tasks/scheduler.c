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
#include <kernel/stdc.h>
#include <kernel/tasks.h>
#include <kora/llist.h>
#include <kora/splock.h>
#include <sys/signum.h>

scheduler_t __scheduler;
task_t *__current = NULL;


void scheduler_init(/*fs_anchor_t *fsanchor, void *net*/)
{
    splock_init(&__scheduler.lock);
    splock_init(&__scheduler.sch_lock);
    bbtree_init(&__scheduler.task_tree);
    llist_init(&__scheduler.sch_queue);
    //__scheduler.net = net;
    //__scheduler.vfs = vfs;
}

void scheduler_add(scheduler_t *sch, task_t *task)
{
    splock_lock(&sch->sch_lock);
    ll_enqueue(&sch->sch_queue, &task->sch_node);
    task->status = TS_READY;
    if (task->start_time == 0)
        task->start_time = xtime_read(XTIME_CLOCK);
    splock_unlock(&sch->sch_lock);
}

void scheduler_rm(scheduler_t *sch, task_t *task, int status)
{
    assert(status != TS_READY);
    splock_lock(&sch->sch_lock);
    ll_remove(&sch->sch_queue, &task->sch_node);
    task->status = status;
    splock_unlock(&sch->sch_lock);
}

/* */
void scheduler_switch(int status)
{
    scheduler_t *sch = &__scheduler;
    assert(status >= TS_ZOMBIE && status <= TS_READY);
    irq_reset(false);

    // Save the current task
    task_t *task = __current;
    if (task) {
        if (cpu_save(&task->jmpbuf) != 0)
            return;

        // TODO -- Register elapsed time !
        splock_lock(&sch->sch_lock);
        if (task->status == TS_ABORTED || status == TS_ZOMBIE) {
            ll_enqueue(&sch->sch_zombie, &task->sch_node);
            task->status = TS_ZOMBIE;
            task_raise(task->parent, SIGCHLD);
        } else if (status == TS_READY) {
            ll_enqueue(&sch->sch_queue, &task->sch_node);
            task->status = TS_READY;
            // .. SYSCALL LOGS IS INTERRUPTED...
            if (task->sc_log[0] != '\0') {
                strncat(task->sc_log, " .....", 64 - strlen(task->sc_log));
                kprintf(-1, "Task.%d] \033[96m%s\033[0m\n", task->pid, task->sc_log);
                task->sc_log[0] = '\0';
            }
        } else {
            task->status = status;
        }
    } else {
        splock_lock(&sch->sch_lock);
    }

    // Restore next task
    task = ll_dequeue(&sch->sch_queue, task_t, sch_node);
    if (task)
        task->status = TS_RUNNING;
    splock_unlock(&sch->sch_lock);
    irq_zero();
    clock_state(task == NULL ? CKS_IDLE : CKS_SYS);
    __current = task;
#ifdef KORA_KRN
    if (task == NULL) {
        cpu_halt();
    } else {
        // mmu_context(task->vm);
        cpu_restore(&task->jmpbuf);
    }
#endif
}
EXPORT_SYMBOL(scheduler_switch, 0);
