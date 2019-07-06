#include <kernel/core.h>
#include <kernel/task.h>

void exec_kloader()
{
    kprintf(-1, "New kernel loader\n");
    for (;;)
        sys_sleep(SEC_TO_KTIME(1));
}

void exec_init()
{
    kprintf(-1, "First process\n");
    for (;;)
        sys_sleep(SEC_TO_KTIME(1));
}

void exec_process(proc_start_t *info)
{
    kprintf(-1, "New process\n");
    for (;;)
        sys_sleep(SEC_TO_KTIME(1));
}

void exec_thread(task_start_t *info)
{
    kprintf(-1, "New thread\n");
    for (;;)
        sys_sleep(SEC_TO_KTIME(1));
}
