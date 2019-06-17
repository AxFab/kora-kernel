# KoraOS

KoraOs is an operating system written and designed from scratch.
It's developed as a hobby by an enthusiasts developer.

This repository contains code for the kernel of the Kora system. If you wish to build the entire system look at [KoraOs](https://github.com/axfab/koraos) repository.

I've been trying to build my own kernel from some time. After many rework, breaks and rethinking I restart one entirely with meticulous care for the design. My objective has always been to build program that do just what they supposed to do and nothing more, light and fast with a strongly decoupled architecture.

This software is still a young pup but I have hope to build a reliable system soon. It'll provide all the basics features of a modern system and will be portable on several architectures (goal: x86, x86_64, ARM).

> **Interoperability**: As I try to understand how things are made, and why, I sometimes get outside of strict POSIX or UNIX specifications. I try to be as compliant as possible -- with both NT and UNIX world -- but I authorize myself to take different choices.

## Build Instructions

Build a kernel isn't like building a regular application an some extra work might be needed. 

The repository (like other from Kora) contains a close to standard `Makefile`.
A `configure` script is available but only to build from another directory and is not mandatory.
This script is only used to search and persist build options.



Here a list of main commands:

```bash
make 	        # Build the kernel for the host architecture
make check      # Build and run unit-tests
make coverage   # Run tests and coppute coverage
make install    # Build and update the kernel (not yet)
make install-headers # Copy and erase kernelheaders to $(prefix)/include

# For cross-compile using 'i386-kora-gcc' compiler
make target=i386-pc-kora CROSS=i386-kora- CC=gcc

# Simpler using configure
/path/to/sources/configure --target=i386
make # No need to set target or cross.
```

### Cross-compile

Remember that for a kernel __it's almost always a cross-compilation__.
The most __common pitfall__ is trying to build a `i386` kernel on `x86_64` host.

By default it use a __freestanding__ environment but the host headers might cause some issues.

On case or errors, it's prefered to use a 'cross-compilor'.

> **Note**: Be sure to have all package installed like _binutils, gcc, make and git_ but also the assembler for some architecture (like _nasm_ for `i386` and `amd64`.
> For checks, add also _valgrind and lcov_.

### Debug

Here also some variablez accepted by the `Makefile` for extend configuration:

 - `VERBOSE=y`: Print compilation commands
 - `QUIET=y`: Don't print verbose compilation step
 - `CFLAGS`: Add extra flags to c compiler
 - `NODEPS=y`: Don't include \*.d dependancies files (default for clean or if 'obj' directory doesn't exist yet)
 - `NOCOV=y`: Remove coverage options for unit-tests
 - `kname=?`: Change the name of the kernel delivery file

After the build, if you're here to get dirty, think about `qemu` and `gdb` for debugging and investigation.


### Toolchain

 - Download, look at `scripts/toolchain.sh` (it takes time, no logs)
 - Rebuild, look at `koraos/make/build_toolchain.sh` (it takes even more time).


## Features

It's not easy to describe kernel features, as most components are required and all kernels provide roughly the same, but differs in quality and behavior for all those features.

### Platform

KoraOs'kernel is using virtual memory, with basic page allocation. It use proper recycling of pages, but no swap is available.
On Intel architecture, proper CPUs features identification is made but yet poorly used.
The kernel is multi-core ready (thread-safe) but still require some work to be used.

The kernel is multi-process, multi-thread but not multi-user yet. Tasks can be created to be either on kernel or in userspace. New task can be created either from clean slate (Window way) or forked (Unix way).

### Devices capabilities

The kernel provide numerous types of files and devices: block devices, char devices, pipes, directory (or volume), regular files, video streams (surface, camera or screens), network devices and sockets. (Note that file types are different from UNIX).

All those files and devices can be access through an evolved VFS (virtual file system) with good caching facilities.

The network stack support the protocols: Ethernet, ARP, IPv4, ICMP, DHCP, UDP, ~~TCP, DNS and NTP~~.
The kernel provided a basic DHCP client which allow for automatic IP configuration.


### Syscalls

> _Incomplete section_

As the system is not fully functional, most of my tests are done using kernel thread, but here some working system calls -- nothing finalized yet:

```c
/* --------
  Tasks, Process & Sessions
--------- */
long sys_stop(unsigned tid, int status);
long sys_sleep(long timeout);

/* --------
  Input & Output
--------- */
long sys_read(int fd, char *buf, int len);
long sys_write(int fd, const char *buf, int len);
long sys_open(int fd, CSTR path, int flags);
long sys_close(int fd);

/* --------
  File system
--------- */
int sys_pipe(int *fds);
int sys_window(int width, int height, unsigned features, unsigned evmask);

/* --------
  Network
--------- */
int sys_socket(int protocol, const char *address, int port);

/* --------
  Memory
--------- */
void *sys_mmap(void *address, size_t length, int fd, off_t off, unsigned flags);
long sys_munmap(void *address, size_t length);
long sys_mprotect(void *address, size_t length, unsigned flags);

/* --------
  Signals
--------- */

/* --------
  System
--------- */
long sys_ginfo(unsigned info, void *buf, int len);
long sys_sinfo(unsigned info, const void *buf, int len);
long sys_log(CSTR msg);
```

## Kernel headers

> _Target bit not current files_

 - `kernel/types.h`: Reference basic types and declare opaque structures.
 - `kernel/utils.h`: Provide basic runtime like allocation, string and time.
 - `kernel/core.h`: Provide an access to core structures `kSYS` & `kCPU`.
 - `kernel/syscalls.h`: List of syscall routines as `sys_*`.
 - `kernel/io.h`: Interface for ioport, mmio and dma.
 - `kernel/{...}.h`: Provide API of a kernel core module (tasks, memory, vfs...).
 - `kernel/net/{...}.h`: API for network protocol (lo, eth, ip4).
 - `kernel/bus/{...}.h`: API for bus provider (pci or usb).

## On the roadmap

 I planned on deliver version 0.1 once I can run basics programs _logon, desktop, krish, folder and lua_ and have at least one full-featured writable file-system (like vfat or ext2).


## To know more

 - [Github wiki](https://github.com/axfab/koraos/wiki) _Still not ready yet!_
 - Reach me on [Slack](http://koraos.slack.com)
