#include <bits/cdefs.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdatomic.h>
#include <kernel/types.h>
#include <kernel/core.h>
#include <kernel/task.h>

struct kSys kSYS;

thread_local __cpu_no = 0;
atomic_int __cpu_inc = 1;

void cpu_setup()
{
    __cpu_no = atomic_fetch_add(&__cpu_inc, 1);
    kCPU.running = (task_t*)calloc(sizeof(task_t), 1);
    kCPU.running->pid = 1;
}

int cpu_no()
{
    return __cpu_no;
}

int *__errno_location()
{
    return &kCPU.err_no;
}

void __assert_fail()
{
}

bool irq_enable() {}
void irq_disable() {}

void *kalloc(size_t len)
{
    return calloc(len, 1);
}

void kfree(void *ptr)
{
    free(ptr);
}

void *kmap()
{
}

void kunmap()
{
}

void kprintf(int log, const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
}

