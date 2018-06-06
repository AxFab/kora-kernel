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


void mmu_enable()
{
	// Setup first heap arena [1Mib - 2Mib]
	setup_allocator((void *)(1024 * _Kib_), 1024 * _Kib_);
	
	// Record physical available memory
	
	
	// Prepare kernel PGD
	page_t *dir = 
	page_t *tbl = 
	memset(dir, 0, PAGE_SIZE);
	memset(tbl, 0, PAGE_SIZE);
	tbl[1023] = tbl[1022] = (page_t)dir | MMU_KRW;
	for (i = 0; i < 512; ++i)
	    tbl[i] = (i * PAGE_SIZE) | MMU_KRW;
	
	kMMU.kspace->lower_bound = KSPACE_LOWER;
	kMMU.kspace->upper_bound = KSPACE_UPPER;
	x86_enable_mmu();
}

void mmu_leave()
{
	
}

int mmu_resolve(size_t vaddr, page_t phys, int flags)
{
    page_t *dir = (page_t*)(0xFFFFF000 | ((vaddr >> 20) & ~3));
	page_t *tbl = (page_t*)(0xFFC00000 | ((vaddr >> 10) & ~3));
	if (*dir == 0) {
		if (vaddr >= KSPACE_LOWER) {
			
		}
	}
}

page_t mmu_read(size_t vaddr)
{
	page_t *dir = (page_t*)(0xFFFFF000 | ((vaddr >> 20) & ~3));
	page_t *tbl = (page_t*)(0xFFC00000 | ((vaddr >> 10) & ~3));
	if ((*dir & 1) == 0 || ((*tbl & 1) == 0)
	    return 0;
	return *tbl & ~(PAGE_SIZE - 1);
}

page_t mmu_drop(size_t vaddr)
{
	page_t *dir = (page_t*)(0xFFFFF000 | ((vaddr >> 20) & ~3));
	page_t *tbl = (page_t*)(0xFFC00000 | ((vaddr >> 10) & ~3));
	if ((*dir & 1) == 0 || ((*tbl & 1) == 0)
	    return 0;
	page_t pg = *tbl & ~(PAGE_SIZE - 1);
    *tbl = 0;
    return pg;
}

int mmu_protect(size_t vaddr, int flags)
{
	page_t *dir = (page_t*)(0xFFFFF000 | ((vaddr >> 20) & ~3));
	page_t *tbl = (page_t*)(0xFFC00000 | ((vaddr >> 10) & ~3));
	if ((*dir & 1) == 0 || ((*tbl & 1) == 0)
	    return -1;
	page_t pg = *tbl & ~(PAGE_SIZE - 1);
	*tbl = pg | 
    return 0;
}

void mmu_open_uspace(mspace_t*);
void mmu_close_uspace();
void mmu_context(mspace_t *);

void mmu_explain(mspace_t*, size_t);
void mmu_dump();
