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
    uint16_t gs, unused_g1, unused_g2, unused_g3;
    uint16_t fs, unused_f;
    uint16_t es, unused_e;
    uint16_t ds, unused_d;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rbp;
    uint64_t tmp;
    uint64_t rbx;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rax;
    uint64_t rip;
    uint16_t cs, unused_c;
    uint64_t eflags;
    /* ESP and SS are only pop on priviledge change  */
    uint64_t rsp;
    uint64_t ss;
};

static inline void outb(unsigned short port, unsigned char val)
{
    asm volatile ("outb %1, %0" : : "dN" (port), "a" (val));
}

static inline void outw(int port, uint16_t val)
{
    asm volatile ("outw %1, %0" : : "dN" (port), "a" (val));
}

static inline void outl(int port, uint32_t val)
{
    asm volatile ("outl %1, %0" : : "dN" (port), "a" (val));
}

static inline uint8_t inb(int port)
{
    unsigned char val;
    asm volatile ("inb %%dx, %%rax" : "=a" (val) : "dN" (port));
    return val;
}

static inline uint16_t inw(int port)
{
    unsigned short val;
    asm volatile ("inw %%dx, %%rax" : "=a" (val) : "dN" (port));
    return val;
}

static inline uint32_t inl(int port)
{
    unsigned int val;
    asm volatile ("inl %%dx, %%rax" : "=a" (val) : "dN" (port));
    return val;
}

static inline void outsb(int port, const uint8_t *buf, int count)
{
    asm volatile ("rep outsb" : "+S" (buf), "+c" (count) : "d" (port));
}

static inline void outsw(int port, const uint16_t *buf, int count)
{
    asm volatile ("rep outsw" : "+S" (buf), "+c" (count) : "d" (port));
}

static inline void outsl(int port, const uint32_t *buf, int count)
{
    asm volatile ("rep outsl" : "+S" (buf), "+c" (count) : "d" (port));
}

static inline void insb(int port, uint8_t *buf, int count)
{
    asm volatile ("rep insb" : "+D" (buf), "+c" (count) : "d" (port) : "memory");
}

static inline void insw(int port, uint16_t *buf, int count)
{
    asm volatile ("rep insw" : "+D" (buf), "+c" (count) : "d" (port) : "memory");
}

static inline void insl(int port, uint32_t *buf, int count)
{
    asm volatile ("rep insl" : "+D" (buf), "+c" (count) : "d" (port) : "memory");
}

#endif /* _KERNEL_CPU_H */
