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
#ifndef _KERNEL_STDC_H
#define _KERNEL_STDC_H 1

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <bits/cdefs.h>
#include <kora/time.h>
#include <kernel/mods.h>

#define VM_RD 04
#define VM_WR 02
#define VM_EX 01
#define VM_RX 05
#define VM_RW 06
#define VM_RWX 07
#define VM_RESOLVE 0100
#define VM_SHARED 0200
#define VM_UNCACHABLE 0400


#define KSTACK_PAGES 2
#define PAGE_SIZE  4096
#define HZ 100
// TODO --- REMOVE FROM HERE !

typedef size_t page_t;
typedef int64_t xoff_t;
typedef enum klog klog_t;

enum klog {
    KL_ERR = 0,
    KL_MSG,
    KL_DBG,
    KL_PF, // Page fault
    KL_IRQ,
    KL_MAL, // Memory Allocation
    KL_USR,
    KL_INO, // VFS ressources alloc
    KL_FSA, // File system allocation
    KL_BIO, // Block IO
    KL_VMA, // Log on VMA

    KL_MAX,
};


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
/* Memory allocation */
void *kalloc(size_t len);
void kfree(void *ptr);
#ifndef KORA_KRN
# define kalloc(n) kalloc_(n,#n " at " __AT__)
void *kalloc_(size_t len, const char *);
char *kstrdup(const char *str);
char *kstrndup(const char *str, size_t max);
#else
# define kstrdup strdup
# define kstrndup strndup
#endif

void *kmap(size_t len, void *ino, xoff_t off, int access);
void kunmap(void *ptr, size_t len);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
/* String manipulations */
char *sztoa(int64_t lg);
char *sztoa_r(int64_t number, char *sz_format);

int snprintf(char *buf, size_t lg, const char *msg, ...);
int vsnprintf(char *buf, size_t lg, const char *msg, va_list ap);
#if defined(_WIN32)
int sprintf_s(char *const buf, size_t lg, const char *const msg, ...);
# define snprintf(s,i,f,...) sprintf_s(s,i,f,__VA_ARGS__)
#endif

void kprintf(klog_t log, const char *msg, ...);
// _Noreturn void kpanic(const char *msg);
void kwrite(const char *buf, size_t len);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int futex_wait(int *addr, int val, long timeout, int flags);
int futex_requeue(int *addr, int val, int val2, int *addr2, int flags);
int futex_wake(int *addr, int val, int flags);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

uint8_t rand8();
uint16_t rand16();
uint32_t rand32();
uint64_t rand64();

#ifndef _WIN32
int atoi(const char *nptr);
long atol(const char *nptr);
unsigned long strtoul(const char *nptr, char **endptr, int base);
long strtol(const char *nptr, char **endptr, int base);
char *itoa(int value, char *str, int base);
#endif

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void *memcpy32(void *dest, void *src, size_t lg);
void *memset32(void *dest, uint32_t val, size_t lg);

void stackdump(size_t frame);


#endif /* _KERNEL_STDC_H */
