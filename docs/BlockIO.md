# Block I/O Layer — State, Critique, and Redesign Proposal

This document covers the full block I/O stack as it exists today: from raw
hardware I/O inside device drivers up through the page cache, the async I/O
scheduler stub, and the two competing block-mapping abstractions used by
filesystem drivers.  It then proposes a cleaner layered design.

---

## 1. Current Architecture Overview

```
 ┌─────────────────────────────────────────────────────────────┐
 │  Consumers                                                  │
 │  vma_file.c  (page-fault)   elf32.c (ELF load)   vfs_read  │
 └────────────┬───────────────────────┬────────────────────────┘
              │  vfs_fetch_page       │  blk_map / kmap(ino)
              ▼                       ▼
 ┌─────────────────────────────────────────────────────────────┐
 │  Page Cache  (src/vfs/block.c)                              │
 │  block_file_t  →  BB-tree of block_page_t  +  LRU list      │
 │  block_fetch / block_release / block_read / block_write     │
 └────────────────────────────┬────────────────────────────────┘
                              │  ino->ops->read / write
                 ┌────────────┼────────────┐
                 ▼            ▼            ▼
           ATA PIO         tarfs        ext2/vfat
           (pata.c)      (tar_read)    (read via bkmap)
```

There are three distinct access paths into the page cache, each with different
semantics.  They are not cleanly unified, and the async I/O scheduler that
should connect them is entirely non-functional.

---

## 2. Layer by Layer

### 2.1 Block Device Drivers (`drivers/pc/ata/`)

The ATA driver exposes two `ino_ops_t` tables:

- `ata_ops_pata` — PIO mode read/write for PATA disks
- `ata_ops_patapi` — ATAPI (CD-ROM) variant

Both `read` and `write` functions take a byte **offset** and a byte **length**,
convert to 512-byte sector counts, then loop calling `ata_poll()` and
`insw`/`outsw` synchronously.

Key observations:

- **Busy-wait polling.** `ata_poll()` spins on the ATA status register.  The
  CPU is held while the disk head seeks and the platter rotates.  No IRQ
  completion, no sleep.

- **No mutual exclusion.** There is no spinlock or mutex around the drive.  If
  two tasks perform concurrent I/O to the same physical drive, the `outb`
  sequences interleave and the drive receives corrupted commands.

- **Retry logic is hand-rolled.** Each driver implements its own 2–3 retry
  loop without a shared policy.

- **No DMA.** Bus-master DMA (BMIDE) registers exist in the hardware and are
  supported by QEMU but the driver never programs them.

- **IRQ is registered but ignored.** `ata_probe()` calls `irq_register` for
  IRQs 14 and 15, but the interrupt handler only prints a message and sends
  EOI.  The read/write path never sleeps on an IRQ semaphore.

---

### 2.2 Page Cache (`src/vfs/block.c`)

Every `FL_REG` or `FL_BLK` inode gets a `block_file_t` attached to
`ino->fl_data` when it is first created (`vfs_createfile` → `block_create`).
This happens for **all** regular inodes, including in-memory filesystems like
tarfs that have no need for a page cache.

#### Data Structures

```c
struct block_file {
    bbtree_t   tree;   // BB-tree of block_page_t indexed by page number
    splock_t   lock;
    llhead_t   llru;   // LRU eviction list
    bool       async;  // always false in practice
};

struct block_page {
    bool       ready;    // data has been read from driver
    bool       dirty;    // data has been written and not synced
    bool       in_ops;   // async I/O in progress
    bbnode_t   node;     // BB-tree node; value_ = page index (off / PAGE_SIZE)
    atomic_int rcu;      // reference count
    size_t     phys;     // physical address of the backing page
    mtx_t      mtx;      // serialises fill / writeback
    cnd_t      cnd;      // wakeup for async completion
    llnode_t   nlru;     // LRU list linkage
};
```

#### Read Path

```
block_read(ino, buf, len, off)
  └─ block_fetch(ino, page_aligned_off, blocking=true)
       └─ block_get(ino, off, create=true)   → finds or creates block_page_t
       └─ block_fill(ino, page)              → fills page from driver
            ├─ [async]  bio_create + bio_request + bio_push + bio_wait  [STUB]
            └─ [sync]   kmap(PAGE_SIZE, NULL, 0, VMA_PHYS)
                        mmu_read(ptr) → page->phys
                        ino->ops->read(ino, ptr, PAGE_SIZE, off, 0)
                        kunmap(ptr, PAGE_SIZE)
  └─ kmap(PAGE_SIZE, NULL, page->phys, VMA_PHYS)  → map physical page
  └─ memcpy(buf, mapped + offset_within_page, cap)
  └─ kunmap + block_release
```

The sync path has a design quirk: `kmap(PAGE_SIZE, NULL, 0, VMA_PHYS)` passes
physical address 0.  The intent (based on the commented-out `page_new()` call)
is that the kernel allocates a fresh physical page here.  The physical address
is then recovered with `mmu_read`.  This round-trip is fragile — it depends on
the VM layer treating offset=0 with `VMA_PHYS` as "allocate anonymous" rather
than "map physical page 0".

#### Write Path

Write goes through `block_write` → `block_fetch` (to load the existing page
first for partial-page writes) → modify in-place → `block_release(dirty=true)`
→ at release time, `block_writeback` → `ino->ops->write`.  The writeback
happens when the last reference is dropped, not on a timer or explicit sync.

#### LRU Eviction

Pages with `rcu==0` and no dirty flag are enqueued into `block->llru`.
`block_scavenge(block, max)` dequeues from the tail and calls `page_release`.
It does not check `page->in_ops` before releasing, which is a potential race
with in-flight async I/O.  Eviction is never called proactively — no memory
pressure hook exists.

---

### 2.3 Async I/O Scheduler (`src/vfs/bio.c`)

This file is almost entirely non-functional.  The intended design is visible
but none of it is wired up:

| Function | State |
|---|---|
| `bio_create(ino)` | Empty — returns nothing (undefined behaviour) |
| `bio_request(bio, page, lba, cnt, flags)` | Empty |
| `bio_push(bio)` | Calls `algo->push` on `ino->drv_data`, which is never an `bio_algo_t` |
| `bio_wait(bio)` | Empty — declared `int` in `block.c`, defined `void` here |
| `bio_deamon(ino)` | Defined but never started as a task |
| `bio_deadline_push` | Commented out (the only scheduler implementation) |

`block_file_t.async` is always `false`.  The async branch in `block_fill` and
`block_writeback` is never exercised.

The `bio_deadline_t` scheduler struct shows good intent: separate read and
write queues with deadline expiry, a background daemon task, mutex+cond
sleeping.  But the push function is commented out, the pop function has an
infinite-wait loop that references a never-populated `sched->head`, and the
daemon is never spawned.

---

### 2.4 Two Competing Block-Mapping Abstractions

There are two unrelated APIs that both let driver/fs code "map a block of data
from a device into memory".  They are incompatible and used in different parts
of the codebase.

#### `blkmap_t` — Heap object, sliding window (ELF loader, `src/stdc/blkmap.c`)

```c
blkmap_t *bkm = blk_open(ino, blocksize);
void *ptr = blk_map(bkm, block_no, VM_RD);   // kmap(msize, ino, off, flags)
blk_close(bkm);
```

This goes through `kmap` with an inode, which takes the VM page-fault path
→ `vma_file.c` → `vfs_fetch_page` → `block_fetch`.  It properly uses the page
cache and reference counting.  Intended for consumers that keep a long-lived
sliding view of a file (ELF section tables, string tables).

#### `bkmap` struct — Stack-allocated, one block at a time (ext2, vfat)

```c
struct bkmap bm;
uint8_t *ptr = bkmap(&bm, block_no, blocksize, 0, blkdev, VM_RD);
// ... use ptr ...
bkunmap(&bm);
```

This is a completely different type (`struct bkmap`, not `blkmap_t`), defined
inside the driver's own headers.  Looking at ext2's usage, `bkmap` calls
directly to the device's read function — **bypassing the page cache entirely**.
Every `bkmap` call issues a fresh read from disk even if the block was just
read a moment ago.  This means the page cache in `block.c` gives no benefit
to filesystem metadata operations.

The duplication and inconsistency is significant: the ELF loader gets caching,
filesystem metadata does not.

---

### 2.5 `vfs_createfile` and the Unnecessary Block Cache on Memory Filesystems

`vfs_inode` always calls `vfs_createfile`, which installs a `block_file_t` on
every `FL_REG` inode regardless of the filesystem.  Tarfs inodes backed by
in-memory data get a page cache allocated and then their data is *copied* into
that cache on first access — a wasteful double-copy since the data is already
in memory at a known address.

Tarfs should be able to implement a custom `ino_ops_t.fetch` that returns the
physical address of the tar data directly without any copying.  The
`vfs_fetch_page` function already has the hook:

```c
size_t vfs_fetch_page(inode_t *ino, xoff_t off, bool blocking)
{
    if (ino->ops->fetch)
        return ino->ops->fetch(ino, off, blocking);  // ← tarfs should use this
    else if (ino->type == FL_REG || ino->type == FL_BLK)
        return block_fetch(ino, off, blocking);
    return 0;
}
```

---

## 3. Known Bugs

### 3.1 Physical Page Leak on Failed Read (block.c:113) — Fixed

In `block_fill` synchronous path, `kmap` allocates a physical page and stores
its address in `page->phys`.  If `ino->ops->read` returns an error, the error
path returns -1 without resetting `page->phys`.  On the retry loop in
`block_fetch` (up to 3 attempts), `block_fill` fires `assert(page->phys == 0)`
because the field is now non-zero.

**Trigger:** Attempting to load a zero-length file through the block cache.
macOS AppleDouble (`._filename`) entries in a tar archive have zero-length
content; tarfs sets `ino->length = 0`; `tar_read` returns -1; the assert fires.

**Fix applied:** Reset `page->phys = 0` in the sync error path immediately
after `kunmap`, before returning -1.

### 3.2 `bio_wait` Return Type Mismatch

`block.c` declares `int bio_wait(bio_t *)` and uses `ret = bio_wait(bio)`.
`bio.c` defines it as `void bio_wait(bio_t *)`.  In C this is undefined
behaviour; in practice it always returns 0 via register garbage, silently
masking any I/O error that a real implementation would propagate.

### 3.3 `bio_create` Returns Undefined Value

`bio_create` has an empty body and no `return` statement, but `block_fill`
assigns its result to `bio_t *bio`.  The pointer is then passed to
`bio_request` and `bio_push`, both of which are also empty.  The entire async
path is inert but does not crash only because all three callees ignore their
arguments.

### 3.4 Double `block_rel` in `block_release`

```c
int block_release(inode_t *ino, xoff_t off, size_t pg, bool dirty) {
    ...
    block_rel(ino, page);   // drops the rcu added by block_get
    block_rel(ino, page);   // second drop — intentional?
    return 0;
}
```

`block_fetch` and `block_get` each increment `rcu` once, but `block_release`
decrements twice.  Whether this is intentional (to balance a fetch+get pair) or
a bug depends on whether callers of `block_fetch` are expected to also call
`block_get` internally.  The current code paths suggest it is a bug.

### 3.5 No Mutual Exclusion on ATA Drive Access

`ata_read_pata_pio` and `ata_write_pata_pio` directly program hardware
registers without holding any lock.  Concurrent I/O from two tasks (e.g., the
network stack reading a file while another task writes) will corrupt the LBA
registers mid-transaction.

### 3.6 `block_scavenge` Does Not Check `in_ops`

When a page is evicted from the LRU, `page_release(page->phys)` is called
without verifying `page->in_ops`.  If the async path were ever enabled, this
would free a page whose physical address is still held by an in-flight DMA
operation.

### 3.7 Page Cache Bypassed by Filesystem Metadata (`bkmap` in ext2/vfat)

As described in §2.4, ext2 and vfat use a `bkmap` macro that reads directly
from the block device, bypassing `block.c` entirely.  A superblock read during
mount and a superblock read during a stat(2) call will each hit the disk even
though the data has not changed.

---

## 4. Proposed Redesign

The goal is a clean three-layer design:

```
 ┌─────────────────────────────────────────────────────────┐
 │  L3: File Cache  (page cache indexed by inode + offset) │
 │  Unified API: vfs_fetch_page / vfs_release_page          │
 └───────────────────────────────┬─────────────────────────┘
                                 │ page-aligned block I/O
 ┌───────────────────────────────▼─────────────────────────┐
 │  L2: I/O Request Queue  (per block device)               │
 │  bio_t requests, merging, elevator/deadline scheduler    │
 │  IRQ-driven completion, sleeping callers                 │
 └───────────────────────────────┬─────────────────────────┘
                                 │ sector I/O
 ┌───────────────────────────────▼─────────────────────────┐
 │  L1: Block Device Driver  (ata, nvme, virtio-blk, …)     │
 │  DMA or PIO, IRQ handler wakes bio completion            │
 └─────────────────────────────────────────────────────────┘
```

### 4.1 L1 — Device Driver Interface

The driver exposes a single function:

```c
typedef void (*bio_submit_fn)(inode_t *dev, bio_t *bio);
```

It receives a pre-built `bio_t` with a scatter-gather list of physical pages,
programs DMA, and returns immediately.  The IRQ handler calls
`bio_complete(bio, 0)` on success or `bio_complete(bio, errno)` on error.

For PIO drivers that cannot DMA, the driver can optionally expose a synchronous
fallback:

```c
typedef int (*pio_rw_fn)(inode_t *dev, char *buf, size_t len, xoff_t off);
```

The L2 layer wraps it: `kmap` the target page, call `pio_rw_fn`, `kunmap`,
signal completion.  This keeps PIO out of the I/O path for DMA-capable drivers.

### 4.2 L2 — I/O Request Queue

Each block device gets a `bioq_t` (bio queue):

```c
typedef struct bioq {
    mtx_t        lock;
    cnd_t        wake;         // wakes bio_daemon when new work arrives
    bbtree_t     pending;      // requests sorted by LBA
    llhead_t     in_flight;    // requests submitted to driver
    bio_submit_fn submit;
    inode_t      *dev;
} bioq_t;
```

A per-device `bio_daemon` task sleeps on `wake`, dequeues the next request
using a simple deadline elevator (reads before writes, sorted by LBA), calls
`submit`, and waits for completion.

```c
// The caller (page cache) allocates and fills a bio_t:
bio_t *bio = bio_alloc(dev, BIO_READ, page_phys, lba, n_sectors);
bio_submit(bioq, bio);           // enqueues and wakes the daemon
bio_wait(bio);                   // caller sleeps until bio_complete() fires
bio_free(bio);
```

`bio_complete` signals a per-bio condition variable.  `bio_wait` is a
straightforward `cnd_wait(&bio->done, &bio->lock)`.  Multiple waiters are
supported (for speculative read-ahead).

### 4.3 L3 — Page Cache

The page cache is largely as today (`block.c`) with the following fixes:

**Explicit physical page allocation.** Replace the `kmap(NULL, 0, VMA_PHYS)`
round-trip with a direct `page_new()` call:

```c
page->phys = page_new();
if (page->phys == 0) { /* OOM */ return -ENOMEM; }
void *ptr = kmap(PAGE_SIZE, NULL, page->phys, VM_RW | VMA_PHYS);
```

On failure, `page_release(page->phys); page->phys = 0;` is explicit and safe.

**Unified block mapping.** Replace `bkmap`/`bkunmap` in filesystem drivers with
`blk_map`/`blk_close` (`blkmap_t`).  Since `blk_map` goes through
`vfs_fetch_page`, filesystem metadata (superblock, inode tables, directory
blocks) will be cached and the page cache will give hits across operations.

**Memory-mapped filesystems bypass the cache.** Add `ino_ops_t.fetch` to tarfs
and similar in-memory filesystems:

```c
static size_t tar_fetch(inode_t *ino, xoff_t off, bool blocking)
{
    tar_info_t *info = ino->drv_data;
    tar_entry_t *entry = ADDR_OFF(info->base, (ino->no - 2) * TAR_BLOCK_SIZE);
    char *data = ADDR_OFF(entry, TAR_BLOCK_SIZE) + off;
    // Return the physical address of the tar data in-place — no copy
    return mmu_read((size_t)data);
}
```

This makes tarfs zero-copy for reads: the ELF loader maps the tar data directly
without going through the block cache.

**Proactive eviction.** Connect `block_scavenge` to the memory allocator's
pressure callback so pages are reclaimed under low-memory conditions rather than
only when `block_scavenge` is called manually (which currently never happens).

**Writeback on a timer.** Add a periodic kernel task (e.g., every 5 seconds)
that walks all `block_file_t` trees and writes back dirty pages, capping
`block_release`'s writeback to dirty pages that have aged past a threshold.

### 4.4 Migration Path

The changes can be staged without breaking anything:

1. **Fix existing bugs** (§3.1–3.7) without changing the overall design — most
   are single-function fixes.
2. **Add `page_new()` + explicit free** to `block_fill` sync path.
3. **Wire `bio_wait` return type** — change `bio.c` signature to `int`, return
   -1 until a real implementation exists, so callers see errors instead of
   silent success.
4. **Port ext2/vfat `bkmap` to `blkmap_t`** — straightforward mechanical
   change; both APIs have the same shape.
5. **Implement `bio_t` + `bioq_t`** + IRQ-driven ATA completion for the ATA
   driver, then flip `block_file_t.async = true` for ATA inodes.
6. **Add `tar_fetch`** to tarfs; remove the unnecessary `block_create` for
   in-memory filesystems.

---

## 5. File Reference

| File | Role |
|---|---|
| `src/vfs/block.c` | Page cache: `block_file_t`, `block_page_t`, `block_fetch/release/read/write` |
| `src/vfs/bio.c` | Async I/O scheduler (stub) |
| `src/vfs/data.c` | `vfs_createfile`, `vfs_fetch_page`, `vfs_release_page` |
| `src/stdc/blkmap.c` | `blkmap_t` sliding-window abstraction (used by ELF loader) |
| `include/kernel/blkmap.h` | `blkmap_t` type and `blk_map/close/clone` macros |
| `drivers/pc/ata/pata.c` | PIO read/write for PATA disks |
| `drivers/pc/ata/patapi.c` | PIO read for ATAPI (CD-ROM) |
| `drivers/fs/ext2/blocks.c` | `bkmap` / `bkunmap` usage (bypasses page cache) |
| `src/mem/vma_file.c` | VM-side consumer: `vma_fetch_file` → `vfs_fetch_page` |
| `src/mem/elf32.c` | ELF loader: uses `blkmap_t` via `blk_open` |
| `src/vfs/tarfs.c` | In-memory tar filesystem (has block cache but shouldn't need it) |
