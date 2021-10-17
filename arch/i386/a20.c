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
#include <kernel/stdc.h>
#include <kernel/arch.h>

static int enabled_A20()
{
    // int *p0 = (int*)(0x1008 | 0x100000);
    // int *p1 = (int*)0x1008;
    // *p0 = (int)p0;
    // *p1 = (int)p1;
    // return *p0 != *p1;
    return !0;
}

static void enable_A20()
{
    while (inb(0x64) & 2); // A20 Wait
    outb(0x64, 0xAD); // disable keyboard

    while (inb(0x64) & 2); // A20 Wait
    outb(0x64, 0xD0); // read from input

    while (inb(0x64) & 2); // A20 Wait
    uint8_t a = inb(0x60);

    while (inb(0x64) & 2); // A20 Wait
    outb(0x64, 0xD1); // write to output

    while (inb(0x64) & 2); // A20 Wait
    outb(0x60, a | 2); // write to output

    while (inb(0x64) & 2); // A20 Wait
    outb(0x64, 0xAE); // enable keyboard
}

void init_A20(void)
{
    if (enabled_A20()) {
        kprintf(-1, "A20 was already enabled\n");
        return;
    }

    kprintf(-1, "A20 need to be enabled\n");
    enable_A20();

    if (enabled_A20()) {
        kprintf(-1, "A20 is now enabled\n");
        return;
    }

    kprintf(-1, "A20 activation failed\n");
}
