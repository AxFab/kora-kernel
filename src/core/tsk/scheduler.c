#include <kernel/core.h>
#include <kernel/task.h>

task_t *anchor = NULL;

void scheduler_add(task_t *item)
{
    if (anchor == NULL) {
        item->next = item;
        item->prev = item;
    } else {
        item->prev = anchor;
        item->next = anchor->next;
        anchor->next->prev = item;
        anchor->next = item;
    }
    anchor = item;
}

void scheduler_rm(task_t *item)
{
    if (item == kCPU.running) {
        kCPU.running = NULL;
    }
    item->prev->next = item->next;
    item->next->prev = item->prev;
    if (item == item->next) {
        anchor = NULL;
        kCPU.running = NULL;
    } else if (anchor == item) {
        anchor = item->next;
    }
    item->next = NULL;
    item->prev = NULL;
}

void scheduler_ticks()
{
    // TODO -- Count quanta -- time update...
    scheduler_next();
}

_Noreturn void scheduler_next()
{
    if (kCPU.running == NULL) {
        kCPU.running = anchor;
    } else {
        kCPU.running = kCPU.running->next;
    }

    if (kCPU.running == NULL) {
        cpu_halt();
    }

    kCPU.running->other_elapsed += cpu_elapsed(&kCPU.running->last);
    // TODO - We need to change TSS of the CPU in order to switch back to the correct kernel-stack!
    // kprintf(KLOG_DBG, "[SCH ] Run task pid=%d\n", kCPU.running->pid);
    task_signals();
    task_leave_sys();
    cpu_run(kCPU.running);
}

