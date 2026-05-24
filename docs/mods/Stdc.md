# Stdc — Kernel Standard Library

**Source:** `src/stdc/`  
**Headers:** `include/kernel/stdc.h`, `include/kora/`, `include/string.h`,
`include/ctype.h`

---

## Purpose

The kernel cannot link against a normal C standard library — there is no
operating system to provide `malloc`, `printf`, or `pthread_mutex_lock`.
`src/stdc` implements the subset of these that the kernel needs,
built on top of the two low-level primitives it does have: `kmap()` for
page-mapped memory and architecture spinlock instructions.

This module is compiled into every build target (kernel image and all CLI
test harnesses).

---

## Components

### Memory allocation — heap and arena (`heap.c`, `arena.c`, `allocator.h`)

The allocator is a **multi-arena free-list heap**.

- An **arena** is a `kmap()`-ed contiguous memory region (default 2 MiB).
  Each arena can carve out small allocations up to a configurable chunk
  limit (default 16 KiB).
- Allocations larger than the chunk limit get their own dedicated arena
  mapped at exactly the requested size.
- Inside an arena, free blocks are maintained in a **singly linked
  free-list** sorted by address.  On `free`, adjacent free blocks are
  coalesced.

Public API (from `include/kernel/stdc.h`):

```c
void *kalloc(size_t len);     // allocate, zero-fills in debug builds
void  kfree(void *ptr);       // release

// Debug build extras:
void *kalloc_(size_t len, const char *tag);  // tagged allocation
char *kstrdup(const char *str);
char *kstrndup(const char *str, size_t max);
```

`kmap(len, ino, off, flags)` and `kunmap(ptr, len)` are the lower-level
primitives (implemented in `src/mem/`) that the arena uses internally.

### String manipulation (`string.c`, `mbstr.c`)

Standard routines: `memcpy`, `memset`, `memcmp`, `memmove`, `strlen`,
`strcmp`, `strncmp`, `strcpy`, `strncpy`, `strcat`, `strncat`,
`strchr`, `strrchr`, `strstr`, `strtok_r`, `strtol`, `strtoul`, `atoi`,
`itoa`.

`mbstr.c` provides multi-byte (UTF-8) helpers for character classification
and conversion.

`sztoa()` / `sztoa_r()` convert a 64-bit size to a human-readable string
(e.g. `"4 KiB"`).

### Formatting (`format_integer.c`, `format_print.c`, `format_scanf.c`,
`format_vfprintf.c`, `format_vfscanf.c`)

A `FILE`-based formatting core with a generic `vfprintf` / `vfscanf`
implementation.  Building on these:

```c
int snprintf(char *buf, size_t n, const char *fmt, ...);
int vsnprintf(char *buf, size_t n, const char *fmt, va_list ap);
void kprintf(klog_t level, const char *fmt, ...);   // kernel log
```

`kprintf` routes output to the architecture's `kwrite()` function (VGA
console + serial on i386) and filters by log level.

Log levels (`klog_t`): `KL_ERR`, `KL_MSG`, `KL_DBG`, `KL_PF`
(page-fault), `KL_IRQ`, `KL_MAL` (allocation), `KL_INO` (inode),
`KL_FSA` (filesystem), `KL_BIO` (block I/O), `KL_VMA` (VMA events).

> **Limitation:** Floating-point conversion (`%f`, `%e`, `%g`) is not
> implemented.  Positional format arguments have a known bug
> (`format_vfprintf.c:233: FIXME change arg position`).

### Linked list (`include/kora/llist.h`, implicit in `src/stdc/`)

Intrusive doubly-linked list.  Nodes are embedded directly in the
containing struct:

```c
struct foo { int x;  llnode_t node; };
llhead_t head = INIT_LLHEAD;

ll_append(&head, &item->node);
ll_remove(&head, &item->node);
for ll_each(&head, item, struct foo, node) { … }
```

Helpers: `ll_first`, `ll_last`, `ll_next`, `ll_prev`, `ll_enqueue`,
`ll_dequeue`, `ll_pop_front`, `ll_pop_back`.

### Balanced binary tree (`bbtree.c`, `include/kora/bbtree.h`)

Intrusive weight-balanced binary tree (BB-tree).  Keys are `size_t`.
Used throughout for VMAs (address key), tasks (PID key), inodes
(inode-number key), protocols (AF key), and advents (timestamp key).

```c
bbtree_t tree;
bbtree_init(&tree);
bbtree_insert(&tree, &node);                  // node.value_ = key
bbtree_search_eq(&tree, key, struct T, node); // exact lookup
bbtree_search_le / search_ge                  // floor / ceiling
bbtree_remove(&tree, key);
bbtree_first / bbtree_last / bbtree_next / bbtree_prev
```

### Hash map (`hmap.c`, `include/kora/hmap.h`)

Open-addressing hash map with string keys.  Used by VFS for fnode child
lookup and filesystem type registry.

```c
hmap_t map;
hmap_init(&map, initial_bits);
hmap_put(&map, key, value);
hmap_get(&map, key);
hmap_remove(&map, key);
```

> **Note:** The hash function only works correctly on little-endian
> machines (`hmap.c:60: TODO`).

### Spinlocks and read-write locks (`include/kora/splock.h`, `include/kora/rwlock.h`, `lock.c`)

`splock_t` — a recursive spinlock that **masks IRQs** while held.  This
is the primary synchronisation primitive in the kernel.

```c
splock_lock(&lock);
splock_unlock(&lock);
bool splock_locked(&lock);   // assert helper
```

`rwlock_t` — allows multiple concurrent readers; writers exclude all.
Masks IRQs only on write.

### POSIX-like synchronisation (`sem.c`, `mtx.c`, `cnd.c`)

For contexts where blocking is permissible (inside tasks, not in IRQ
handlers):

```c
sem_t sem;   sem_init(&sem, value); sem_wait(&sem); sem_release(&sem);
mtx_t mtx;   mtx_init(&mtx, mtx_plain); mtx_lock(&mtx); mtx_unlock(&mtx);
cnd_t cnd;   cnd_init(&cnd); cnd_wait(&cnd, &mtx); cnd_signal(&cnd);
```

These are implemented using `futex_wait` / `futex_wake`, which in turn
use scheduler primitives to sleep the calling task.

### Bitmap operations (`bits.c`)

Bit-array helpers used by the physical page allocator:

```c
void bitsset(uint8_t *ptr, int start, int count);
void bitsclr(uint8_t *ptr, int start, int count);
int  bitschrz(uint8_t *ptr, int len);   // find first zero run
bool bitstest(uint8_t *ptr, int start, int count);
```

### Block map helper (`blkmap.c`, `include/kernel/blkmap.h`)

`bkmap` / `bkunmap` — maps a disk block number to a kernel virtual
address using `kmap()`.  Used by filesystem drivers (ext2, vfat) to
access on-disk structures without manual page-alignment arithmetic.

### Time utilities (`time.c`, `include/kora/time.h`)

`xtime_t` is a 64-bit microsecond timestamp.  Helpers:
- `xtime_read(name)` — read `XTIME_WALL`, `XTIME_BOOT`, or
  `XTIME_CLOCK` from the master clock.
- Conversion macros: `SEC_TO_USEC`, `MSEC_TO_USEC`, `NSEC_OF_USEC`, etc.
- `strftime`-style formatting functions (partial).

### Random number generation (`random.c`)

`rand8()`, `rand16()`, `rand32()`, `rand64()` — simple LCG PRNG.  Used
for UUID generation in device registration and ephemeral port allocation.

### Debug utilities (`debug.c`, `error.c`)

`stackdump(frame)` — walks the call stack and prints symbol names
(requires debug symbols in the kernel image).

`kwrite(buf)` — raw string output bypassing `kprintf` formatting.

---

## Known issues

- Floating-point formatting not implemented.
- Hash map (`hmap_t`) is endian-sensitive.
- Heap free-list is single per arena; multi-bucket sizing is a TODO.
- `valloc()` in `heap.c` contains a logic bug (assert condition inverted).
- Timezone handling in `time.c` is marked `FIXME`.
