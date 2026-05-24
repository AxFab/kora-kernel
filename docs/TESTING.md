# Testing

In the process of kernel develoment, a lot of tests are performed on the kernel, to ensure the stability and functionality of the product.
For this, there is many ways of running some kernel piece of code. 

Because the kernel is build as loosely couple module, building one module with some stubs is achevable. For this reason I create small cli program to be able to simulate any load the kernel might recive.

Here is a list of all ways I used to run and test some part of the kernel.

## Run the kernel image

Running the full kernel is of course the easiest method, but It required often several external programs and to build the entire system image.

However, using qemu, loading the kernel without the full image is possbile, but not really usefull.

The kernel all by himself won't do much, it might load kernel modules (if provided boot-drive) and will stop because unable to find any program to run.


## Bootstrap

> __Not ready__: For the moment the kernel only work on x86 compatible architecture however, everythink have been thought to ease code portability.
> As such, all the architecture-dependant code is isolate on a specific folder and must complie to a (more or less defined) contract.
>
> The code must be hable to start the machine and initialize the system (clock, cpus, virtualization...). The main processor must call the function `kstart()` and others processors must call `kready()`.
> The platform must also provide an implementation for several routines (`cpu_*()` and `mmu_*()` functions).

> _A bootstrap test is under design to be compiled with only the architecture code and testing all features without loading the rest of the kernel._

## Virtual memory space

```
make clean
make cli-mem
cd tests
../bin/cli-mem mm_start.sh
```

This program create smalls addressable spaces to test allocation of memory areas, paging routines and copy-on-write mechanisms (which is often hard to debug live).


## Virtual File System

```
make clean
make cli-vfs
cd tests
../bin/cli-vfs fs_ext2.sh
../bin/cli-vfs fs_isofs.sh
../bin/cli-vfs fs_main.sh
../bin/cli-vfs -i  # Open interactive shell (not much documentation yet)
```

This test all file-system operations but is also testing any file-system drivers.
The tool come also with a usefull module to be able to use files as disk-image (support raw, VHD fixed or VHD dynamic formats).

> _Update on 2023-05-11_: Most common FS routines have been tested, mainly with ext2 driver. I don't have user support so uid/gid is not working as expected, and I have some knowned issue on file larger than 12 blocks (ext2 specific). I used this program to track memory leak wich is not a trivial task on such system.

## Network

```
make clean
make cli-net
cd tests
../bin/cli-net net_ping0.sh
../bin/cli-net net_ip0.sh
```

The interactive netowrk test program is capable to instantiate several network stacks (one per kernel/machine usualy) and create a full network by adding interfaces (network cards) and link them as fit for any tests.

> _Update on 2023-05-11_: Currently I tested DHCP, ARP and ping command of ICMP. I also have a proper implementation for UDP sockets.


## Tasks (threads and processes)

Still incomplete, this one will allow to make some test on the scheduler.
Right now, it only only allows to check process allocation is working.


