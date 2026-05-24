# Tasks

**Source:** `src/tasks/`  
**Header:** `include/kernel/tasks.h`  
**Test harness:** `make cli-tsk` → `tests/tsk_main.sh`

---

## Purpose

The Tasks module manages processes and threads: their creation, context
switching, scheduling, time accounting, file descriptors, and the
userspace execution environment.

---

## Design overview

### Task (`task_t`)

The fundamental scheduling unit is a `task_t`.  It represents both a
process (with its own address space) and a kernel thread (sharing the
kernel address space).  Key fields:

| Field | Description |
|---|---|
| `pid` | Unique process ID (assigned sequentially from a BB-tree) |
| `jmpbuf` (`cpu_state_t`) | Saved CPU registers for context switch |
| `stack` | Kernel stack mapping |
| `status` | Current state (`TS_READY`, `TS_RUNNING`, `TS_BLOCKED`, etc.) |
| `vmsp` | Virtual memory space (`vmsp_t *`) |
| `fsa` | Filesystem namespace / cwd (`fs_anchor_t *`) |
| `fset` | File descriptor table (`streamset_t *`) |
| `net` | Network stack pointer |
| `parent` | Spawning task |
| `raised` | Pending signal bitmask |
| `advent_list` | Pending timer events |

Task states:

```
TS_ZOMBIE ← TS_ABORTED
    ↑              ↑
TS_READY ↔ TS_RUNNING
    ↑
TS_BLOCKED
TS_INTERRUPTIBLE
```

### Scheduler (`scheduler.c`)

The scheduler is a simple **round-robin** queue.  All ready tasks sit in
`sch_queue` (an intrusive linked list).

`scheduler_switch(status)`:
1. Saves the current task's CPU state with `cpu_save()` (arch-provided
   setjmp analogue).
2. Moves the current task to `sch_queue` (if `TS_READY`), `sch_zombie`
   (if terminating), or leaves it blocked.
3. Dequeues the next task from `sch_queue`.
4. Calls `mmu_context()` to load the new task's page directory.
5. Calls `cpu_restore()` (`_Noreturn` — restores registers and jumps back
   into the new task).

If no tasks are ready, `cpu_halt()` is called (x86 `hlt` instruction).

The scheduler is also driven by timer IRQs: `clock_ticks()` calls
`scheduler_switch(TS_READY)` at the configured `HZ` rate (100 Hz).

### Task creation (`threads.c`)

`task_create(sch, parent, name, flags)` allocates the task and, based on
`flags`, either **shares** or **clones** the parent's resources:

| Flag | Effect |
|---|---|
| `KEEP_FS` | Share the same `fs_anchor_t` |
| `KEEP_FSET` | Share the file descriptor table |
| `KEEP_VM` | Share the address space (thread) |

`task_start(name, func, arg)` creates a kernel thread (shares kernel
`vmsp`, no userspace).

`task_spawn(program, args, nodes)` loads an ELF executable into a new
address space and starts it in userspace mode.  Steps:
1. `task_create()` with a fresh `vmsp`.
2. `elf_load()` maps the ELF PT_LOAD segments into the new space.
3. A `task_params_t` is built with the entry point and argument array.
4. The task is started via `task_usermode()` which calls `cpu_usermode()`
   (arch-provided jump to ring 3).

`task_thread(name, entry, params, len, flags)` creates a userspace thread
inside the current process's address space.

### File descriptors (`streams.c`)

The `streamset_t` holds an array of typed resource slots:
```c
resx_put(strms, type, data, close_fn)  → int handle
resx_get(strms, type, handle)          → data pointer
resx_remove(strms, handle)
```

Currently only `RESX_FILE` (wrapping a `file_t` = inode + offset +
open-flags) is used.  The handle number (file descriptor) is the array
index.  `stream_create_set()` allocates a new table;
`stream_open_set()` increments the refcount (shared); `stream_close_set()`
decrements and frees when zero.

### Clock (`clock.c`)

The `masterclock_t` tracks:
- `wall` — real-world time (set from RTC at boot, adjustable)
- `boot` — time since power-on
- `monotonic` — never-decreasing clock

`clock_ticks(elapsed)` is called from the timer IRQ.  It advances all
clocks, checks the `advent` tree for expired timers, and switches the
scheduler at the quantum boundary.

**Timers / advents:**  An `advent_t` is a one-shot or repeating timer
event.  It is inserted into both a linear `advent_list` and a sorted
BB-tree.  When the wall time passes `advent->until`, `advent->wake()` is
called (typically `scheduler_add()` to wake a sleeping task).

`sleep_timer(timeout)` blocks the current task until the timeout expires.
`futex_wait/wake/requeue` build on the same mechanism for userspace
synchronisation primitives.

### ELF loader (`elf.c`)

Parses an ELF32 binary from a `file_t`, validates the header, iterates
`PT_LOAD` segments, and calls `vmsp_map()` for each one with the
appropriate flags (text → `VM_RX`, data → `VM_RW`, etc.).  Sets up BSS
as anonymous zero pages.

---

## Signal handling

`task_raise(task, signum)` sets bit `signum` in `task->raised`.
`SIGCHLD` is raised on the parent when a child exits.

> **Incomplete:** Signal delivery (checking `raised` before returning to
> userspace, executing the handler, `sigreturn`) is not implemented.
> `sigaction` and `sigreturn` syscalls are absent from the dispatch table.

---

## Known issues

- Signal delivery is not implemented (ROADMAP TASKS-1).
- Zombie reaping (`sch_zombie` list) never frees task memory (TASKS-2).
- Elapsed time accounting in `scheduler_switch` is TODO (TASKS-3).
- SMP: the scheduler lock is in place but multi-CPU testing has not been
  done (TASKS-4).
- `waitpid` / child exit status collection syscall is missing (TASKS-6).
- The entry-point remapping for userspace tasks is incomplete (TASKS-5).
