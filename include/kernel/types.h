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
#ifndef _KERNEL_TYPES
#define _KERNEL_TYPES 1

#include <kora/stddef.h>
#include <kora/llist.h>
#include <kernel/mmu.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>


typedef long off_t;
typedef int pid_t;

#if !defined(_WIN32)
# define __time_t long // Unix time seconds since EPOCH (max is +/-68 years -> 2038)
#else
# define __time_t long long
#endif
#define __clock64_t long long // in us -> Max is +/-292'471 years (start from EPOCH)
#define __clock_t long

typedef __clock64_t clock64_t;


clock64_t kclock();
#define USEC_PER_SEC  1000000ULL

/* Kernel development kit */
typedef struct kdk_api kdk_api_t;

// Tasks
typedef struct task task_t;
typedef struct event event_t;
typedef struct advent advent_t;
typedef struct emitter emitter_t;


// Memory
typedef struct mspace mspace_t;

// Files
typedef struct inode inode_t;
typedef struct device device_t;
typedef struct volume volume_t;

// Network
typedef struct netdev netdev_t;
typedef struct skb skb_t;
typedef struct socket socket_t;


// Users
typedef struct user user_t;
typedef struct acl acl_t;

// System
typedef struct regs regs_t;
typedef struct fault fault_t;

typedef struct bio bio_t;

typedef struct proc proc_t;

typedef struct dynlib dynlib_t;
typedef struct dynsec dynsec_t;
typedef struct dynsym dynsym_t;
typedef struct dynrel dynrel_t;
typedef struct dyndep dyndep_t;


typedef int(*irq_handler_t)(void *);



struct fault {
    int raise;
    const char *name;
    const char *mnemonic;
};



struct emitter {
    llhead_t list;
    // TODO - Add lock as soon as we dont required global lock.
};

typedef struct ino_ops ino_ops_t;
typedef struct dev_ops dev_ops_t;
typedef struct fs_ops fs_ops_t;


#endif  /* _KERNEL_TYPES */
