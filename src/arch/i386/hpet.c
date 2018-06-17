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
#include <kernel/core.h>
#include <kernel/cpu.h>
#include "acpi.h"
#include "hpet.h"

int hpet_interval(int freq_hz);
int hpet_stop();

size_t hpet_mmio;

int hpet_setup(acpi_hpet_t *hpet)
{
    kprintf(KLOG_ERR, "HPET mmio at %x\n", hpet_mmio);
    // kdump(rstb, rstb->length);
    hpet_regs_t *regs = kmap(PAGE_SIZE, NULL, hpet->base.base, VMA_PHYSIQ);
    uint32_t period = regs->capacities >> 32;
    if (period == 0 || period > 0x5F5E100)
        return -1;

    // reset
    kprintf(KLOG_ERR, "HPET, period of %d ns\n", period / 1000);
    regs->config = 0;
    regs->counter = 0;
    int timers = (regs->capacities >> 8) & 0x1F;
    kprintf(KLOG_ERR, "HPET, found %d timers\n", timers);
    int i, idx = -1;
    for (i = 0; i < timers; ++i) {
        if (regs->timers[i].config & HPET_TM_PERIODIC) {
            idx = i;
            break;
        }
    }
    if (idx < 0)
        return -1;

    // setup periodic timer
    uint64_t config = regs->timers[idx].config;
    config &= ~0x4004;
    config |= 0x150;
    regs->timers[idx].config = config;


    // int irq = 0;

    // config &= ~(0x1F << 9);
    // config |= irq << 9;
    // regs->timers[idx].config = config;
}

