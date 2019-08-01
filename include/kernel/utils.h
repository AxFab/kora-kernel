/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2019  <Fabien Bavent>
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
#ifndef __KERNEL_UTILS_H
#define __KERNEL_UTILS_H 1

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <bits/libio.h>
#include <kora/llist.h>
#include <kora/bbtree.h>
#include <kora/splock.h>


typedef int64_t utime_t;
typedef struct inode inode_t;
typedef struct task task_t;


#define CLOCK_ID_MAX  2
#define CLOCK_MONOTONIC  0
#define CLOCK_REALTIME  1
#define CLOCK_LOWLATENCY 2

#define STATE_ID_MAX  5
#define CPUSTATE_IDLE  0
#define CPUSTATE_USER  1
#define CPUSTATE_SYSTEM  2
#define CPUSTATE_IRQ  3
#define CPU_STATE_STR  32

#define TASK_ZOMBIE  0
#define TASK_BLOCKED  1
#define TASK_SLEEPING  2
#define TASK_WAITING  3
#define TASK_RUNNING  4


/* Format string and write on syslog. */
int kprintf(int log, const char *msg, ...);
/* Allocate a block of memory and initialize it to zero */
void *kalloc(size_t len);
void *malloc(size_t len);
/* Free a block of memory previously allocated by `kalloc' */
void kfree(void *ptr);
void free(void *ptr);
/* Map a area into kernel memory */
void *kmap(size_t len, void *ino, size_t off, unsigned flags);
/* Unmap a area into kernel memory */
void kunmap(void *addr, size_t len);


uint8_t rand8();
uint16_t rand16();
uint32_t rand32();
uint64_t rand64();

/* - */
int snprintf(char *buf, size_t len, const char *msg, ...);
/* - */
int vfprintf(FILE *fp, const char *msg, va_list ap);


/* - */
int kwrite(FILE *fp, const char *buf, size_t len);
/* - */
int cpu_no();
/* - */
char *cpu_rdstate(char *buf);
/* - */
void scheduler_switch(int state, int code);


#endif  /* __KERNEL_UTILS_H */
