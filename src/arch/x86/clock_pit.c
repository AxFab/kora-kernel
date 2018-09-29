/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2018  <Fabien Bavent>
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
#include <kora/mcrs.h>

#define MICROSEC_IN_SEC (1000 * 1000)
#define PIT_CH0   0x40  /**< Channel 0 data port (read/write) */
#define PIT_CH1   0x41  /**< Channel 1 data port (read/write) */
#define PIT_CH2   0x42  /**< Channel 2 data port (read/write) */
#define PIT_CMD   0x43  /**< Mode/Command register (write only, a read is ignored) */


unsigned PIT_period = 0;
unsigned PIT_frequency = 0;

void PIT_set_interval(unsigned frequency)
{

    unsigned divisor = 1193180 / frequency; /* Calculate our divisor */
    divisor = MIN(65536, MAX(1, divisor));

    PIT_frequency = 1193180 / divisor;
    PIT_period = MICROSEC_IN_SEC / PIT_frequency;

    outb(PIT_CMD, 0x36);             /* Set our command byte 0x36 */
    outb(PIT_CH0, divisor & 0xff);   /* Set low byte of divisor */
    outb(PIT_CH0, (divisor >> 8) & 0xff);     /* Set high byte of divisor */

    kprintf(KLOG_MSG, "Set cpu ticks frequency: %d Hz\n", PIT_frequency);
}


