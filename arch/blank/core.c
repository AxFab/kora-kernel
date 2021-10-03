/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2019  <Fabien Bavent>
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
#include <stddef.h>
#include <stdarg.h>
#include <assert.h>
#include <stdatomic.h>
#include <kernel/arch.h>
#include <kernel/utils.h>


/* - */
int write(int fd, const char *buf, size_t len);


typedef struct kCpu kCpu_t;
typedef struct kSys kSys_t;

struct kCpu {
    int irq_sem;
    int err_no;
    task_t *running;
    int state;
    utime_t elapsed[STATE_ID_MAX];
    utime_t chrono;
};

struct kSys {
    utime_t clocks[CLOCK_ID_MAX];
    splock_t syslog_lock;
    splock_t sched_lock;
    unsigned syslog_bits;
    kCpu_t *cpus[64];
};

kSys_t kSYS;
#define kCPU  (*(kSYS.cpus[cpu_no()]))



/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int *__errno_location()
{
    return &kCPU.err_no;
}

void __assert_fail(const char *expr, const char *file, int line)
{
    char buf[CPU_STATE_STR];
    kprintf(-1, "\033[91m;Assertion <%s> %s: on %s:%d\033[0m;\n",
            cpu_rdstate(buf), expr, file, line);
    if (kCPU.running)
        scheduler_switch(TASK_ZOMBIE, -1);
    splock_lock(&kSYS.sched_lock);
    for (;;);
}


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

bool irq_enable()
{
    assert(kCPU.irq_sem > 0);
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
    assert(kCPU.irq_sem > 0);
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

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

#define KL_ERR  0
#define KLOG_WRN  1
#define KLOG_NFO  2
#define KL_DBG  3

int kprintf(int log, const char *msg, ...)
{
    if (log >= 0 && log < 32 && (kSYS.syslog_bits | (1 << log)))
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
    ret = vprintf(msg, ap);
    // ret = _PRT(vfprintf)(&fp, msg, ap);
    va_end(ap);
    splock_unlock(&kSYS.syslog_lock);
    return ret;
}


void *kmap(size_t len, void *ino, size_t off, unsigned flags)
{
}

void kunmap(void *addr, size_t leng)
{
}


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

__thread int __cpu_no;
atomic_int __cpu_nb = 0;

int cpu_no()
{
    return __cpu_no;
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
    snprintf(buf, 16, "Cpu.%d.%s-%c.%d", cpu_no(), __cpu_states[kCPU.state], task ? 'T' : 'K', pid);
    return buf;
}


void cpu_state(int state)
{
    char p[16];
    char n[16];
    kCpu_t *cpu = &kCPU;
    utime_t now = cpu_clock(CLOCK_MONOTONIC);

    cpu_rdstate(p);
    cpu->elapsed[cpu->state] += cpu->chrono - now;
    cpu->state = state;
    cpu->chrono = now;
    cpu_rdstate(n);
    kprintf(-1, "CPU state <%s> --> <%s>\n", p, n);
}


void cpu_prepare(task_t *task/* cpu_save_t *buf */)
{
    // Set SP, IP, TSS, MMU
    // struct cpu_save { size_t bx, si, di, bp, sp, eip, tss, cr3; };
}

void cpu_halt()
{
    // No tasks, so we sleep until next ticks
    // reset TSS, CR3 use cpu stack!
}

int cpu_save(/* cpu_save_t *buf */)
{
    // Save SP, IP, TSS, MMU + ctx_reg(float/sse)

    // i386: EBX, ESI, EDI, EBP, ESP, EIP, TSS, CR3
}

void cpu_restore(/* cpu_save_t *buf */)
{
    // Restore SP, IP, TSS, MMU + ctx_reg(float/sse)

    // i386: EBX, ESI, EDI, EBP, ESP, EIP, TSS, CR3
}

void cpu_setup()
{
    utime_t now = cpu_clock(CLOCK_MONOTONIC);
    __cpu_no = atomic_xadd(&__cpu_nb, 1);
    kCpu_t *cpu = malloc(sizeof(kCpu_t));
    memset(cpu, 0, sizeof(kCpu_t));
    cpu->chrono = now;
    kSYS.cpus[cpu_no()] = cpu;
    cpu_state(CPUSTATE_SYSTEM);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


void mmu_setup()
{
    size_t *pgd = malloc(sizeof(size_t) * 128);
    pgd[63] = 0x5000 | 6;
    pgd[0] = 0x6000 | 5;

}




/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int kwrite(FILE *fp, const char *buf, size_t len)
{
    return write(1, buf, len);
}

void clock_elapsed() {}

void mmu_context() {}
void cpu_tss() {}


int main()
{
    memset(&kSYS, 0, sizeof(kSYS));
    cpu_setup();
    mmu_setup();


    return 0;
}
