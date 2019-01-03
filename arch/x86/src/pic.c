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
#include <kernel/core.h>
#include <kernel/cpu.h>
#include "pic.h"

void pic_setup()
{
    /* PIC - ICW1 Initialization */
    outb(PIC1_CMD, 0x11);
    outb(PIC2_CMD, 0x11);

    /* PIC - ICW2 Initialization */
    outb(PIC1_DATA, 0x20); /* vector IRQ master */
    outb(PIC2_DATA, 0x28); /* vector IRQ slave */

    /* PIC - ICW3 Initialization */
    outb(PIC1_DATA, 0x04);
    outb(PIC2_DATA, 0x02);

    /* PIC - ICW4 Initialization */
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);

    /* PIC - Set interrupts mask */
    outb(PIC1_DATA, 0x0);
    outb(PIC2_DATA, 0x0);
}


void pic_mask_off()
{
    inb(PIC1_DATA);
    outb(PIC1_DATA, 0xFF);
    inb(PIC2_DATA);
    outb(PIC2_DATA, 0xFF);
}
