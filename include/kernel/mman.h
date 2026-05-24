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
#ifndef _KERNEL_MMAN_H
#define _KERNEL_MMAN_H 1

#define PROT_EXEC 1
#define PROT_WRITE 2
#define PROT_READ 4
#define PROT_NONE 0

#define MAP_SHARED 8
#define MAP_PRIVATE 0


// The mapping is not backed by any file; its contents are initialized to zero. The fd and offset arguments are ignored; however, some implementations require fd to be -1 if MAP_ANONYMOUS (or MAP_ANON) is specified, and portable applications should ensure this. The use of MAP_ANONYMOUS in conjunction with MAP_SHARED is only supported on Linux since kernel 2.4.
#define MAP_ANON MAP_ANONYMOUS
#define MAP_ANONYMOUS 0x10

// This flag is ignored. (Long ago, it signaled that attempts to write to the underlying file should fail with ETXTBUSY. But this was a source of denial-of-service attacks.)
#define MAP_DENYWRITE 0x20
// This flag is ignored.
#define MAP_EXECUTABLE 0x40
// Compatibility flag. Ignored.
#define MAP_FILE 0x80
// Don't interpret addr as a hint: place the mapping at exactly that address. addr must be a multiple of the page size. If the memory region specified by addr and len overlaps pages of any existing mapping(s), then the overlapped part of the existing mapping(s) will be discarded. If the specified address cannot be used, mmap() will fail. Because requiring a fixed address for a mapping is less portable, the use of this option is discouraged.
#define MAP_FIXED 0x100
// Used for stacks. Indicates to the kernel virtual memory system that the mapping should extend downward in memory.
#define MAP_GROWSDOWN 0x200
// Allocate the mapping using "huge pages." See the Linux kernel source file Documentation/vm/hugetlbpage.txt for further information.
#define MAP_HUGETLB 0x400
// Lock the pages of the mapped region into memory in the manner of mlock(2). This flag is ignored in older kernels.
#define MAP_LOCKED 0x800
// Only meaningful in conjunction with MAP_POPULATE. Don't perform read-ahead: only create page tables entries for pages that are already present in RAM. Since Linux 2.6.23, this flag causes MAP_POPULATE to do nothing. One day the combination of MAP_POPULATE and MAP_NONBLOCK may be reimplemented.
#define MAP_NONBLOCK 0x1000
// Do not reserve swap space for this mapping. When swap space is reserved, one has the guarantee that it is possible to modify the mapping. When swap space is not reserved one might get SIGSEGV upon a write if no physical memory is available.
#define MAP_NORESERVE 0x2000
// Populate (prefault) page tables for a mapping. For a file mapping, this causes read-ahead on the file. Later accesses to the mapping will not be blocked by page faults. MAP_POPULATE is only supported for private mappings.
#define MAP_POPULATE 0x4000
// Allocate the mapping at an address suitable for a process or thread stack.
#define MAP_STACK 0x8000
// Don't clear anonymous pages. This flag is intended to improve performance on embedded devices. This flag is only honored if the kernel was configured with the CONFIG_MMAP_ALLOW_UNINITIALIZED option. Because of the security implications, that option is normally enabled only on embedded devices (i.e., devices where one has complete control of the contents of user memory).
#define MAP_UNINITIALIZED 0x10000
// Put the mapping into the first 2 Gigabytes of the process address space. This flag is only supported on x86-64, for 64-bit programs. It was added to allow thread stacks to be allocated somewhere in the first 2GB of memory, so as to improve context-switch performance on some early 64-bit processors. Modern x86-64 processors no longer have this performance problem, so use of this flag is not required on those systems. The MAP_32BIT flag is ignored when MAP_FIXED is set.
#define MAP_32BIT 0x20000
// Allocate the mapping at an address suitable for a process heap.
#define MAP_HEAP 0x40000



#if defined(__GNU) || defined(__BSD)
#define MADV_NORMAL      0
#define MADV_RANDOM      1
#define MADV_SEQUENTIAL  2
#define MADV_WILLNEED    3
#define MADV_DONTNEED    4
#define MADV_FREE        8
#define MADV_REMOVE      9
#define MADV_DONTFORK    10
#define MADV_DOFORK      11
#define MADV_MERGEABLE   12
#define MADV_UNMERGEABLE 13
#define MADV_HUGEPAGE    14
#define MADV_NOHUGEPAGE  15
#define MADV_DONTDUMP    16
#define MADV_DODUMP      17
#define MADV_WIPEONFORK  18
#define MADV_KEEPONFORK  19
#define MADV_HWPOISON    100
#define MADV_SOFT_OFFLINE 101
#endif


#endif /* _KERNEL_MMAN_H */
