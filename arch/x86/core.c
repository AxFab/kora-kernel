#include <stddef.h>

int _errno;

int *__errno_location()
{
    return &_errno;
}

void __assert_fail(const char *expr, const char *file, int line)
{
    for (;;);
}

void irq_enable() {}
void irq_disable() {}
void irq_reset() {}

void kmap(size_t addr, size_t len, void *ino, size_t off, unsigned flags)
{
}

void kunmap(size_t addr, size_t len)
{
}


void clock_elapsed() {}


int cpu_no() {}
void cpu_halt() {}
void cpu_save() {}
void cpu_restore() {}



void mmu_context() {}
void cpu_tss() {}

struct kSys kSYS;
