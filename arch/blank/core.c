#include <stddef.h>
#include <kora/splock.h>
#include <kernel/arch.h>
#include <bits/libio.h>


#define CLOCK_ID_MAX  2
#define CLOCK_MONOTONIC  0
#define CLOCK_REALTIME  1
#define CLOCK_LOWLATENCY 2


#define STATE_ID_MAX  5
#define CPUSTATE_IDLE  0
#define CPUSTATE_USER  1
#define CPUSTATE_SYSTEM  2
#define CPUSTATE_IRQ  3
#define CPU_STATE_STR  32

#define TASK_ZOMBIE  0
#define TASK_BLOCKED  1
#define TASK_SLEEPING  2
#define TASK_WAITING  3
#define TASK_RUNNING  4

typedef void * task_t;
typedef signed long long utime_t;

typedef struct kCpu kCpu_t;
struct kCpu {
    int irq_sem;
    int err_no;
    task_t *running;
    int state;
    utime_t elapsed[STATE_ID_MAX];
    utime_t chrono;
} kCPU;

struct {
    utime_t clocks[CLOCK_ID_MAX];
    splock_t syslog_lock;
    splock_t sched_lock;

} kSYS;


char *cpu_rdstate(char *buf);
int kprintf(int log, const char *msg, ...);


int *__errno_location()
{
    return &kCPU.err_no;
}

void __assert_fail(const char *expr, const char *file, int line)
{
    char buf[CPU_STATE_STR];
    task_t *task = kCPU.running;
    kprintf(-1, "\033[91m;Assertion <%s> %s: on %s:%d\033[0m;\n",
            cpu_rdstate(buf), expr, file, line);
    splock_lock(&kSYS.syslog_lock);
    // kwrite(-1, buf, len);
    splock_unlock(&kSYS.syslog_lock);
    if (kCPU.running)
        scheduler_switch(TASK_ZOMBIE, -1);
    splock_lock(&kSYS.sched_lock);
    for (;;);
}

bool irq_enable()
{
    if (--kCPU.irq_sem == 0) {
        IRQ_ON;
        return true;
    }
    return false;
}

void irq_disable()
{
    if (kCPU.irq_sem++ == 0)
        IRQ_OFF;
}

void irq_reset(bool enable) 
{
    if (enable) {
        kCPU.irq_sem = 0;
        IRQ_ON;
    } else {
        kCPU.irq_sem = 1;
        IRQ_OFF;
    }
}

void kmap(size_t addr, size_t len, void *ino, size_t off, unsigned flags)
{
}

void kunmap(size_t addr, size_t len)
{
}



utime_t cpu_clock(int clock_id)
{
    return kSYS.clocks[clock_id % CLOCK_ID_MAX];
}

const char *__cpu_states[] = {
    "idle", "user", "sys", "irq",
};

char *cpu_rdstate(char *buf)
{
    task_t *task = kCPU.running;
    int pid = 0; // task ? task->pid : 0;
    snprintf(buf, 12, "Cpu.%d.%s-%c.%d", cpu_no(), __cpu_states[kCPU.state], task ? 'T' : 'K', pid);
    return buf;
}


void cpu_state(int state)
{
    kCpu_t *cpu = &kCPU;
    utime_t now = cpu_clock(CLOCK_MONOTONIC);

    cpu->elapsed[cpu->state] += cpu->chrono - now;
    cpu->state = state;
    cpu->chrono = now;
}

void clock_read() {}
void clock_elapsed() {}


int cpu_no() {}
void cpu_halt() {}
void cpu_save() {}
void cpu_restore() {}



void mmu_context() {}
void cpu_tss() {}

int kwrite(FILE *fp, const char *buf, int len);

#define KLOG_MEM  1
#define KLOG_INO  2


int kprintf(int log, const char *msg, ...)
{
    if (/*(log == KLOG_DBG && no_dbg) || */log == KLOG_MEM || log == KLOG_INO)
        return 0;;
    int ret;
    va_list ap;
    FILE fp;

    fp.fd_ = log;
    fp.lbuf_ = EOF;
    fp.write = kwrite;
    fp.lock_ = -1;

    splock_lock(&kSYS.syslog_lock);
    va_start(ap, msg);
    ret = vfprintf(&fp, msg, ap);
    va_end(ap);
    splock_unlock(&kSYS.syslog_lock);
    return ret;
}


int kwrite(FILE *fp, const char *buf, int len)
{
    return write(1, buf, len);
}


