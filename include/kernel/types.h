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
#ifndef _KERNEL_TYPES
#define _KERNEL_TYPES 1

#include <kora/stddef.h>
#include <kernel/mmu.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>


typedef long off_t;
typedef int pid_t;


#define __time_t long // Unix time seconds since EPOCH (max is +/-68 years -> 2038)
#define __time64_t long long // in us -> Max is +/-292'471 years (start from EPOCH)
#define __clock_t long

typedef __time64_t time64_t;
time64_t time64();


typedef struct inode inode_t;
typedef struct user user_t;
typedef struct task task_t;
typedef struct mspace mspace_t;
typedef struct event event_t;
typedef struct acl acl_t;

#endif  /* _KERNEL_TYPES */
