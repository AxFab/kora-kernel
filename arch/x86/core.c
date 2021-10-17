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

int _errno;

int *__errno_location()
{
    return &_errno;
}

void __assert_fail(const char *expr, const char *file, int line)
{
    for (;;);
}

void irq_enable() {}
void irq_disable() {}
void irq_reset() {}

void kmap(size_t addr, size_t len, void *ino, size_t off, unsigned flags)
{
}

void kunmap(size_t addr, size_t len)
{
}


void clock_elapsed() {}


int cpu_no() {}
void cpu_halt() {}
void cpu_save() {}
void cpu_restore() {}



void mmu_context() {}
void cpu_tss() {}

struct kSys kSYS;
