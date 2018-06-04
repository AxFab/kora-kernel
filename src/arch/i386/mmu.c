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


void mmu_enable();
void mmu_leave();
int mmu_resolve(mspace_t*, size_t, page_t, int);
page_t mmu_read(mspace_t*, size_t);
page_t mmu_drop(mspace_t*, size_t);
int mmu_protect(mspace_t*, size_t, size_t, int);
void mmu_open_uspace(mspace_t*);
void mmu_close_uspace();
void mmu_context(mspace_t *);

void mmu_explain(mspace_t*, size_t);
void mmu_dump();
