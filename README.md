# KoraOS

KoraOs is an operating system written and designed from scratch.
It's developed as a hobby by an enthusiasts developer.

This repository contains code for the kernel of the system, along with some drivers.

I've been trying to build my own kernel from some time. After many rework, breaks and rethinking I restart one entirely with meticulous care for the design. My objective has always been to build program that do just what they supposed to do and nothing more, light and fast with a strongly decoupled architecture.

This software is still a young pup but I have hope to build a reliable system soon. It'll provide all the basics features of a modern system and will be portable on several architectures (goal: x86, x86_64, ARM).

> **Interoperability**: As I try to understand how things are made, and why, I sometimes get outside of strict POSIX or UNIX specifications. I definitively try to be as compliant as possible -- with both NT and UNIX world -- but I sure authorize myself to take different choices.

# Build Instructions

Build a kernel isn't like building a regular application an some extra work might be needed. The first issue might be to get a cross-compiler. By default the `makefile` will try to support the host architecture (for building 32bits kernel on 64bits host).

The more robust way to do it is building a cross toolchain (cf cross_gcc). It might not be required but strongly preferred. However if you wanna build for the same target or just hack into the code, some deliveries can be made with a regular `gcc` install.

## Fast build

The easiest way to build the kernel is not to use the Makefile directly, but to use `./scripts/build.sh`
It take a list of commands that are executed in order:
 - `-m [IMAGE]` change of targeted platform.
 - `clean` Remove temporaries files that might get on the way of the build
 - `build` Build the image (ignore dependencies, so be sure to clean first if you're in doubt)
 - `run` Run the build image using the qemu.

**Do it all**: `./scripts/build.sh -m x86 clean build run`

> **Note**: This script might require several packages to be installed...
> As:  cross-gcc, nasm, make, grub...

## Build the kernel

The `makefile` present at the root of the repository is designed to be portable, work with minimum configuration. However it might not do always what you would expect. If you are on an unsupported architecture, the command will fails. If you don't use the right compiler for the right targeted architecture you'll at best get an unusable kernel.

You must check you have all required packages installed: `binutils, gcc, gnu-make` but also the assembler used for the targeted architecture (for Intel architecture I use `nasm`). Some variables use `git`, but it shouldn't block the compilation.

The `make` command will required 2 environment variables to be set correctly `CROSS` and `target` :

 - `CROSS` is the prefix to used with the compiler (`CC, default gcc`). If you set `CROSS=i386-elf-` then you are gonna use the compiler `i386-elf-gcc` also make sure the command is available on the `PATH`.
 - `target` is a triplet for the target platform. by default we have `target=${uname -m}-pc-kora`. Only **x86** architecture is available yet, but to see which architectures are for support, look at the content of `/arch/` directory.

> By default tests programs make use of `gcov, lcov` and `check` library.
After the build, if you're here to get dirty, think about `qemu` and `gdb` for debugging.

> **Note:** A clean build is fast but make will require `*.d` files on a second run, which will increase build time. It's perfectly normal and most makefiles always build them anyway. You can avoid this by a `make clean` or use the option `NODEPS=1`. To ignore `*.d` files. Be aware of the effects of each command.

```bash
make kernel         # Will build the kernel image as './bin/kora.krn
make libc           # Will build the library './lib/lic.so
make check          # Will run every tests available
make cov_ck*        # Will run coverage on a single test program
make clean          # Remove temporary building files
make distclean      # Remove all generated files
# Other options:
  # VERBOSE=1               Print the commands send during compilation
  # QUIET=1                 Disabled progress logs
  # NODEPS=1                Do not generate or use dependencies files
  # CROSS=i686-elf-         Add a prefix to build command to use a cross-toolchain.
  # target=x86-pc-kora      Allow to change the targeted platform
  #                         (Os is ignored as that's what you're building, use full triplet).
```

## Build drivers

Drivers are put on `src/drivers` repository. On Kora, a driver is a library that run with the kernel. It can be embedded on kernel image, or distributed as a shared library. To build the driver by itself, you can use the dedicated `makefile` and select the driver by setting the variable `driver`.

`make driver='media/vga'`

> There is still some issue to configure easily if a kernel must be embedded or not, it's a work in progress. Also not all drivers are portable on all platforms.

Driver list:

 -`fs/fat` FAT and VFAT file systems (at this time only FAT16-ro is working properly.
 -`fs/isofs` ISO9660 file system, for CD-ROM.
 -`disk/ata:` Driver for ATA disk (still using only PIO mode)
 -`media/vga`: Basic VGA driver, tested only on VMs.
 -`net/e1000` : Support for Intel network cards.


## Package kernel header

> _comming soon_

## Build toolchain

> _comming soon,
> I still don't use libgcc, which is mandatory for others architectures_

 - Build GCC, `./scripts/cross_gcc.sh`
 - Download binaries for `x86-64` host using ` ./scripts/toolchain.sh install`


> **Common build issues:**
> - If you get the error `mformat invocation failed`, you will need to install `mtools` as an extra package.
> - If the ISO image isn't bootable, set `isomode=y` to use grub-legacy, or install grub2.


# Features

It's not easy to describe kernel features, as most components are required and all kernels provide roughly the same, but differs in quality and behavior for all those features.

## Platform

KoraOs'kernel is using virtual memory, with basic page allocation. It use proper recycling of pages, but no swap is available.
On Intel architecture, proper CPUs features identification is made but yet poorly used.
The kernel is multi-core ready (thread-safe) but still require some work to be used.

The kernel is multi-process, multi-thread but not multi-user yet. Tasks can be created to be either on kernel or in userspace. New task can be created either from clean slate (Window way) or forked (Unix way).

## Devices capabilities

The kernel provide numerous types of files and devices: block devices, char devices, pipes, directory (or volume), regular files, video streams (surface, camera or screens), network devices and sockets. (Note that file types are different from UNIX).

All those files and devices can be access through an evolved VFS (virtual file system) with good caching facilities.

The network stack support the protocols: Ethernet, ARP, IPv4, ICMP, DHCP, UDP, ~~TCP, DNS and NTP~~.
The kernel provided a basic DHCP client which allow for automatic IP configuration.

## Commands

> _Incomplete section_


## Syscalls

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


## Third-parties ports

Even if writing his personal system allow to re-think and rewrite everything it will still be a bit pointless if no other code could run on this system.

> _Incomplete section_

# To know more

 - Take a look at `doc/` folder
 - [Github wiki](https://github.com/axfab/kora-os/wiki) _Still not ready yet!_
 - Reach me on [Slack](http://koraos.slack.com)
