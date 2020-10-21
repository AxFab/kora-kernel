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
#include <kernel/stdc.h>
#include <kernel/arch.h>
#include <kora/mcrs.h>


#define PIT_CH0   0x40  /**< Channel 0 data port (read/write) */
#define PIT_CH1   0x41  /**< Channel 1 data port (read/write) */
#define PIT_CH2   0x42  /**< Channel 2 data port (read/write) */
#define PIT_CMD   0x43  /**< Mode/Command register (write only, a read is ignored) */

void pit_interval(unsigned freq_hz)
{
    unsigned divisor = 1193180 / freq_hz; /* Calculate our divisor */
    divisor = MIN(65536, MAX(1, divisor));

    unsigned pit_frequency = 1193180 / divisor;
    outb(PIT_CMD, 0x34);             /* Set our command byte 0x36 */
    outb(PIT_CH0, divisor & 0xff);   /* Set low byte of divisor */
    outb(PIT_CH0, (divisor >> 8) & 0xff);     /* Set high byte of divisor */

    kprintf(KL_MSG, "Set cpu ticks frequency: %d Hz\n", pit_frequency);
}

void pit_stop()
{
    outb(PIT_CMD, 0x30);
    outb(PIT_CH0, 0xFF);
}

