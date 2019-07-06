#ifndef __KERNEL_CPU_H
#define __KERNEL_CPU_H 1

#include <stddef.h>
#include <stdint.h>

typedef size_t cpu_state_t[6];

#define IRQ_ON ((void)0)
#define IRQ_OFF ((void)0)

#define KSTACK (PAGE_SIZE)

void outl(int, uint32_t);
void outw(int, uint16_t);
uint32_t inl(int);
uint16_t inw(int);


#endif  /* __KERNEL_CPU_H */

