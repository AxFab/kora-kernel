# KoraOS Kernel — ROADMAP

This document catalogues the known issues, incomplete features, and
architectural gaps in the kernel, grouped by subsystem.  Items are
ordered roughly by priority / blocking dependency.

The goal stated in `README.md` is: **run basic userspace programs
(logon, desktop, krish, folder, lua) with at least one full-featured
writable filesystem**.

---

## Critical / Blocking

These items must be resolved before the kernel can run any real userspace
program reliably.

### TASKS-1 — Signal delivery is not implemented

`task_raise()` only sets a bit in `task->raised`.  There is no code that
actually delivers pending signals to a task before it returns to
userspace, and `sigaction` / `sigreturn` syscalls are missing entirely
from `__syscalls_info[]`.

Impact: processes cannot be killed, `SIGCHLD` is set but never delivered,
`waitpid` cannot work.

Files: `src/tasks/threads.c`, `src/core/scall_task.c`,
`include/kernel/syscalls.h`.

---

### TASKS-2 — `task_stop()` / zombie reaping is incomplete

`scheduler_switch(TS_ZOMBIE)` enqueues the task in `sch_zombie` but
nothing ever dequeues and frees those tasks.  Memory and resources leak
on every process exit.

Files: `src/tasks/scheduler.c`, `src/tasks/threads.c`.

---

### MEM-1 — `valloc()` has a logic bug

In `src/stdc/heap.c`, the function `valloc()` contains:
```c
assert(map != NULL && map == (void *) - 1);
```
This assertion is always false: it asserts the mapping both succeeds
(`!= NULL`) and fails (`== (void*)-1`).  The `valloc` path is therefore
dead.  Should be `!= (void*)-1`.

File: `src/stdc/heap.c`.

---

### MEM-2 — Page bitmap zones are allocated with `kalloc` before the heap exists

`page_range()` in `src/mem/pages.c` calls `kalloc()` to allocate
`mzone_t` structures.  At the time `mmu_setup()` runs, the heap is not
yet initialised on a fresh i386 boot — the initial heap area is
identity-mapped manually and `kalloc` relies on `kmap()` which requires
the MMU to be configured.  This chicken-and-egg issue needs an explicit
static zone for early allocations.

File: `src/mem/pages.c`, `arch/i386/boot/mmu.c`.

---

### VFS-1 — `fnode_t` scavenging policy is undefined

Comment in `src/vfs/fnode.c`: *"RCU+LRU mechanisms, but note that
scavenge policy haven't been properly defined yet."*  `vfs_scavenge()`
exists but the LRU list is never pruned automatically.  Under memory
pressure the fnode cache will grow without bound.

Files: `src/vfs/fnode.c`, `src/vfs/device.c`.

---

### VFS-2 — Device release is incomplete

`vfs_rmdev()` in `src/vfs/device.c` has `// TODO -- Remove all devices`.
Teardown of a device with active inodes will corrupt internal state.

File: `src/vfs/device.c`.

---

## Network

### NET-1 — TCP has no state machine

`drivers/net/ip4/tcp.c` contains header parsing and port allocation but
no connection state machine (SYN/SYN-ACK/ACK, data transfer, FIN/RST).
TCP sockets cannot be used.

File: `drivers/net/ip4/tcp.c`.

---

### NET-2 — DNS is a stub

`drivers/net/ip4/dns.c` exists but DNS resolution is not implemented.
It is commented out in the README as ~~DNS~~.

File: `drivers/net/ip4/dns.c`.

---

### NET-3 — NTP is not implemented

Time synchronisation over the network is mentioned in docs but no code
exists.

---

### NET-4 — DHCP acknowledgement handling is incomplete

Multiple `// TODO` in `drivers/net/ip4/dhcp.c` around lease validation,
the proposal state check, and the DHCP DECLINE path.  The kernel can
receive an OFFER but the full four-way handshake is fragile.

File: `drivers/net/ip4/dhcp.c`.

---

### NET-5 — ARP lacks protection

`drivers/net/ip4/arp.c` has open TODOs:
- No ARP poisoning protection
- No subnet membership check before responding
- No unsolicited ARP reply detection

File: `drivers/net/ip4/arp.c`.

---

## Memory

### MEM-3 — VMA merging is not implemented

After `vmsp_unmap()` or `vmsp_protect()`, adjacent VMAs with compatible
flags are never merged.  Over time, heavily forking or remapping programs
will fragment the VMA tree with many small nodes.

File: `src/mem/vmsp.c` (TODOs at lines ~455, ~494).

---

### MEM-4 — Copy-on-write for `VMA_SHDANON` is commented out

The shared anonymous VMA type (`VMA_SHDANON`) was planned for IPC shared
memory but its implementation is commented out.  `VM_SHARED` flag is
not usable.

File: `src/mem/vma.c`.

---

### MEM-5 — Heap `free_list` is a single list

The arena allocator uses a single linked free-list per arena, which has
`O(n)` search.  Multiple free-lists by size class (`// TODO multi free
list` in `src/stdc/arena.c`) would improve allocation speed.

File: `src/stdc/arena.c`.

---

### MEM-6 — Page sharing / COW tracking uses a hash map without eviction

`page_sharing_t` uses a `hmap_t` to track shared pages for COW.
There is no eviction or cleanup path when pages are freed after a fork
and the mapping is split.

File: `src/mem/vmsp.c`, `src/mem/vma_anon.c`.

---

## VFS / Filesystem

### VFS-3 — ext2 large file support is broken (>12 direct blocks)

The README / TESTING.md notes: *"I have some known issue on file larger
than 12 blocks (ext2 specific)."*  Indirect block handling in
`drivers/fs/ext2/blocks.c` needs verification and testing.

File: `drivers/fs/ext2/blocks.c`.

---

### VFS-4 — VFAT long filename (LFN) support is missing

Multiple TODOs in `drivers/fs/vfat/diterator.c` note that long name
matching is not implemented — only 8.3 short names are compared.
Writing LFN entries is also stubbed out.

File: `drivers/fs/vfat/diterator.c`.

---

### VFS-5 — VFAT directory cluster extension is not implemented

`// TODO - alloc cluster` / `// TODO - Got next cluster...` in
`drivers/fs/vfat/diterator.c`.  Creating more files than fit in an
existing directory cluster will fail silently.

File: `drivers/fs/vfat/diterator.c`.

---

### VFS-6 — VFAT format calculation is incomplete

`drivers/fs/vfat/format.c` has several TODOs including FAT32 sector
count and the case where two computed values collide.

File: `drivers/fs/vfat/format.c`.

---

### VFS-7 — ext2 rollback on error is missing

`drivers/fs/ext2/ext2.c` has multiple `// TODO -- Rollback` comments
around `link`, `unlink`, and `create` operations.  A crash or error
mid-operation can leave the filesystem in an inconsistent state.

File: `drivers/fs/ext2/ext2.c`.

---

### VFS-8 — Block cache race condition on page allocation

`src/vfs/block.c` line ~71:
```c
// TODO -- Race condition, is page_mutex released !?
```
The page fetch path may race when two threads fault on the same file
page simultaneously.

File: `src/vfs/block.c`.

---

### VFS-9 — Partition scheme integration is hard-coded

From `docs/VirtualFileSystem.md`: *"MBR and GPT are supported but only
hard-plugged into the VFS after block device creation."*  `src/vfs/parts.c`
is called directly rather than going through a pluggable partition layer.
This prevents stacking (software RAID, encryption modules).

File: `src/vfs/parts.c`, `src/vfs/device.c`.

---

### VFS-10 — `sys_opendir` syscall slot is not wired

`SYS_OPENDIR` appears in the enum in `include/kernel/syscalls.h` but
has no entry in `__syscalls_info[]` in `src/core/syscalls.c`.  Userspace
cannot open directories via the system call interface.

File: `src/core/syscalls.c`.

---

## Scheduler / Tasks

### TASKS-3 — Elapsed time accounting is not implemented

`src/tasks/scheduler.c`:
```c
// TODO -- Register elapsed time !
```
The per-task CPU time counters (`elapsed_counters[CKS_*]`) are declared
but never updated during a context switch.

File: `src/tasks/scheduler.c`.

---

### TASKS-4 — SMP scheduling is untested

`kready()` exists and the scheduler queue uses a spinlock, but no
testing or validation has been done with more than one CPU.  APIC
inter-processor interrupts for scheduler preemption are not sent.

Files: `src/core/launch.c`, `src/tasks/scheduler.c`.

---

### TASKS-5 — `task_spawn` entry-point remapping is incomplete

In `src/tasks/threads.c::task_usermode()`:
```c
if (info->start) {
    // TODO -- Find info->func (need remap?)
}
```
The userspace program's entry point is read directly from the ELF
struct rather than through the task's own address space mapping.

File: `src/tasks/threads.c`.

---

### TASKS-6 — No `waitpid` / child status collection syscall

`sys_wait` is referenced in comments but absent from the syscall table.
A parent that `spawn()`s a child has no way to collect its exit status.

File: `include/kernel/syscalls.h`, `src/core/scall_task.c`.

---

## Security / Multi-user

### SEC-1 — No permission checks anywhere

`user_t` is passed through VFS operations but every call in the kernel
passes `NULL` for `user`.  `vfs_access()` exists but is never called
from system calls.  The kernel effectively runs as root with no
sandboxing.

Files: `src/core/scall_*.c`, `src/vfs/fnode.c`.

---

### SEC-2 — `vmsp_check` / `mspace_check` is not used consistently

`vmsp_check_str()` is called before some string arguments in syscalls
but not all pointer arguments are validated.  A malicious userspace
pointer could dereference kernel memory.

Files: `src/core/scall_fs.c`, `src/core/scall_io.c`.

---

## Architecture / Portability

### ARCH-1 — x86_64 and arm64 ports are stubs

`arch/x86_64/` contains only two header files.  `arch/arm64/` contains
only `make.mk`.  Neither can be built.

---

### ARCH-2 — `config.yml` is not consumed by the build system

`config.yml` lists driver enable/disable flags, but the Makefile does not
read it.  Driver selection is done by editing the `DRV` list in the
Makefile directly.

File: `Makefile`, `config.yml`.

---

### ARCH-3 — `modules.c` is nearly entirely dead code

`src/core/modules.c` is almost entirely commented out.  It holds
reference comments for the ADL contract but adds noise to the build.
Should either be cleaned up into a proper reference document or removed.

File: `src/core/modules.c`.

---

## Documentation

### DOC-1 — `docs/MemoryManagment.md` references old API

The document uses `mspace_t` (old name) instead of `vmsp_t` (current),
and `memory_map()` instead of `vmsp_map()`.  Several code snippets are
out of date.

File: `docs/MemoryManagment.md`.

---

### DOC-2 — `docs/Structures.md` is a title-only skeleton

Every entry is just a heading with no content.

File: `docs/Structures.md`.

---

### DOC-3 — `CONTRIBUTING.md` and `DESIGN.md` are essentially empty

`CONTRIBUTING.md` has section headings and no content.  `DESIGN.md` is
a rough bullet list.

Files: `CONTRIBUTING.md`, `DESIGN.md`.

---

### DOC-4 — `docs/Home.md` is a placeholder

Single line of text.

File: `docs/Home.md`.

---

## Minor / Polish

| ID | File | Note |
|---|---|---|
| MIN-1 | `src/stdc/hmap.c:60` | Hash function works only on little-endian; comment says so |
| MIN-2 | `src/stdc/time.c:242` | `FIXME use local` — timezone handling is wrong |
| MIN-3 | `src/stdc/format_vfprintf.c:233` | `FIXME change arg position` — positional format args broken |
| MIN-4 | `src/vfs/pipe.c:299` | Pipe size is hardcoded to `PAGE_SIZE`; auto-extend to 64 KB is noted but not done |
| MIN-5 | `drivers/pc/ata/ata.c:289` | `// TODO -- use parameter to know which bus` — ATA bus selection is hard-coded |
| MIN-6 | `drivers/pc/e1000/e1000.c:101` | TX semaphore not used; may overflow TX ring under heavy load |
| MIN-7 | `drivers/pc/ps2/ps2.c:61` | `// TODO - Remove this hack!` — PS/2 initialisation contains a workaround |
| MIN-8 | `src/vfs/block.c:326` | Entire page is marked dirty even for sub-page writes |
| MIN-9 | `src/core/launch.c:122` | Module `setup()` runs in the loader context; isolation noted as TODO |
| MIN-10 | `src/mem/dlib.c:270` | Symbol relocation loop `while (base == 0 && true)` is an infinite loop if relocation fails |

---

## Milestone targets

### v0.1 — "Can run krish"

- [ ] Signal delivery (TASKS-1)
- [ ] Zombie reaping (TASKS-2)
- [ ] `waitpid` (TASKS-6)
- [ ] Fix `valloc` bug (MEM-1)
- [ ] `sys_opendir` wired (VFS-10)
- [ ] ext2 large file (VFS-3)
- [ ] VFAT LFN support (VFS-4, VFS-5)

### v0.2 — "Stable userspace"

- [ ] TCP state machine (NET-1)
- [ ] Full permission checks (SEC-1)
- [ ] VMA merging (MEM-3)
- [ ] Fnode scavenging (VFS-1)
- [ ] Elapsed time accounting (TASKS-3)

### v0.3 — "Multi-user + networking"

- [ ] DNS (NET-2)
- [ ] NTP (NET-3)
- [ ] ACL enforcement (SEC-1 full)
- [ ] SMP validation (TASKS-4)
- [ ] x86_64 port (ARCH-1)
