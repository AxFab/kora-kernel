#include <kernel/task.h>
#include <kernel/core.h>
#include <kora/splock.h>

splock_t scheduler_lock;
llhead_t scheduler_list;

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

void scheduler_switch(int state, int code)
{
    if (state == TS_READY) {
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
        futex_tick();
    } 
}

void scheduler_init() 
{
    splock_init(&scheduler_lock);
    llist_init(&scheduler_list);
}

