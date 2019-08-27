#ifndef __KERNEL_RESX__CORE_H
#define __KERNEL_RESX__CORE_H 1

#include <stddef.h>

int mmu_resolve(size_t vaddr, page_t phys, int falgs);
page_t mmu_drop(size_t vaddr);
page_t mmu_protect(size_t vaddr, int falgs);
page_t mmu_read(size_t vaddr);
page_t mmu_read(size_t vaddr);
void mmu_context(mspace_t *mspace);
void mmu_create_uspace(mspace_t *mspace);
void mmu_destroy_uspace(mspace_t *mspace);
void mmu_enable();
void mmu_leave();

_Noreturn void cpu_halt();
char *cpu_rdstate(char *buf);
bool cpu_task_return_uspace(task_t *task);
int cpu_no();
int cpu_save(cpu_state_t state);
int64_t cpu_clock(int no);
time_t cpu_time();
void cpu_awake();
void cpu_reboot();
void cpu_restore(cpu_state_t state);
void cpu_return_signal(task_t *task, regs_t *regs);
void cpu_setup();
void cpu_setup_signal(task_t *task, size_t entry, long args);
void cpu_setup_task(task_t *task, size_t entry, long args);
void cpu_stack(task_t *task, size_t entry, size_t param);
void cpu_tss(task_t *);
void cpu_usermode(void *start, void *stack);

bool irq_enable();
long irq_syscall(long no, long a1, long a2, long a3, long a4, long a5);
void irq_ack(int no);
void irq_disable();
void irq_enter(int no);
void irq_fault(const fault_t *fault);
void irq_register(int no, irq_handler_t func, void *data);
void irq_reset(bool enable);
void irq_unregister(int no, irq_handler_t func, void *data);

void kernel_ready();
void kernel_start();
void kernel_sweep();

void platform_setup();

void clock_elapsed(int status);
void clock_init();
void clock_ticks();

#endif  /* __KERNEL_RESX__CORE_H */
