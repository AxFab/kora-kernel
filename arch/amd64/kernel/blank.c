#include <kernel/types.h>

int cpu_no()
{
    return 0;
}

void cpu_setup()
{

}

void cpu_save() {}
void cpu_restore() {}
void cpu_stack() {}
void cpu_halt() {}
int64_t cpu_clock(int no)
{
    return 0;
}

void cpu_time() {}
void cpu_usermode() {}

page_t mmu_read(size_t addr)
{

}

page_t mmu_drop(size_t addr)
{

}

int mmu_protect(size_t addr, int flags)
{

}

int mmu_resolve(size_t addr, page_t pg, int flags)
{

}

void mmu_leave() {}
void mmu_enable() {}
void mmu_context() {}
void mmu_create_uspace() {}
void mmu_destroy_uspace() {}

void irq_ack(int no)
{

}

void platform_setup()
{

}

void kwrite(const char *buf, int lg)
{
}

int abs(int v)
{
    return v > 0 ? v : -v;
}

int slog;

void _start()
{

}
