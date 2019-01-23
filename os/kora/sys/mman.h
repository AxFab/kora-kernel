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
#ifndef __SYS_MMAN_H
#define __SYS_MMAN_H 1

#include <sys/types.h>

#define PROT_EXEC 1
#define PROT_WRITE 2
#define PROT_READ 4

void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t off);
int munmap(void *addr, size_t length);

#define MMAP_HEAP  (1 << 8)
#endif /* __SYS_MMAN_H */
