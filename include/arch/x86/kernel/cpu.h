/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015  <Fabien Bavent>
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
#ifndef _KERNEL_CPU_H
#define _KERNEL_CPU_H 1

#include <kernel/types.h>

#define IRQ_ON   asm("sti")
#define IRQ_OFF  asm("cli")


struct regs
{
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

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

static inline void outb(uint16_t port, uint8_t val)
{
    asm volatile ("outb %1, %0" : : "dN" (port), "a" (val));
}

static inline void outw(uint16_t port, uint16_t val)
{
    asm volatile ("outw %1, %0" : : "dN" (port), "a" (val));
}

static inline void outl(uint16_t port, uint32_t val)
{
    asm volatile ("outl %1, %0" : : "dN" (port), "a" (val));
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t val;
    asm volatile ("inb %%dx, %%al" : "=a" (val) : "dN" (port));
    return val;
}

static inline uint16_t inw(uint16_t port)
{
    uint16_t val;
    asm volatile ("inw %%dx, %%ax" : "=a" (val) : "dN" (port));
    return val;
}

static inline uint32_t inl(uint16_t port)
{
    uint32_t val;
    asm volatile ("inl %%dx, %%eax" : "=a" (val) : "dN" (port));
    return val;
}

static inline void outsb(uint16_t port, const uint8_t *buf, int count)
{
    asm volatile ("rep outsb" : "+S" (buf), "+c" (count) : "d" (port));
}

static inline void outsw(uint16_t port, const uint16_t *buf, int count)
{
    asm volatile ("rep outsw" : "+S" (buf), "+c" (count) : "d" (port));
}

static inline void outsl(uint16_t port, const uint32_t *buf, int count)
{
    asm volatile ("rep outsl" : "+S" (buf), "+c" (count) : "d" (port));
}

static inline void insb(uint16_t port, uint8_t *buf, int count)
{
    asm volatile ("rep insb" : "+D" (buf), "+c" (count) : "d" (port) : "memory");
}

static inline void insw(uint16_t port, uint16_t *buf, int count)
{
    asm volatile ("rep insw" : "+D" (buf), "+c" (count) : "d" (port) : "memory");
}

static inline void insl(uint16_t port, uint32_t *buf, int count)
{
    asm volatile ("rep insl" : "+D" (buf), "+c" (count) : "d" (port) : "memory");
}

#endif /* _KERNEL_CPU_H */
