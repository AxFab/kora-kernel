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
#ifndef _SRC_APIC_H
#define _SRC_APIC_H 1

#include <kernel/core.h>

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

extern volatile uint32_t *apic_mmio;

#endif  /* _SRC_APIC_H */