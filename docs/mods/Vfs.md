# Virtual File System (VFS)

**Source:** `src/vfs/`  
**Header:** `include/kernel/vfs.h`  
**Test harness:** `make cli-vfs` → `tests/fs_*.sh`

---

## Purpose

The VFS provides a single, uniform interface for all I/O operations
regardless of the underlying storage technology.  Userspace programs
and kernel code both see the same `inode_t`-based API whether they are
accessing an ext2 hard-disk, an ISO 9660 CD-ROM, a named pipe, a network
socket, or a synthetic device file.

---

## Design overview

The VFS has two distinct layers that work together.

### Inode layer — what a file *is*

An **`inode_t`** is the kernel's representation of a single file.  Each
file has exactly one inode, identified by a `(device_no, inode_no)` pair.
Inodes are reference-counted (field `rcu`); they are allocated with
`vfs_inode()`, acquired with `vfs_open_inode()`, and released with
`vfs_close_inode()`.  When the count reaches zero the inode is
destroyed: `ino_ops_t::close()` is called, then the inode is freed and
the per-device refcount is decremented.

Each inode holds:
- file type (`ftype_t`), mode bits, timestamps
- a pointer to an `ino_ops_t` vtable (filesystem operations)
- optionally a pointer to an `fl_ops_t` vtable (open/close/read/write
  for special files: pipes, sockets, framebuffers)
- a `device_t *` back-pointer
- driver-private data (`drv_data`, `fl_data`)

**File types** (`ftype_t`):

| Constant | Meaning |
|---|---|
| `FL_REG` | Regular file (from a filesystem) |
| `FL_DIR` | Directory |
| `FL_LNK` | Symbolic link |
| `FL_BLK` | Block device |
| `FL_CHR` | Character device |
| `FL_PIPE` | Pipe / FIFO |
| `FL_NET` | Network interface |
| `FL_SOCK` | Network socket |
| `FL_FRM` | Framebuffer / video stream |

### Tree layer — where a file *lives*

An **`fnode_t`** is a path-tree node (comparable to a Linux dentry).
It carries the name, a parent pointer, and a reference to an inode.
Multiple fnodes can point to the same inode (hard links).

Fnodes are managed through a per-`fs_anchor_t` hash map and an LRU list.
`vfs_search()` walks the fnode tree from the root or cwd, resolving each
component by calling the directory inode's `ino_ops_t::lookup()` on cache
misses.  Symlinks are followed via `ino_ops_t::readlink()`.

### Per-process namespace — `fs_anchor_t`

Each task holds an `fs_anchor_t *fsa` that records:
- `root` — the process's root fnode (supports `chroot`)
- `pwd` — the current working directory fnode
- `umask` — file creation mask

`vfs_open_vfs()` increments the refcount (shared namespace),
`vfs_clone_vfs()` creates an independent copy (used by `fork`).

### Global state — `vfs_share_t`

The global `__vfs_share` singleton keeps the device list, the mount
list, the filesystem type registry (`fs_hmap`), the global fnode LRU,
and the root `fs_anchor_t` used before any task exists.

---

## Key operations

### Path resolution

```c
fnode_t *vfs_search(fs_anchor_t *fsa, const char *path,
                    user_t *user, bool resolve, bool follow);
```

Breaks the path into components, walks the fnode tree, calls
`ino_ops_t::lookup()` on each directory for cache misses, and follows
symlinks when `follow` is true.  Returns a retained `fnode_t *` or NULL
with `errno` set.

### File open / create

```c
inode_t *vfs_open(fs_anchor_t *fsa, const char *name,
                  user_t *user, int mode, int flags);
```

Resolves the path, then creates (`ino_ops_t::create()`) or opens the
inode according to `flags` (`IO_OPEN`, `IO_CREAT`).  Returns a retained
inode.

### Mounting

```c
int vfs_mount(fs_anchor_t *fsa, const char *dev, const char *fstype,
              const char *path, user_t *user, const char *options);
```

Finds the block device named `dev`, looks up the `fsmount_t` callback
for `fstype`, calls it to get a filesystem root inode, and records a
mount point on the target fnode.

### Block I/O and page cache

Block reads/writes go through `src/vfs/block.c`.  `block_fetch()` maps
the relevant page into the kernel address space (via `kmap()`), calls
`ino_ops_t::fetch()` to populate it from storage if missing, and
returns the physical page.  `block_release()` marks the page dirty when
written and the block layer eventually calls `ino_ops_t::release()` to
write it back.

---

## Driver interface

### Filesystem drivers

Register with:
```c
void vfs_addfs(const char *name, fsmount_t mount, fsformat_t format);
```

A `fsmount_t` receives a block device inode and options, and returns the
root inode of the mounted volume.

### Device drivers

Register with:
```c
int vfs_mkdev(inode_t *ino, const char *name);
```

Creates a named entry in the device namespace (visible under `/dev` via
`devfs`).

---

## Filesystem drivers (in `drivers/fs/`)

| Driver | Type | Status |
|---|---|---|
| `ext2` | Read/write | Mostly complete; known issue with files > 12 blocks |
| `vfat` | Read/write | Short names only; directory extension incomplete |
| `isofs` | Read-only | Working |
| `tarfs` | Read-only (in-memory) | Working — used for initial ramdisk |
| `devfs` | Synthetic | Working — populates `/dev` |

---

## Internal pipe design

Pipes (`src/vfs/pipe.c`) use a single `PAGE_SIZE` ring buffer mapped
into kernel memory.  A read and a write cursor run around the ring.
When the buffer is full the writer blocks (via a semaphore); when it is
empty the reader blocks.  Auto-extension to 64 KB is noted as a TODO.

---

## Known issues

- Fnode LRU scavenging policy is not defined (cache grows without bound).
- Device teardown (`vfs_rmdev`) is incomplete.
- `sys_opendir` syscall is not wired in the dispatch table.
- ext2 files longer than 12 direct blocks (>48 KB on a 4 KB block
  filesystem) have known reliability issues.
- VFAT long filenames (LFN) are not supported.
