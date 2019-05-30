#include <bits/cdefs.h>
#include <stdatomic.h>
#include <kernel/types.h>
#include <kernel/core.h>
#include <kernel/task.h>
#include "crtc.h"

struct kSys kSYS;

thread_local int __cpu_no = 0;
atomic_int __cpu_inc = 1;

void cpu_setup()
{
    __cpu_no = atomic_fetch_add(&__cpu_inc, 1);
    kCPU.running = (task_t *)calloc(sizeof(task_t), 1);
    kCPU.running->pid = __cpu_no;
}

void cpu_sweep()
{
    free(kCPU.running);
}

int cpu_no()
{
    return __cpu_no;
}

int *__errno_location()
{
    return &kCPU.err_no;
}

_Noreturn void __assert_fail(const char *expr, const char *file, int line)
{
    printf("Assertion - %s at %s:%d\n", expr, file, line);
    abort();
}


bool irq_enable();
bool irq_active = false;

void irq_reset(bool enable)
{
    irq_active = true;
    kCPU.irq_semaphore = 0;
    if (enable)
        IRQ_ON;
    else
        IRQ_OFF;
}

bool irq_enable()
{
    if (irq_active) {
        assert(kCPU.irq_semaphore > 0);
        if (--kCPU.irq_semaphore == 0) {
            IRQ_ON;
            return true;
        }
    }
    return false;
}

void irq_disable()
{
    if (irq_active) {
        IRQ_OFF;
        ++kCPU.irq_semaphore;
    }
}

void *kalloc(size_t len)
{
    return calloc(len, 1);
}

void kfree(void *ptr)
{
    free(ptr);
}

void *kmap(size_t len, inode_t *ino, off_t off, int flags)
{
    return valloc(len);
}

void kunmap(void *addr, size_t len)
{
    free(addr);
}

void kprintf(int log, const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    vprintf(msg, ap);
    va_end(ap);
}
