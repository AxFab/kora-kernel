# KoraOS internals

  This book is for enthusiasts who want to know how the KoraOs kernel works. It describes the principles and mechanisms used; how and why the KoraOs kernel works the way it does.

  This book is freely distributable, you may copy and redistribute it under certain conditions. Please refer to the copyright and distribution statement to know more.

## Abstract

  KoraOs is a hobbyist operating system which have been designed only by myself.

  I've been trying to build my own kernel for a long time. After many rework, breaks and rethinking I decided to recreate one entirely with meticulous care for the design. My objective has always been to build program that do just what they supposed to do and nothing more, light and fast with a strongly decoupled architecture.

  I managed to made some kernel running long before that but their complexity didn't gave me enough will to transform them into real usable system, not by myself.

  At the really beginning, I hardly knew much about operating system, but that's what make a challenge interesting. I started with some basic tutorials, almost receiving the code line by line but once I understood I quickly start to move think around to better suited my amateurish vision.

  It's actually a really good way to gain experience. I don't think you learn much from failure, but you gain much more when faced with unexpected challenges. I quickly had the same issue earliest developers might have encountered. It's not surprising but the more issue I fixed the closest I became to the Linux design.

## Kernel utilities

  The kernel is not an ordinary application. You don't have any runtime or standard library to guide you in, and at the beginning no debugging utility. The lack of underlying system makes a lot of things much more complicated. However a clean architecture, strong testing and discipline will be able to make those much easier to handle.

  For this reason a non-neglectable part of the kernel is dedicated to utilities routines. In order to build reusable components, string manipulation, heap management, string and time conversion have been designed similarly to the C89 standard, with a minimum sub-system interface. Some other utilities have been added like locks, binary tree, linked list, hash map and atomics operations.

### String manipulation

  String manipulation are a common things in many programs and the kernel doesn't do exception. Hopefully the routines of `string.h` are far from the hardest to develop.

  In order to go beyond that I have plan to develop the `tchar.h` routines mechanisms as the kernel will probably handle UTF-8.

  One problem I didn't solved yet is that string allocation are really hard to handle efficiently in anything but a heap, which in turn cause memory (???).

### heap management

  This one is a really nice algorithm to know to understand block based data storage.

  My allocator is a multi-arena heap. Each arena are fixed size block built using memory mapping, and each arena can allocate several small blocks of memory. For large allocation – several pages - the arenas by themselves can be used as large page align allocated blocks.

  A fixed size arena is easier to handle. The free blocks are linked to each other in size order for fast allocation and adjacent blocks are merged to each others.

  The free list is one solution, but I preferred it over binary tree, even if complexity doesn't agree. The tree can grow in size and balancing might get costly while a list might be optimized with anchor to key sizes. In both cases, heaps algorithm are terrible in term of CPU cache.

### String conversion and formatting

  As many implementations do, to support all the `printf - scanf` collection you only need a generic `FILE` structure and the functions `vfprintf` and `vfscanf`. The sub-system will be in charge of opening and closing files.

  Which is great with those is that you need to handle all kind of primitive formatting. At this time, I still miss floating number conversion which is a lot tougher but not a prerequisite for the kernel.

### Time on the kernel

  ... Time of day, clock, synchro, NTP

### Spin locks and read-write locks

  When we learn to develop, we are told to never loop over an event as it starve other application of CPU time. However the kernel doesn't work the same as we don't always have the possibility to block a task and we need work to be done as application can deferred work to some other tasks. For this reason the most used synchronization mechanisms are spin-locks.

  The spin-lock is a lock that can't be put to sleep and will test for condition until satisfactory. Of course it means that critical section should be really short in term of execution to reduce concurrency at minimum.

  KoraOs use two type of spin-lock, a recursive spin-lock that always block IRQs and a read write lock that block IRQs only on write. Note that until now, the kernel doesn't use a mutable read-write locks (a lock on read that can be transformed to a write lock) which make the ticket implementation the perfect candidate (this implementation is faster at the depend of this particular case).

### Linked list

### Balanced binary tree

### Hash map

### Atomics operations

## The kernel architecture

  The kernel core procedures are the one that handle most of the kernel complexity but above all they need to be robust. For this reason I created intensive tests for those parts.

  The core can be split among several components. Even if each made little sense on their own I tried to developed them completely decoupled from each others. The core interact with user applications through system calls, with kernel modules and the architecture dependent layer (ADL).

## Memory management

  The memory management is definitely one of the most important subsystem of the kernel. The kernel has the task to provide protected memory resources to every application but also to share some space with external devices or for inter-process communication (IPC).

## Pagination

  > On i386, username starts at 4Mb and grow up, kernel-space ends at 4088Mb and grow down. Both grow until their limit or joining the other address space.

## Virtual File system

  The virtual file system is the layer who provide access to external devices and file system to the user applications.

## IO Layer

## Devices

## Tasks

## Sessions & Users

## Scheduler

## Kernel works & synchronization

## Time management

## Syscalls

## Architecture dependent layer (ADL)

  The kernel core is a portable code which is designed to be run on numerous of architecture. However the kernel must be aware of the hardware capabilities and mechanisms. For this reason some routines and settings are completely dependent of the hardware. The ADL is the layer between the platform requirements and the kernel core which allow it to works the same way on most platforms.

  > As this moment I only support the x86 Intel architecture, which means I only have two ADLs implemented. The Intel one and a user mode mock used for out-of-the-box testing.
  > I started however to work on a ARM one to support the mighty RaspberryPi.

  The ADL has a unique ABI which must be implemented. Amongst all routines required we identify several sub-components, MMU & CPU.

  Also the ADL is in charge of initializing the environment before the kernel is started. Once the platform is ready, one CPU core must call `kernel_start` and after calling `cpu_multi`, others CPUs can call `kernel_ready`. Both of those functions will returned and CPU should then wait for interrupts.

## Hacking and patch KoraOs

