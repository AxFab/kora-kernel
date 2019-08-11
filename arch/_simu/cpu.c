#include <stdatomic.h>
#include <threads.h>
#include <setjmp.h>
#include <kernel/core.h>
#include <kernel/cpu.h>
#include <kernel/task.h>
#include "opts.h"

atomic_int __cpu_inc = 0;
thread_local int __cpu_no = 0;
void new_cpu();

int cpu_no()
{
    return __cpu_no;
}

void usleep(long us);

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

#include <fcntl.h>
#include <kora/syscalls.h>
int hostfs_setup();
extern jmp_buf cpu_jbuf;

void cpu_stack(task_t *task, size_t entry, size_t param)
{
    hostfs_setup();

    task->state[0] = open("master.strace", O_RDONLY);
    task->state[1] = param;
    task->state[2] = 0;

    assert(task->state[0] != -1);
}


int cpu_save(cpu_state_t state)
{
    kprintf(-1, "cpu_save()\n");
    return 1;
}


_Noreturn void cpu_restore(cpu_state_t state)
{
    int pid = kCPU.running->pid;

    char line[256];
    int idx = -1;
    int fd = state[0];
    do {
        idx = -1;
        do {
            if (read(fd, &line[++idx], 1) != 1)
                longjmp(cpu_jbuf, 1);
        } while (line[idx] != '\n');
    } while (line[0] == '#' || idx == 0);
    line[idx] = '\0';

    char sysname[16];
    sscanf(line, "%s (", sysname);

    int i;
    scall_entry_t *sy = NULL;
    scall_entry_t *sc = syscall_entries;
    for (i = 0; i < 64; ++i, ++sc) {
        if (sc->name != NULL && strcmp(sysname, sc->name) == 0) {
            sy = sc;
            break;
        }
    }

    if (sy != NULL)
        sy->txt_call(line);
    else
        kprintf(-1, "\033[91mNo syscall named '%s'\033[0m\n", sysname);
    longjmp(cpu_jbuf, 2);
}

_Noreturn void cpu_halt()
{
    usleep(MSEC_TO_USEC(50));
    longjmp(cpu_jbuf, 1);
}


_Noreturn void cpu_usermode(size_t entry, size_t param)
{
    kprintf(-1, "cpu_usermode() - Not implemented\n");
    for (;;);
}

void cpu_tss(task_t *task)
{
    /* ERROR i386 Must be part of save/restore or usermode! */
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

#include <time.h>

// int64_t cpu_clock(int no)
// {
//     struct timespec sp;
//     clock_gettime(CLOCK_MONOTONIC, &sp);
//     return sp.tv_sec * 1000000LL + sp.tv_nsec / 1000;
// }

time_t cpu_time()
{
    return time(NULL);
}
