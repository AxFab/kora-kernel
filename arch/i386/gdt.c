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
#include <stddef.h>
#include <stdint.h>
#include <bits/cdefs.h>

typedef struct x86_gdesc x86_gdesc_t;
PACK(struct x86_gdesc {
    uint16_t lim0_15;
    uint16_t base0_15;
    uint8_t base16_23;
    uint8_t access;
    uint8_t lim16_19 : 4;
    uint8_t other : 4;
    uint8_t base24_31;
});

void x86_write_gdesc(int no, uint32_t base, uint32_t limit, unsigned access, unsigned other)
{
    x86_gdesc_t *gdesc = (void*)(no * sizeof(x86_gdesc_t));

    gdesc->lim0_15 = (limit & 0xffff);
    gdesc->base0_15 = (base & 0xffff);
    gdesc->base16_23 = (base & 0xff0000) >> 16;
    gdesc->access = access;
    gdesc->lim16_19 = (limit & 0xf0000) >> 16;
    gdesc->other = (other & 0xf);
    gdesc->base24_31 = (base & 0xff000000) >> 24;
}

void x86_gdt()
{
    x86_write_gdesc(1, 0, 0xfffff, 0x9B, 0x0D); // Kernel code
    x86_write_gdesc(2, 0, 0xfffff, 0x93, 0x0D); // Kernel data
    x86_write_gdesc(3, 0, 0x00000, 0x97, 0x0D); // Kernel stack
    x86_write_gdesc(4, 0, 0xfffff, 0xff, 0x0D); // User code
    x86_write_gdesc(5, 0, 0xfffff, 0xf3, 0x0D); // User data
    x86_write_gdesc(6, 0, 0x00000, 0xf7, 0x0D); // User stack
}

