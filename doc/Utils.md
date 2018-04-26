

## Basics

 - __Allocator__:
 - __Format print__:
 - Format scan
 - Strings
 - Time

 - Linked list
 - Balanced binary tree
 - Spinlocks
 - RW-locks
 - Hash-map
 - Multi-bytes strings

## Memory

### Memory mapping

  Map areas of memory into processes or kernel address spaces.

  - `void *memory_map(mspace_t *mspace, size_t address, size_t length, inode_t* ino, off_t offset, int flags);` Map a memory area inside the provided address space.
  - `int memory_flag(mspace_t *mspace, size_t address, size_t length, int flags);` Change the flags of a memory area.
  - `memspace_t *memory_userspace();` Create a memory space for a user application
  - `int memory_scavenge();` Remove disabled memory area
  - `void memory_display();` Display the state of the current address space

### Memory hardware

  - `page_t mmu_new_page();`
  - `void mmu_release_page(page_t paddress);`
  - `void mmu_resolve(size_t vaddress, page_t paddress, int access, int zeroed);`
  - `void mmu_drop(size_t vaddress);`
  - `page_t mmu_directory();`
  - `void mmu_initialize(...);`

### Memory paging

  For the moment only handle paging error like page fault.
  Later, will also handle page swapping.

  - `Memory_pagefault();`

## Virtual file system

 - file tree: Create `dentry` nodes first unlinked with inodes, a read return an entry with `usage` incremented. Then call release to auto-clean the tree.

 - devices: Use module to `mount` device, which in turn create devices. Devices are
 not referenced by inode. To grab a device you need to use `id`.

 - Inode: All threatments on inodes are send back to drivers.

 - PCI: A kernel module that can create multiple devices but only for other modules.

 - FS: A kernel module that require a block device to work with.

 - BLK/CHR: A kernel module that create block or char files (from a device or not)

 - IRQ: An array of list with entry to register to an async IRQ.

## IO

 - rw on block, rw on pipe, synchro-copy, code-library, syslogs

 - Seat: keyboard + mouse + display!?

 - MQ and events

 - Termio: Read lines, echoing, input... (ASCII ctrl, )

 - Syslog (strace redirect..) serial...

## Tasks

 - Fiber: Smallest unit for task contains only a stack, stack ptr, entry, couple of args. Use CPU module to fill up.

 - Session: Contains info about user, root, quota...

 - Exectution context: Regroup process and thread, can be cloned/ fork... control fibers.

 - File descriptor: Map number with open file or resources

 - Schedulder: send signals, push-pop of round-robin, stop task, block task, handle task status..., ticks& quantums.

 - Time and clock: can refer to CPU to handle part of it.

 - CPU: Handle context switching, time,

## Network

 - Create network stack

 - Do ARP handshakes (Lvl3 module !?)

 - Do IP table stuffs (Lvl3 module !?)

 - Open level 4 sockets (ICMP, UDP, TCP) -- level 5 (DHCP, DNS, SSL/TLS)

## System call

 - Create basic scalls, by modules...

## Utilities

 - Basics: kmap, kprintf, kalloc, kassert...

 - Debug: stack print, dump symbols, assert, kpanic, coredump
