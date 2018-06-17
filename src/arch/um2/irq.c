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
#include <assert.h>
#include <stdbool.h>
#include <bits/cdefs.h>
#include <kora/atomic.h>

bool irq_enable();

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

bool irq_active = false;

bool irq_last = false;


void irq_reset(bool enable)
{
    irq_active = true;
    kCPU.irq_semaphore = enable ? 1 : 0;
    irq_last = false;
    if (enable)
        irq_enable();
}

bool irq_enable()
{
    if (irq_active) {
        assert(kCPU.irq_semaphore > 0);
        if (--kCPU.irq_semaphore == 0) {
            irq_last = true;
            return true;
        }
    }
    return false;
}

void irq_disable()
{
    if (irq_active) {
        irq_last = false;
        ++kCPU.irq_semaphore;
    }
}

void irq_ack(int no)
{
}

