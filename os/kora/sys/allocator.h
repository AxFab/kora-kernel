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
#ifndef _SYS_ALLOCATOR_H
#define _SYS_ALLOCATOR_H 1


#ifdef KORA_KRN
#include <kernel/core.h>
#define MMAP(l) kmap(l, NULL, 0, VMA_HEAP_RW)
#define MUNMAP(a,l) kunmap(a, l)
#else
#include <sys/mman.h>
#define MMAP(l) mmap(NULL, l, PROT_READ | PROT_WRITE, MMAP_HEAP, -1, 0);
#define MUNMAP(a,l) munmap(a, l)
#endif

#endif /* _SYS_ALLOCATOR_H */