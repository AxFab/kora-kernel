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
#include <kora/llist.h>
#include <kernel/mmu.h>
#include <kernel/types.h>
#include <kernel/vma.h>
#include <time.h>

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

page_t mmu_read(mspace_t *mspace, size_t vaddress);
void kernel_tasklet(void* start, long arg, CSTR name);

int rand(void);
uint16_t rand16();
uint32_t rand32();
uint64_t rand64();
unsigned long strtoul(const char *nptr, char **endptr, int base);
int sprintf(char *str, const char *format, ...);
int snprintf(char *, int, const char *, ...);
int vsnprintf(char *, int, const char *, va_list ap);
int vprintf(const char *format, va_list ap);

void *malloc(size_t size);
void free(void *ptr);


const char *ksymbol(void *eip);
void stackdump(size_t frame);
void kdump(const void *buf, int len);
/* Store in a temporary buffer a size in bytes in a human-friendly format. */
char *sztoa(size_t number);
uint32_t crc32(const void *buf, size_t len);


// page_t mmu_read(size_t vaddress);
void kdump(const void *buf, int len);
void task_core(task_t *task);
void task_wait(void *listener, long timeout_ms);

#define KLOG_MSG 0 // Regular messages
#define KLOG_DBG 1 // Debug information
#define KLOG_ERR 2 // Errors
#define KLOG_MEM 3 // Memory information
#define KLOG_IRQ 4 // IRQ information
#define KLOG_PF 5 // Page fault and resolution info
#define KLOG_SYC 6 // Syscall trace
#define KLOG_NET 7 // Network packets
#define KLOG_TSK 8 // Task information
#define KLOG_USR 9 // User syslog

typedef struct kmod kmod_t;
struct kmod {
    const char *name;
    int license;
    void (*setup)();
    void (*teardown)();
    llnode_t node;
};

enum license {
    MOD_AGPL,
    MOD_GPL,
    MOD_LGPL,
    MOD_ZLIB,
    MOD_COMMERCIAL,
    MOD_PRIVATE,
};

#define MODULE(n,l,s,t) \
    kmod_t kmod_info_##n = { \
        .name = #n, \
        .license = l, \
        .setup = s, \
        .teardown = t, \
    }

#define MOD_REQUIRE(n) \
    static CSTR _mod_require_##n __attribute__(section("reqr"), used) = #n

#define KMODULE(n) extern kmod_t kmod_info_##n
#define KSETUP(n) kmod_info_##n.setup();
#define KTEARDOWN(n) kmod_info_##n.teardown();

#endif  /* _KERNEL_CORE_H */
