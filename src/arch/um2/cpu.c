#include <kernel/core.h>
#include <time.h>

void cpu_awake()
{
    kprintf(KLOG_MSG, "CPU: %s, Single CPU\n", "HostSimulatedCPU");
}


void cpu_elapsed() {}

int cpu_no()
{
    return 0;
}

void cpu_run() {}
void cpu_setup_signal() {}
void cpu_setup_task() {}
void cpu_task_return_uspace() {}

time_t cpu_time()
{
    return time(NULL);
}

void cpu_reboot()
{
}



void cpu_save_task(task_t *task)
{

}

