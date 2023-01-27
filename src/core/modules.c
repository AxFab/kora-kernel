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
#include <kernel/mods.h>
// #include <kernel/irq.h>
#include <kernel/vfs.h>
// #include <kernel/net.h>
#include <kernel/memory.h>
#include <kernel/dlib.h>
#include <kernel/tasks.h>


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

// void arch_init() {}

// int cpu_no() { return 0; }
// void cpu_setup() {}
// int cpu_save(task_t *task) {}
// void cpu_prepare(task_t *task, void *func, void *arg) {}
// _Noreturn void cpu_usermode(void *start, void *stack) {}

// void mmu_enable() {}
// void mmu_leave() {}
// void mmu_create_uspace(mspace_t *mspace) {}
// void mmu_destroy_uspace(mspace_t *mspace) {}
// size_t mmu_read(size_t vaddr) {}
// size_t mmu_resolve(size_t vaddr, size_t phys, int falgs) {}
// int mmu_read_flags(size_t vaddr) {}
// size_t mmu_drop(size_t vaddr) {}
// bool mmu_dirty(size_t vaddr) {}
// size_t mmu_protect(size_t vaddr, int falgs) {}


// void *kmap(size_t len, void *ino, xoff_t off, int access) {}
// void kunmap(void *ptr, size_t len) {}


// blkmap_t *blk_open(inode_t *ino, size_t blocksize) { }

