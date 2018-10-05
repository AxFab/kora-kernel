# KoraOS

KoraOs is an operating system written and designed from scratch.
It's developed as a hobby by an enthusiasts developer.

This repository contains code for the kernel of the system,  along with some drivers.

## Motivation

I've been trying to build my own kernel from some time. After many rework, breaks and rethinking I restart one entirely with meticulous care for the design. My objective has always been to build program that do just what they supposed to do and nothing more, light and fast with a strongly decoupled architecture.

This software is still a young pup but I have hope to build a reliable system soon. It'll provide all the basics features of a modern system and will be portable on several architecture (goal: x86, x86_64, ARM).

# Build Instructions

Build a kernel isn't like building a regular application an some extra work might be needed. The first issue might be to get a cross-compiler (for building 32bits kernel on 64bits host).

The more robust way to do it is building a cross toolchain (cf cross_gcc). It might not be required but strongly preferred. However if you wanna build for the same target or just hack into the code some delivery can be made with a regular `gcc` install.

# Build the kernel

To build the kernel image, nothing is simpler than a `make` command. However some errors might occurs without the proper settings.

You must check you have all required packages installed: `binutils, gcc, gnu-make` but also the assembler used for the targeted architecture (for Intel architecture I use `nasm`).

Some variables use `git`, but shouldn't block the compilation.
By default tests programs make use of `gcov, lcov` and `check` library.
After the build, if you're here to get dirty, think about `qemu` and `gdb`.

> **Note:** A clean build is fast but make will require `*.d` files on a second run, which will increase build time. It's perfectly normal and most makefiles always build them anyway. You can avoid this by a `make clean` or use the option `NODEPS=1`. To ignore `*.d` files. Be aware of the effects of each command.

```bash
make [kImage]         # Will build the kernel image as './bin/kImage
make check          # Will run every tests available
make cov_ck*        # Will run coverage on a test program
make kSim           # Will build a kernel simulator program for advanced testing
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

For the moment drivers must be embedded on the kernel. Some configuration can be made on `src/drivers/drivers.mk` but options are limited for now.

## Package kernel header

> _comming soon_

## Build toolchain

> _comming soon_

 - GCC, `./scripts/cross_gcc.sh`
 - Download for `x86-64` host at 

## building the system

> _Incomplete section_

 - ISO cdrom x86: `./scripts/build.sh x86` (will require `xorriso` and `grub`)

> **Commom issues:**
>
> - If you get the error `mformat invocation failed`, you will need to install `mtools` as an extra package.

# Features

It's not easy to describe kernel features, as most components are required and all kernels provide roughly the same, but differs in quality and behavior for those features.

## Platform

KoraOs'kernel is using virtual memory, with basic page allocation. It use proper recycling of pages, but no swap is available.
On Intel architecture, proper CPUs features identification is made but yet poorly used.
The kernel is multi-core ready (thread-safe) but still require some work to be initiated.

The kernel is multi-process, multi-thread but not multi-user yet. Tasks can be created to be either on kernel or in userspace. New task can be created either from clean slate (Window way) or forked (Unix way).

## Devices capabilities

The kernel provide numerous types of files and devices: block devices, char devices, pipes, directory (or volume), regular files, video streams (surface, camera or screens), network devices and sockets.

Note that Tty is not a file type on Kora, and video streams have no equivalent in Unix world (even if closed to frame-buffers).

All those files and devices can be access through an evolved VFS (virtual file system) with good caching facilities.

The network stack support the protocols: Ethernet, ARP, IPv4, ICMP, DHCP, ~~DNS, UDP, TCP and NTP~~.
However IP config is still lacking which make use of the network still difficult.

## Drivers

Here's a list of yet provided drivers:

 - **Data devices:** ATA, PS/2, serials
 - **File system:** isofs, ext2, fat
 - **Network:** Intel PRO/1000
 - **Others:** standard vga

## Commands

> _Incomplete section_


## Syscalls

> _Incomplete section_


## Library port

Even if writing his personal system allow to re-think and rewrite everything it will still be a bit pointless if no other code could run on this system.
With new functionality and support, I'm able to start porting and testing third-party libraries on my own system. Here's a list of successful port on Kora :

 - ~~openlibm~~

> A package manager is under planning to make use of those libraries.

# To know more

 - Take a look at `doc/` folder
 - [Github wiki](https://github.com/axfab/kora-os/wiki) _Still not ready yet!_
 - Reach me on [Slack](http://koraos.slack.com)