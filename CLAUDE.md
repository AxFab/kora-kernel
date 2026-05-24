# CLAUDE.md — KoraOS Kernel

This file gives a working AI assistant (or a new contributor) a complete
picture of the `kora-kernel` repository: what it is, how it is organised,
how to build and test it, and the current state of each subsystem.

---

## Project overview

**KoraOS** is a hobbyist monolithic kernel written from scratch in C and
x86 assembly, designed and developed by Fabien Bavent.  The primary target
is the **i386** (IA-32) architecture.  Port stubs exist for x86_64 and
arm64 but are not functional.

The kernel aims to be light, fast, and modular, with a clean separation
between portable core code and architecture-dependent layers.  It is
**not** POSIX-compliant but borrows ideas from both UNIX and Windows.

License: **GNU Affero GPL v3**.

Current version tag: `v0.0` (pre-release, hobby project).

---

## Repository layout

```
kora-kernel/
├── arch/
│   ├── i386/          # Fully implemented target architecture
│   │   ├── boot/      # Start-up: start.asm, GDT, IDT, IRQ, MMU, main.c
│   │   ├── include/kernel/arch.h   # Arch ABI (cpu_state_t, port I/O inlines)
│   │   ├── kernel.ld  # Linker script
│   │   ├── make.mk    # Arch-specific build rules
│   │   ├── apic.c  cpuid.c  mboot.c  pci.c  pic.c  rtc.c  serial.c  tss.c
│   │   └── ...
│   ├── x86_64/        # STUB — headers only, no implementation
│   └── arm64/         # STUB — make.mk + include/bits/atomic.h (macOS/arm64 host builds)
├── drivers/
│   ├── fs/
│   │   ├── ext2/      # ext2 read/write (mostly complete, known issues)
│   │   ├── isofs/     # ISO 9660 read-only
│   │   └── vfat/      # FAT12/16/32 (partial write support)
│   ├── misc/vbox/     # VirtualBox guest additions
│   ├── net/ip4/       # IPv4 stack: ARP, DHCP, ICMP, UDP (working); TCP (stub)
│   └── pc/
│       ├── ata/       # PATA/PATAPI block driver
│       ├── e1000/     # Intel e1000 NIC driver
│       ├── ps2/       # PS/2 keyboard + mouse
│       ├── vga/       # VGA text/framebuffer
│       ├── ac97/      # AC97 audio (not built by default)
│       └── sb16/      # SoundBlaster 16 (not built by default)
├── include/
│   ├── kernel/        # Public kernel API headers
│   │   ├── arch.h     # Re-exported from arch/$(target)/include/
│   │   ├── core.h     # sys_info_t, cpu_info_t, kstart/kready
│   │   ├── dlib.h     # Dynamic library / ELF loader types
│   │   ├── irq.h      # IRQ registration, syscall dispatch
│   │   ├── memory.h   # vmsp_t, vma_t, page_*, mmu_* API
│   │   ├── mods.h     # kmodule_t, EXPORT_MODULE, EXPORT_SYMBOL
│   │   ├── net.h      # Network stack types and API
│   │   ├── stdc.h     # kalloc/kfree, kmap/kunmap, kprintf
│   │   ├── syscalls.h # sys_* prototypes, syscall numbers
│   │   ├── tasks.h    # task_t, scheduler_t, masterclock_t
│   │   └── vfs.h      # inode_t, fnode_t, device_t, vfs_* API
│   ├── kora/          # Internal utility headers (mcrs, llist, bbtree, hmap, …)
│   ├── bits/          # Low-level / compiler-specific (cdefs, atomic, io)
│   └── cc/            # Freestanding standard header wrappers
├── make/              # Build system fragments
│   ├── global.mk      # Directory vars, toolchain detection, helper macros
│   ├── build.mk       # Generic compile/link rules (comp_source, link_bin)
│   ├── drivers.mk     # Driver .ko build rules
│   ├── targets.mk     # Install targets
│   └── host.sh        # Host triple detection script
├── scripts/           # Utility scripts (toolchain, coverage, CI helpers)
├── src/
│   ├── core/          # Kernel entry, IRQ, syscall dispatch, module loader
│   ├── mem/           # Virtual memory, page allocator, ELF/dlib loader
│   ├── net/           # Network stack core (eth, lo, skb, sock)
│   ├── stdc/          # Kernel-internal C stdlib (heap, format, locks, …)
│   ├── tasks/         # Scheduler, threads, clock, streams
│   └── vfs/           # Virtual file system (inodes, fnodes, block cache, …)
├── tests/
│   ├── stub/          # Stubs/mocks replacing arch and mm for host testing
│   ├── vfs/           # VFS CLI test harness sources
│   ├── net/           # Network CLI test harness sources
│   ├── mem/           # Memory CLI test harness sources
│   ├── tasks/         # Task CLI test harness sources
│   ├── *.sh           # Shell test scripts run by the CLI programs
│   ├── cli.c/h        # CLI test framework
│   ├── threads.c      # pthreads shim for hosted tests (excluded when ADD_C11=y)
│   └── c11/threads.h  # C11 <kernel/threads.h> polyfill for platforms without it (e.g. macOS)
├── docs/              # Sparse documentation (see state below)
├── Makefile           # Root build entry
├── configure          # Out-of-tree build helper
└── config.yml         # Driver selection config (not yet consumed by make)
```

---

## Build system

The build system is **GNU Make** with a modular include structure under
`make/`.  All build artefacts go into `obj/`, `bin/`, and `lib/`
directories inside the build tree (defaults to the source tree).

### Key variables

| Variable | Default | Purpose |
|---|---|---|
| `target` | host triple | Build target (`i386-pc-kora`, `x86_64-pc-linux`, …) |
| `CROSS` | (empty) | Cross-compiler prefix (e.g. `i386-elf-`) |
| `CC` | `$(CROSS)gcc` | C compiler |
| `prefix` | `/usr/local` | Install prefix |
| `VERBOSE=y` | — | Print full compiler commands |
| `QUIET=y` | — | Suppress step banners |
| `NOCOV=y` | — | Disable `--coverage` flags on host tests |
| `NODEPS=y` | — | Skip `.d` dependency files |

### Main targets

```bash
make                   # Build the kernel (freestanding, requires cross-compiler)
make check             # Build and run all hosted unit tests
make coverage          # Run tests and generate lcov coverage report

make cli-vfs           # Build the VFS test harness
make cli-mem           # Build the memory test harness
make cli-net           # Build the network test harness
make cli-tsk           # Build the task/scheduler test harness

make install           # Install kernel image + initrd to $(prefix)/boot
make install-headers   # Copy kernel headers to $(prefix)/include
make clean             # Remove obj/, bin/, lib/
```

### Cross-compiling for i386

```bash
# Using a pre-built cross-compiler (see scripts/toolchain.sh):
make target=i386-pc-kora CROSS=i386-kora- CC=gcc

# Or using configure for out-of-tree builds:
/path/to/sources/configure --target=i386
make
```

### Running tests (no toolchain needed)

```bash
make clean && make cli-vfs
cd tests
../bin/cli-vfs fs_ext2.sh
../bin/cli-vfs fs_main.sh
../bin/cli-vfs -i          # interactive shell

make clean && make cli-net
../bin/cli-net net_ping0.sh
../bin/cli-net net_ip0.sh

make clean && make cli-mem
../bin/cli-mem mm_start.sh
```

On **macOS (arm64)** the system does not provide `<kernel/threads.h>`.  Use the
`ADD_C11=y` flag, which activates a header-only polyfill (`tests/c11/threads.h`)
that maps the full C11 threads API to pthreads, and excludes the now-redundant
`tests/threads.c` from the build:

```bash
ADD_C11=y make cli-vfs
ADD_C11=y make cli-net
ADD_C11=y make cli-mem
ADD_C11=y make cli-tsk
```

`-lpthread` is already always linked for `cli-*` targets so no extra flag is
needed.

### CI

GitLab CI (`.gitlab-ci.yml`) runs:
- `make check` (host tests + coverage) for every push
- `make` targeting `i386-pc-kora` using the `axfab/kora-gcc` Docker image

---

## Boot sequence

1. **`arch/i386/boot/start.asm`** — multiboot entry, sets up a minimal
   stack, enters 32-bit protected mode, calls `cpu_setup()`.
2. **`arch/i386/boot/main.c :: cpu_setup()`** — initialises MMU, ACPI,
   CPUID, APIC, PIC/PIT, TSS; registers IRQ 0 (clock handler).
3. **`src/core/launch.c :: kstart()`** — single-CPU kernel entry point:
   - Calls `clock_init()`, `module_init()`, `vfs_init()`,
     `scheduler_init()`, `net_setup()`, `arch_init()` in order.
   - Spawns the `kloader` kernel thread, then calls `irq_zero()` to
     enter the scheduler loop.
4. **`kloader()`** — scans bootstrap ramdisk directories (`/mnt/boot*`)
   for `.ko` kernel modules, loads them (ELF32 + symbol relocation),
   mounts a CD-ROM as root, mounts `devfs`, then `task_spawn()`s the
   first userspace program (`krish`).
5. **Secondary CPUs** call `kready()` — minimal CPU init then spin until
   `sysinfo.is_ready` is set.

---

## Subsystem summaries

### `src/stdc` — Kernel standard library

Provides standard C runtime primitives that cannot use syscalls:

- **Heap / allocator** (`heap.c`, `arena.c`, `allocator.h`): multi-arena
  allocator; each arena is a `kmap()`-ed region, free-list based.
  `kalloc()` / `kfree()` are the public API.
- **Formatting** (`format_*.c`): `vfprintf`/`vfscanf` core with a `FILE`
  abstraction; `snprintf`, `vsnprintf`, `kprintf` (kernel log).
  Floating point not yet supported.
- **String / mbstring** (`string.c`, `mbstr.c`): `memcpy`, `strcmp`,
  `strlen`, UTF-8 multi-byte string helpers.
- **Collections**:
  - `llist.c` — intrusive doubly linked list (`llhead_t`, `llnode_t`)
  - `bbtree.c` — balanced binary tree (red-black / BB), used for VMAs,
    tasks, inodes, protocols
  - `hmap.c` — open-addressing hash map
- **Synchronisation** (`lock.c`, `splock.h`, `rwlock.h`, `sem.c`,
  `mtx.c`, `cnd.c`): spinlocks (IRQ-masking), read-write spinlocks,
  POSIX-like semaphores, mutexes, condition variables.
- **Misc**: `random.c` (PRNG), `time.c` (xtime / xtime_name_t helpers),
  `bits.c` (bitmap operations), `blkmap.c` (block-to-page helper),
  `debug.c`, `error.c`.

### `src/mem` — Virtual memory management

The memory subsystem manages both physical pages and virtual address
spaces.

- **`pages.c`** — Physical page allocator.  Memory zones are registered
  via `page_range()`.  A bitmap (`mzone_t`) tracks free pages.
  `page_new()` / `page_get()` / `page_release()` are the public API.
  `__mmu` (`struct kMmu`) is the global descriptor.
- **`vmsp.c`** — Virtual Memory Space (`vmsp_t`).  Each task has one;
  the kernel has `__mmu.kspace`.  Backed by a BB-tree of `vma_t` sorted
  by base address.  `vmsp_map()` / `vmsp_unmap()` / `vmsp_protect()` /
  `vmsp_resolve()` (page fault handler).
- **`vma.c`** — VMA creation dispatcher.  Dispatches to type-specific
  operation tables (`vma_ops_t`).
- **`vma_anon.c`** — Anonymous memory (e.g. `malloc` backing).
  Copy-on-write (`VMA_COW`) is supported.
- **`vma_file.c`** — File-backed mapping.  Pages are fetched from the
  block cache via `vfs_fetch_page()`.
- **`vma_misc.c`** — Stack, heap, pipe, physical (MMIO) mappings.
- **`vma_dlib.c`** — Library section mappings (text/data/rodata).
- **`dlib.c`**, **`elf32.c`** — ELF32 dynamic library loader.
  Parses ELF, resolves symbols against the kernel symbol table, rebases
  sections into the target address space.  Used for both kernel modules
  (`.ko` files) and userspace ELF executables.
- **`mcheck.c`** — Debug heap checker.
- **`memory.c`** — `memory_initialize()` / `memory_sweep()` / `memory_info()`.

Key types: `vmsp_t` (address space), `vma_t` (mapped region),
`vma_ops_t` (per-type callbacks), `page_sharing_t` (COW page tracking).

### `src/vfs` — Virtual File System

All file I/O flows through the VFS.  The design has two layers:

**Tree layer (`fnode_t`)** — path tree nodes, roughly equivalent to
dentries in Linux.  Each `fnode_t` has a name, a parent, and an
associated `inode_t`.  The tree is maintained in a hash map and an LRU
list for scavenging.

**File layer (`inode_t`)** — file objects.  One inode per file, keyed by
`(device_no, inode_no)` and stored in a per-device BB-tree.  Reference
counted (RCU).  Lifecycle: `vfs_inode()` (create/get) → `vfs_open_inode()`
(inc RCU) → `vfs_close_inode()` (dec RCU, destroy when 0).

File types (`ftype_t`): `FL_REG`, `FL_BLK`, `FL_PIPE`, `FL_CHR`,
`FL_NET`, `FL_SOCK`, `FL_LNK`, `FL_FRM` (framebuffer), `FL_DIR`.

Driver interface: `ino_ops_t` (filesystem operations) + `fl_ops_t`
(open/close/read/write for special files).  Filesystem drivers register
via `vfs_addfs()`.  Devices are registered via `vfs_mkdev()`.

Key source files:
- `inode.c` — inode lifecycle, `vfs_chmod`, `vfs_chown`, `vfs_utimes`
- `fnode.c` — fnode lifecycle, `vfs_open`, `vfs_mkdir`, `vfs_unlink`,
  `vfs_rename`, `vfs_mount`, `vfs_umount`
- `fswalk.c` — path resolution (`vfs_search()`)
- `block.c` — page-cache backed block I/O
- `bio.c` — block I/O queue
- `pipe.c` — kernel pipe (single-page ring buffer, auto-extends to 64 K)
- `sock.c` — socket inode wrapper
- `devfs.c` — `devfs` filesystem (populates `/dev`)
- `tarfs.c` — read-only in-memory tarball filesystem
- `framebuffer.c` — framebuffer inode
- `parts.c` — MBR / GPT partition scanning
- `device.c` — device registry
- `directory.c` — generic directory helpers
- `data.c` — data I/O helpers

### `src/net` — Network stack core

The network subsystem is designed around a per-stack-instance model
(`netstack_t`) to allow multiple independent stacks in tests.

- `net.c` — stack creation, interface registration, event dispatch,
  protocol registry, daemon thread (`net_deamon()`).
- `skb.c` — socket kernel buffer (tx: `net_packet()` + `net_skb_write()`
  + `net_skb_send()`; rx: `net_skb_recv()` enqueues to the daemon).
- `sock.c` — socket lifecycle and send/recv.
- `eth.c` — Ethernet (802.3) framing, registered as protocol `NET_AF_ETH`.
- `lo.c` — loopback device (`NET_AF_LO`).

IPv4 and higher protocols live in `drivers/net/ip4/`:
- `ip4.c` / `ip4_mod.c` — IPv4 routing and receive dispatch
- `ip4_sock.c` — port allocation and socket lookup
- `arp.c` — ARP (request / reply / cache)
- `icmp.c` — ICMP echo request/reply (ping)
- `dhcp.c` — DHCP client (discover/offer/request/ack)
- `udp.c` — UDP sockets (working)
- `tcp.c` — TCP **stub** (headers only, no state machine)
- `dns.c` — DNS **stub** (disabled)
- `route.c` — IPv4 routing table

### `src/tasks` — Scheduler and task management

- **`scheduler.c`** — Round-robin scheduler.  `scheduler_switch()` saves
  the current task via `cpu_save()`, selects the next from the run queue,
  and restores it via `cpu_restore()`.  Zombie tasks are enqueued in
  `sch_zombie`.
- **`threads.c`** — Task creation (`task_create()`), `task_start()`,
  `task_spawn()` (launches an ELF userspace program), `task_thread()`.
  A task holds its `vmsp_t`, `fs_anchor_t`, `streamset_t`, and `net`.
- **`streams.c`** — File descriptor table (`streamset_t`).  `resx_put()`
  / `resx_get()` / `resx_remove()` manage integer handles mapping to
  typed resources (currently only `RESX_FILE`).
- **`clock.c`** — Master clock (`masterclock_t`).  Tracks wall time,
  boot time, monotonic time.  `clock_ticks()` drives the scheduler
  quantum.  `sleep_timer()` / `itimer_create()` for timed waits.
- **`elf.c`** — ELF32 userspace program loader (parses PT_LOAD segments,
  sets up VM mappings).
- **`dlib.c`** — Per-process dynamic library context (symbol table,
  list of loaded libs).

Task states: `TS_ZOMBIE → TS_BLOCKED → TS_INTERRUPTIBLE → TS_READY →
TS_RUNNING → TS_ABORTED`.

### `src/core` — Kernel glue

- **`launch.c`** — `kstart()` / `kready()` entry points, `kloader()`
  kernel thread, `module_init()` (exports kernel symbols to dlib),
  `ksys()` / `kcpu()` accessors.
- **`irq.c`** — IRQ vector table (32 slots), `irq_register()` /
  `irq_unregister()` / `irq_enter()` (dispatch), `irq_fault()` (CPU
  exception → signal delivery), IRQ semaphore (`irq_enable()` /
  `irq_disable()`).
- **`syscalls.c`** — `irq_syscall()` dispatcher.  Looks up
  `__syscalls_info[]`, logs strace output in DEBUG builds, calls the
  `sys_*` handler.
- **`scall_fs.c`** — `sys_open`, `sys_close`, `sys_opendir`,
  `sys_readdir`, `sys_pipe`, `sys_mount`, `sys_mkfs`, `sys_fstat`
- **`scall_io.c`** — `sys_read`, `sys_write`, `sys_seek`
- **`scall_mem.c`** — `sys_mmap`, `sys_munmap`, `sys_mprotect`
- **`scall_sys.c`** — `sys_ginfo`, `sys_sinfo`, `sys_xtime`
- **`scall_task.c`** — `sys_exit`, `sys_spawn`, `sys_thread`,
  `sys_sleep`, `sys_futex_*`
- **`common.c`** — `stackdump()`, misc helpers
- **`modules.c`** — Commented-out stubs (kept as reference during refactors)

---

## Testing approach

The kernel cannot run on the host without hardware stubs.  The test
strategy is to compile each subsystem as a **hosted** Linux binary linked
against minimal stubs (`tests/stub/`) that mock the MMU and IRQ.

| CLI binary | Subsystem tested | Shell scripts |
|---|---|---|
| `cli-vfs` | VFS + drivers/fs | `tests/fs_*.sh` |
| `cli-mem` | Memory (vmsp/vma/pages) | `tests/mm_*.sh` |
| `cli-net` | Network stack + ip4 | `tests/net_*.sh` |
| `cli-tsk` | Tasks / scheduler | `tests/tsk_main.sh` |

Test scripts begin with `#!/usr/bin/env cli_vfs` (or equivalent) and
issue commands to the CLI interpreter.  `make check` runs them all via
`make/check.mk`.

Coverage: `scripts/coverage.sh` uses `lcov` / `genhtml`.

---

## Architecture-dependent layer (ADL)

The kernel core calls the following functions whose implementations live
in `arch/$(target)/`:

| Function | Description |
|---|---|
| `cpu_setup(sys_info_t *)` | Platform init (MMU, APIC, IRQ, clock) |
| `arch_init()` | Post-scheduler device discovery |
| `cpu_no()` | Return current CPU index |
| `cpu_save(cpu_state_t *)` | Save context (setjmp-like) |
| `cpu_prepare(cpu_state_t *, stack, func, arg)` | Init context for new task |
| `cpu_restore(cpu_state_t *)` | Restore context (`_Noreturn`) |
| `cpu_halt()` | CPU idle loop |
| `cpu_usermode(start, stack)` | Jump to userspace |
| `mmu_enable()` / `mmu_leave()` | Switch MMU on/off |
| `mmu_context(vmsp_t *)` | Load page directory |
| `mmu_resolve(vaddr, phys, flags)` | Map virtual → physical |
| `mmu_read(vaddr)` / `mmu_drop(vaddr)` | PTE read / unmap |
| `mmu_protect(vaddr, flags)` | Change PTE flags |
| `mmu_create_uspace` / `mmu_destroy_uspace` | Allocate/free page directory |

`cpu_state_t` is defined as `size_t[8]` on i386 (enough for a setjmp
context).

---

## Module system

Drivers are built as ELF shared objects (`.ko` files, position-independent
`-fPIC`).  The kernel module loader (`kloader`) finds them on the boot
ramdisk, performs ELF relocation, and calls `module->setup()`.

A module declares itself with:
```c
EXPORT_MODULE(name, setup_fn, teardown_fn);
```

Kernel symbols are exported to be visible to modules with:
```c
EXPORT_SYMBOL(fn_name, flags);
```

The `.ksymbols` ELF section holds pointers to all `kapi_t` records;
`module_init()` walks it at boot and registers them in the kernel's
`dlib` symbol table.

---

## Memory layout (i386)

```
0x00000000 - 0x003FFFFF   Reserved / identity map
0x00400000 - 0x00FFFFFF   Kernel code + data (loaded by multiboot)
  0x0000–0x07FF           GDT (2 KB)
  0x0800–0x0FFF           IDT (2 KB)
  0x1000–0x1FFF           TSS (4 KB, 128 bytes per CPU × 32)
  0x2000–0x2FFF           Kernel PGD (page global directory)
  0x3000–0x3FFF           First kernel PTE table (identity map)
  0x4000–0x5FFF           BSP stack (2 pages)
  0x20000–...             Kernel code
0x01000000 - ...          Kernel heap / dynamic area
0x00400000 - 0xBFFFFFFF   User space (4 MB – 3 GB)
0xC0000000 - 0xFF7FFFFF   Kernel space (3 GB – ~4 GB)
```

`VMSP_MAX_SIZE` is currently 128 MB (soft limit per vmsp, not physical).

---

## Known limitations and incomplete areas

- **TCP**: Header parsing present, no connection state machine.
- **DNS / NTP**: Not implemented (stubs only).
- **SMP**: Data structures are lock-protected and SMP-ready, but
  `kready()` / multi-CPU scheduling is untested.
- **Signals**: `task_raise()` sets a bitmask; delivery of `SIGCHLD` is
  wired; full `sigaction` / `sigreturn` not implemented.
- **Multi-user / ACL**: `user_t` exists but is always `NULL` in practice;
  no permission checks.
- **Swap**: Not implemented.
- **VMA merging**: Adjacent VMAs are not merged after unmap/protect.
- **Floating point in kprintf**: Not supported.
- **x86_64 / arm64**: Only header stubs.
- **`sys_mprotect`**: Declared in syscall table but not wired.
- **`sys_opendir` as syscall**: Declared but the `SYS_OPENDIR` slot in
  the enum is not wired in `__syscalls_info[]`.
- **`modules.c`**: Nearly entirely commented out; kept as reference only.
- **`config.yml`**: Driver selection config is not yet consumed by the
  build system.

---

## Code conventions

- C99 / C11 with GCC extensions.
- All kernel-internal types end in `_t`.
- Intrusive data structure nodes (bbtree, llist) are embedded directly
  in structs and accessed via `itemof()` / `ll_each()` / `bbtree_*`.
- `splock_t` (spinlock, IRQ-masking) is the primary lock.  `mtx_t` /
  `sem_t` / `cnd_t` are used in sleeping contexts.
- Error codes use `errno` (set by callee) and negative return on failure.
- Logging: `kprintf(level, fmt, ...)` where level is one of `KL_ERR`,
  `KL_MSG`, `KL_DBG`, `KL_FSA`, `KL_BIO`, `KL_VMA`, etc.
- Kernel allocations: `kalloc()` / `kfree()`.  Virtual mappings:
  `kmap(len, ino, off, flags)` / `kunmap(ptr, len)`.
- EXPORT_SYMBOL must be called after the function definition, not before.
- Assembly files use NASM syntax (`.asm` extension for i386).

---

## Useful entry points for navigation

| Goal | Start here |
|---|---|
| Kernel boot | `arch/i386/boot/start.asm` → `arch/i386/boot/main.c` → `src/core/launch.c::kstart()` |
| Page fault handling | `src/mem/vmsp.c::vmsp_resolve()` |
| System call from userspace | `arch/i386/boot/int.asm` → `src/core/irq.c::irq_syscall()` |
| Adding a new syscall | `include/kernel/syscalls.h` (enum + prototype) + `src/core/syscalls.c` (`__syscalls_info[]`) + `src/core/scall_*.c` |
| Adding a filesystem driver | Implement `ino_ops_t`, call `vfs_addfs()` in `EXPORT_MODULE` setup |
| Adding a block device driver | Create `inode_t` via `vfs_inode(no, FL_BLK, dev, ops)`, call `vfs_mkdev()` |
| VMA page fault chain | `vmsp_resolve()` → `vma_resolve()` → `vma_ops_t::fetch()` |
| ELF loader (kernel modules) | `src/core/launch.c::kloader_open_module()` + `src/mem/dlib.c` + `src/mem/elf32.c` |
| ELF loader (userspace) | `src/tasks/elf.c` + `src/tasks/threads.c::task_spawn()` |
