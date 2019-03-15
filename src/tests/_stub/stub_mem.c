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
#include <kernel/memory.h>
#include <kernel/mmu.h>
#include "../check.h"
#include <errno.h>

page_t mmu_read(size_t addr)
{
    void *ptr = _valloc(PAGE_SIZE);
    memcpy(ptr, (void *) addr, PAGE_SIZE);
    return (page_t)ptr;
}

void page_release(page_t addr)
{
    _vfree((void *) addr) ;
}

int page_fault(mspace_t *mspace, size_t address, int reason)
{
    return -1;
}
