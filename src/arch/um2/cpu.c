#include <kernel/core.h>
#include <time.h>

void cpu_awake()
{
    kprintf(KLOG_MSG, "CPU: %s, Single CPU\n", "HostSimulatedCPU");
}


void cpu_elapsed() {}
void cpu_halt() {}
void cpu_no() {}
void cpu_run() {}
void cpu_setup_signal() {}
void cpu_setup_task() {}
void cpu_task_return_uspace() {}

time_t cpu_time()
{
    return time(NULL);
}
