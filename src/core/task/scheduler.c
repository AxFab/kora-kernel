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

