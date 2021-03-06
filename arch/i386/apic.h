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
#ifndef _SRC_APIC_H
#define _SRC_APIC_H 1

#include <kernel/stdc.h>

#define APIC_ID  (0x20 / 4) // Local APIC ID
#define APIC_VERS  (0x30 / 4) // Local APIC Version
#define APIC_TPR  (0x80 / 4) // Task Priority Register
#define APIC_APR  (0x90 / 4) // Arbitration Priority Register
#define APIC_PPR  (0xA0 / 4) // Processor Priority Register
#define APIC_EOI  (0xB0 / 4) // EOI Register
#define APIC_RRD  (0xC0 / 4) // Remote Read Register
#define APIC_LDR  (0xD0 / 4) // Logical Destination Register
#define APIC_DFR  (0xE0 / 4) // Destination Format Register
#define APIC_SVR  (0xF0 / 4) // Spurious Interrupt Vector Register

#define APIC_ESR  (0x280 / 4) // Error Status Register
#define APIC_ICR_LOW  (0x300 / 4)
#define APIC_ICR_HIGH  (0x310 / 4)
#define APIC_LVT_TMR (0x320 / 4) // LVT Timer
#define APIC_LVT_PERF (0x320 / 4) // LVT Perf
#define APIC_LVT_LINT0 (0x320 / 4) // LVT Int 0
#define APIC_LVT_LINT1 (0x320 / 4) // LVT Int 1
#define APIC_LVT_ERR  (0x370 / 4) // LVT Error register
#define APIC_TM_INC (0x380 / 4) // Timer initial counter
#define APIC_TM_CUC (0x390 / 4) // Timer current counter
#define APIC_TM_DIV (0x3E0 / 4) // Timer divide

typedef struct cpu_x86 cpu_x86_t;
struct cpu_x86 {
    int id;
    char *vendor;
    char *family;
    size_t stack;
    int features[4];
};

extern cpu_x86_t *cpu_table;
extern size_t apic_mmio;
void apic_setup();
void cpuid_setup(); // in setup.c

void pic_mask_off();
void tss_setup();
void pic_setup();
xtime_t rtc_time();
void hpet_setup();

#endif  /* _SRC_APIC_H */
