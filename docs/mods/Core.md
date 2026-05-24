# Core

**Source:** `src/core/`  
**Headers:** `include/kernel/core.h`, `include/kernel/irq.h`,
`include/kernel/syscalls.h`, `include/kernel/mods.h`

---

## Purpose

`src/core` is the glue layer of the kernel.  It provides:

1. The **boot entry points** that initialise every subsystem in the
   right order.
2. The **IRQ and fault dispatch** mechanism.
3. The **syscall dispatch** table that routes userspace calls to `sys_*`
   handlers.
4. The **module loader** that finds, links, and starts `.ko` kernel
   modules from the boot ramdisk.
5. The **system/CPU information** accessors used by other subsystems.

---

## Boot sequence (`launch.c`)

### `kstart()` — primary CPU only

Called by the architecture startup code after the MMU and early hardware
are configured.  Sequence:

```
kstart()
  ├── irq_reset(false)        — disable interrupts
  ├── cpu_setup(&sysinfo)     — arch-specific CPU + hardware init
  ├── clock_init(rtc_time())  — initialise master clock
  ├── module_init()           — register kernel symbols in dlib
  ├── vfs_init()              — create global VFS share
  ├── scheduler_init()        — initialise scheduler and task tree
  ├── net_setup()             — create kernel network stack
  ├── arch_init()             — discover and probe devices (PCI, serial…)
  ├── task_start("kloader", kloader, NULL)  — spawn loader thread
  └── irq_zero()              — enter scheduler / idle loop
```

### `kready()` — secondary CPUs

Calls `cpu_setup()` for the local CPU, then spins until `sysinfo.is_ready`
is set by the primary CPU, then calls `irq_zero()` to enter the
scheduler.

### `kloader()` — kernel loader thread

Runs as a kernel task.  Steps:
1. Scans `/mnt/boot0`, `/mnt/boot1`, … for `.ko` files and loads them.
2. Mounts the CD-ROM as root (`sdc` → iso).
3. Mounts `devfs` at `/dev`.
4. `task_spawn()`s the first userspace program (`krish`).
5. Loops sleeping — serves as a cleanup/idle task.

### `module_init()`

Walks the `.ksymbols` ELF section of the kernel image, which contains
`kapi_t **` pointers to every `EXPORT_SYMBOL()` declaration.  Registers
each symbol into the kernel's `dlib` symbol table so that dynamically
loaded `.ko` modules can resolve them.

---

## IRQ management (`irq.c`)

### IRQ semaphore

Each CPU has an `irq_semaphore` counter in `cpu_info_t`.  This
implements a recursive disable/enable pair:
- `irq_disable()` — mask IRQs + increment counter
- `irq_enable()` — decrement counter; unmask when it reaches zero
- `irq_reset(enable)` — unconditionally set state

`might_sleep()` asserts that IRQs are not masked (used as a sanity check
before any operation that can block).

### IRQ vectors

32 IRQ slots (`irqv[0..31]`).  Each slot holds a linked list of
`irq_record_t` (handler + data pointer).  Multiple handlers can share a
single IRQ number.

- `irq_register(no, func, data)` / `irq_unregister(no, func, data)` — add/remove handlers.
- `irq_enter(no)` — called by the architecture interrupt stub; iterates
  all registered handlers for that vector.

### Fault handling

`irq_fault(name, signum)` is called for CPU exceptions (page fault,
general protection, divide by zero, …).  If a task is active it delivers
`signum` via `task_raise()`; otherwise it halts the CPU.

---

## Syscall dispatch (`syscalls.c`)

The global table `__syscalls_info[]` maps each `SYS_*` number to:
- a function pointer `scall`
- a human-readable name (for strace logging)
- argument type annotations (for logging: `ARG_STR`, `ARG_INT`,
  `ARG_PTR`, `ARG_FD`, `ARG_LEN`, `ARG_FLG`)
- a `split` value (number of input vs output args for strace formatting)

`irq_syscall(no, a1..a5)` looks up the entry, optionally logs a strace
line in `DEBUG` builds, calls the handler, and logs the return value.

Syscall handlers live in:

| File | Syscalls handled |
|---|---|
| `scall_io.c` | `read`, `write`, `seek` |
| `scall_fs.c` | `open`, `close`, `opendir`, `readdir`, `pipe`, `mount`, `mkfs`, `fstat` |
| `scall_mem.c` | `mmap`, `munmap`, `mprotect` |
| `scall_sys.c` | `ginfo`, `sinfo`, `xtime` |
| `scall_task.c` | `exit`, `spawn`, `thread`, `sleep`, `futex_wait/requeue/wake` |

All handlers validate userspace pointers with `vmsp_check()` /
`vmsp_check_str()` before dereferencing them.

---

## Module system (`include/kernel/mods.h`)

### Declaring a module

```c
// In the driver's main .c file:
static void my_setup()   { /* register devices, filesystems, … */ }
static void my_teardown(){ /* cleanup */ }
EXPORT_MODULE(mydrv, my_setup, my_teardown);
```

This creates a `kmodule_t kmodule_mydrv` symbol.  The loader scans for
symbols whose names start with `kmodule_` and calls `setup()`.

### Exporting kernel symbols to modules

```c
EXPORT_SYMBOL(vfs_inode, 0);   // after the function definition
```

Places a `kapi_t *` pointer in the `.ksymbols` section.  Modules linked
with the kernel's `dlib` symbol table can then call `vfs_inode()` by
name after relocation.

---

## System info accessors

```c
sys_info_t *ksys();   // global system info (CPU count, uptime, arch)
cpu_info_t *kcpu();   // current CPU's info (mode, ticks, irq_semaphore)
```

`cpu_info_t` tracks the current execution mode (`KMODE_SYSTEM`,
`KMODE_IRQ`, `KMODE_USER`, `KMODE_IDLE`) and per-CPU tick counters.

---

## Known issues

- `modules.c` is almost entirely commented-out stub code kept as reference.
- Module `setup()` runs directly in the `kloader` thread context; if a
  module panics it brings down the loader (TODO: isolate per-module).
- Module dependency ordering (checking that required symbols are loaded
  before `setup()`) is not implemented.
- `sys_opendir` is in the `SYS_*` enum and has a `sys_opendir()`
  function in `scall_fs.c` but is missing from `__syscalls_info[]`.
- `sys_mprotect` is listed in the enum but has no `__syscalls_info[]` entry.
