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
#ifndef _SRC_HPET_H
#define _SRC_HPET_H 1

#include <kernel/stdc.h>

#define HPET_TM_LEVEL 0x2
#define HPET_TM_ENABLE 0x4
#define HPET_TM_PERIOD 0x8
#define HPET_TM_PERIODIC 0x10


typedef struct hpet_mmio_regs hpet_regs_t;

struct hpet_mmio_timer {
    volatile uint64_t config;
    volatile uint64_t counter;
    volatile uint64_t interupt;
    volatile uint64_t unused;
};

struct hpet_mmio_regs {
    volatile uint64_t capacities;
    uint64_t unused0;
    volatile uint64_t config;
    uint64_t unused1;
    volatile uint64_t interupt;
    uint64_t unused2;
    uint64_t reserved1[(0xF0 - 0x30) / 8];
    volatile uint64_t counter;
    uint64_t unusedF;
    struct hpet_mmio_timer timers[0];
};


#endif  /* _SRC_HPET_H */
