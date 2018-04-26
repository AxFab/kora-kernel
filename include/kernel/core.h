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
#ifndef _KERNEL_CORE_H
#define _KERNEL_CORE_H 1

#include <stdarg.h>
#include <kora/stddef.h>
#include <kernel/asm/mmu.h>
#include <kernel/types.h>
#include <kernel/asm/vma.h>
#include <time.h>

#define FPTR "%016p"
typedef const char* CSTR;

void outb(int port, uint8_t val);
void outw(int port, uint16_t val);
void outl(int port, uint32_t val);
uint8_t inb(int port);
uint16_t inw(int port);
uint32_t inl(int port);
void outsb(int port, const uint8_t *buf, int count);
void outsw(int port, const uint16_t *buf, int count);
void outsl(int port, const uint32_t *buf, int count);
void insb(int port, uint8_t *buf, int count);
void insw(int port, uint16_t *buf, int count);
void insl(int port, uint32_t *buf, int count);

void *kalloc(size_t size);
void kfree(void *ptr);
void kprintf(int log, const char *msg, ...);
char *sztoa(size_t lg);
void *kmap(size_t length, inode_t *ino, off_t offset, int flags);
void kunmap(void *address, size_t length);
_Noreturn void kpanic(const char *ms, ...);
void kclock(struct timespec *ts);

void kernel_start();
void kernel_ready();
void kernel_sweep();

int rand(void);
unsigned long strtoul(const char *nptr, char **endptr, int base);
int sprintf(char *str, const char *format, ...);
int snprintf(char *, int, const char *, ...);
int vprintf(const char *format, va_list ap);

void *malloc(size_t size);
void free(void *ptr);

#endif  /* _KERNEL_CORE_H */
