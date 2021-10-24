/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2021  <Fabien Bavent>
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
#ifndef __KERNEL_ARCH_H
#define __KERNEL_ARCH_H 1

#include <stddef.h>
#include <stdint.h>
#include <kora/mcrs.h>

typedef size_t page_t;

typedef size_t cpu_state_t[8];


#define KSTACK  (1 * PAGE_SIZE)

#define IRQ_ON   asm("sti")
#define IRQ_OFF  asm("cli")




/* Larger page in order to support 36bits physical address.
typedef unsigned long long page_t; */

#define MMU_USPACE_LOWER  (4 * _Mib_)
#define MMU_USPACE_UPPER  (512 * _Mib_)
#define MMU_KSPACE_LOWER  (3072U * _Mib_)
#define MMU_KSPACE_UPPER  (4088U * _Mib_)


#define MMU_UTBL_LOW  (MMU_USPACE_LOWER / (4 * _Mib_))
#define MMU_UTBL_HIGH  (MMU_USPACE_UPPER / (4 * _Mib_))
#define MMU_KTBL_LOW  (MMU_KSPACE_LOWER / (4 * _Mib_))
#define MMU_KTBL_HIGH  (MMU_KSPACE_UPPER / (4 * _Mib_))


#define KRN_PG_DIR 0x2000
#define KRN_PG_TBL 0x3000
#define KRN_SP_LOWER 3072
#define KRN_SP_UPPER 4088

/* Page entry flags:
 *  bit0: Is present
 *  bit1: Allow write operations
 *  bit2: Accesible into user mode
 */
#define MMU_WRITE   2
#define MMU_USERMD  4

#define MMU_K_RW 3
#define MMU_K_RO 1
#define MMU_U_RO 5
#define MMU_U_RW 7


#define MMU_PAGES(s)  (*((page_t*)((0xffc00000 | ((s) >> 10)) & ~3)))
#define MMU_K_DIR  ((page_t*)0xffbff000)
#define MMU_U_DIR  ((page_t*)0xfffff000)

void mboot_memory();

/* Kernel memory mapping :: 2 First MB

  0x....0000      0 Kb      2 Kb     GDT
  0x....0800      2 Kb      2 Kb     IDT
  0x....1000      4 Kb      4 Kb     TSS (x32, ~128 bytes each CPU)
  0x....2000      8 Kb      4 Kb     K pages dir
  0x....3000     12 Kb      4 Kb     K pages tbl0
  0x....4000     16 Kb    112 Kb     -
  0x...20000    128 Kb    608 Kb     Kernel code
  0x...B8000    736 Kb    288 Kb     Hardware
  0x..100000   1024 Kb    128 Kb     Kernel initial Stack (x32 each CPU)
  0x..120000   1152 Kb    128 Kb     Pages bitmap
  0x..140000   1280 Kb    256 Kb     -
  0x..180000   1536 Kb    512 Kb     Initial Heap Area
 */

typedef struct regs regs_t;
struct regs {
    uint16_t gs, unused_g;
    uint16_t fs, unused_f;
    uint16_t es, unused_e;
    uint16_t ds, unused_d;
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t tmp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t eip;
    uint16_t cs, unused_c;
    uint32_t eflags;
    /* ESP and SS are only pop on priviledge change  */
    uint32_t esp;
    uint32_t ss;
};

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void x86_enable_mmu();
void x86_set_cr3(page_t dir);
page_t x86_get_cr3();
void x86_set_tss(int no);
void x86_delay(int cns);
void x86_cpuid(int, int, int *);

void acpi_setup();

struct x86_tss {
    uint16_t    previous_task, __previous_task_unused;
    uint32_t    esp0;
    uint16_t    ss0, __ss0_unused;
    uint32_t    esp1;
    uint16_t    ss1, __ss1_unused;
    uint32_t    esp2;
    uint16_t    ss2, __ss2_unused;
    uint32_t    cr3;
    uint32_t    eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
    uint16_t    es, __es_unused;
    uint16_t    cs, __cs_unused;
    uint16_t    ss, __ss_unused;
    uint16_t    ds, __ds_unused;
    uint16_t    fs, __fs_unused;
    uint16_t    gs, __gs_unused;
    uint16_t    ldt_selector, __ldt_sel_unused;
    uint16_t    debug_flag, io_map;
    uint16_t    padding[12];
};

#define TSS_BASE  ((struct x86_tss*)0x1000)
#define TSS_CPU(i)  (0x1000 + sizeof(struct x86_tss) * i)

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

static inline void outb(uint16_t port, uint8_t val)
{
    asm volatile("outb %1, %0" : : "dN"(port), "a"(val));
}

static inline void outw(uint16_t port, uint16_t val)
{
    asm volatile("outw %1, %0" : : "dN"(port), "a"(val));
}

static inline void outl(uint16_t port, uint32_t val)
{
    asm volatile("outl %1, %0" : : "dN"(port), "a"(val));
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t val;
    asm volatile("inb %%dx, %%al" : "=a"(val) : "dN"(port));
    return val;
}

static inline uint16_t inw(uint16_t port)
{
    uint16_t val;
    asm volatile("inw %%dx, %%ax" : "=a"(val) : "dN"(port));
    return val;
}

static inline uint32_t inl(uint16_t port)
{
    uint32_t val;
    asm volatile("inl %%dx, %%eax" : "=a"(val) : "dN"(port));
    return val;
}

static inline void outsb(uint16_t port, const uint8_t *buf, int count)
{
    asm volatile("rep outsb" : "+S"(buf), "+c"(count) : "d"(port));
}

static inline void outsw(uint16_t port, const uint16_t *buf, int count)
{
    asm volatile("rep outsw" : "+S"(buf), "+c"(count) : "d"(port));
}

static inline void outsl(uint16_t port, const uint32_t *buf, int count)
{
    asm volatile("rep outsl" : "+S"(buf), "+c"(count) : "d"(port));
}

static inline void insb(uint16_t port, uint8_t *buf, int count)
{
    asm volatile("rep insb" : "+D"(buf), "+c"(count) : "d"(port) : "memory");
}

static inline void insw(uint16_t port, uint16_t *buf, int count)
{
    asm volatile("rep insw" : "+D"(buf), "+c"(count) : "d"(port) : "memory");
}

static inline void insl(uint16_t port, uint32_t *buf, int count)
{
    asm volatile("rep insl" : "+D"(buf), "+c"(count) : "d"(port) : "memory");
}




#endif  /* __KERNEL_ARCH_H */
