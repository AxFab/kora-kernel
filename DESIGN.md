
## Kernel startup

 At start up an architecture dependant code is run to setup the environment. At first this code save important information but give the hand to main function `kernel_start` as soon as possible.

 The kernel will then call back the architecture dependant code to configure in order. The memory, the cpu(s) and the clock. Those call are done by the kernel core code because we need to initialize some data structures first. 

 After basic hardware is initialize. We initialize all module of the kernel. like file system and network. We need this prior to register any new module and devices.

 With the kernel ready a call is made to `platform_start()` which can declare some modules extensions and prepare some extra devices. It's also important to declare the pre-loaded modules used to complete the kernel. 

 Finally N tasks are started as kernel threads. They will have for role to load modules, and configure the kernel by changing root, starting first usermode tasks and set users. Once all initial work have been completed those threads will terminate to keep only an idle one.

## Kernel core

## Virtual file system

## Network

## Scheduler

## Users & Security

## Modules


### Devices

### File systems

### Network protocols



 ----

## x86 

 Memory mapping

 -      0   GDT
 -    800   IDT
 -   1000   TSS (128 per entry - 32/4k)
 -   2000   MMU global directory page for CR3
 -   3000   MMU first directory page (for identity mapping) 
 -   4000   Stack for BSP (2 pages)
 -   6000
 - 
 -  20000   Kernel Code (37pages (0x25))
 -  9FC00   Start Hardware !
 -  B8000   VBA Console
 - 100000   End !!
 - 200000   Old Code ()




