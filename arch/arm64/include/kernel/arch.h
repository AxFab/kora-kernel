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
#ifndef __KERNEL_ARCH_H
#define __KERNEL_ARCH_H 1

#include <stddef.h>
#include <stdint.h>
#include <kora/mcrs.h>

typedef size_t cpu_state_t[8];

typedef struct arm64_info arm64_info_t;
typedef struct arm64_cpu arm64_cpu_t;

typedef struct arm64_info asys_info_t;
typedef struct arm64_cpu acpu_info_t;

struct arm64_info {
    int reserved;
};

struct arm64_cpu {
    int reserved;
};



#endif  /* __KERNEL_ARCH_H */
