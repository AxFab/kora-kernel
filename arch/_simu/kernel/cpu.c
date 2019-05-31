#include <stdatomic.h>
#include <threads.h>
#include <setjmp.h>
#include <kernel/core.h>
#include <kernel/cpu.h>
#include <kernel/task.h>
#include "opts.h"

atomic_int __cpu_inc = 0;
__thread int __cpu_no = 0;
void new_cpu();

int cpu_no()
{
    return __cpu_no;
}

void cpu_setup()
{
    int i;
    __cpu_no = atomic_fetch_add(&__cpu_inc, 1);
    if (__cpu_no == 0) {
        thrd_t thrd;
        kSYS.cpus = kalloc(sizeof(struct kCpu) * opts.cpu_count);
        kprintf(-1, "CPU.%d BSP\n", cpu_no());
        for (i = 1; i < opts.cpu_count; ++i)
            thrd_create(&thrd, (thrd_start_t)new_cpu, NULL);
        usleep(MSEC_TO_USEC(10));
    }
    // strcpy(kCPU.vendor, opts.cpu_vendor);
}

void cpu_sweep()
{

}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void cpu_stack(task_t *task, size_t entry, size_t param)
{
    // HELLO !!!
    task->state[0] = entry;
    task->state[1] = param;
    task->state[2] = 0;
}

int cpu_save(cpu_state_t state)
{
    kprintf(-1, "cpu_restore() - Not implemented\n");
    for (;;);
}

#include <kora/syscalls.h>
int hostfs_setup();

_Noreturn void cpu_restore(cpu_state_t state)
{
    int pid = kCPU.running->pid;
    if (pid == 1) {
	// First load some drivers
	hostfs_setup();
	// Look for 'boot-device' and mount it as root
	irq_syscall(SYS_OPEN, -1, (size_t)"hdd1", 05, 0, 0);
	// vfs_search(kSYS.dev_ino, "hdd1");
        irq_syscall(SYS_MMAP, 0, 8192, 006, -1, 0);
    }
    kprintf(-1, "cpu_restore() - Not implemented\n");
    for (;;);
}

_Noreturn void cpu_halt()
{
    kprintf(-1, "cpu_halt() - Not implemented\n");
    for (;;);
}


_Noreturn void cpu_usermode(size_t entry, size_t param)
{

}

void cpu_tss()
{
    /* ERROR i386 Must be part of save/restore or usermode! */
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


int64_t cpu_clock(int no)
{
}

time_t cpu_time()
{
}
