# Repository Cleanup Proposals

This document lists concrete cleanup actions to reduce dead code, noise,
and confusion in the repository.  Items are grouped by safety level.

---

## Safe to delete immediately

These files contain no useful live code.

### `src/tasks/dlib.c` — empty file (0 bytes)

The file exists but is completely empty.  The actual dlib implementation
for tasks is in `src/mem/dlib.c`.

```bash
git rm src/tasks/dlib.c
```

---

### `src/mem/mspace.c` — 306 lines, 100% commented out

The old `mspace_t` API implementation was renamed to `vmsp_t` and the
code moved to `src/mem/vmsp.c`.  `mspace.c` is a ghost file: every
single line is commented out.

```bash
git rm src/mem/mspace.c
```

---

### `src/mem/vma.c` — 814 lines, 98% commented out

Only 15 live lines remain (the `vma_ops_t` extern declarations and the
`vma_create()` dispatcher).  The rest is the old VMA implementation
before it was split into `vma_anon.c`, `vma_file.c`, `vma_misc.c`,
`vma_dlib.c`.  The live content should be moved into `src/mem/vmsp.c`
or a small `vma_core.c`, then this file deleted.

```bash
# Move live content first, then:
git rm src/mem/vma.c
```

---

### `drivers/fs/vfat/utils/format.c` — test stub with empty `main()`

This file contains only a `main()` that returns 0.  It appears to be a
leftover from a standalone format-tool experiment.  It is not listed in
any Makefile and is never compiled.

```bash
git rm drivers/fs/vfat/utils/format.c
```
(If the `drivers/fs/vfat/utils/` directory becomes empty, remove it too.)

---

### `src/core/modules.c` — entirely commented-out stubs

The file contains nothing but commented-out stub implementations of
platform functions (kept "as reference during refactors").  The same
information is more usefully captured in `CLAUDE.md` and the
architecture header.  It adds noise to `grep` and `ctags` results.

```bash
git rm src/core/modules.c
```

---

## Rename / fix

### `docs/MemoryManagment.md` — typo in filename

The filename has a typo (`Managment` → `Management`).  It is not
referenced from any internal link, so a rename is safe.

```bash
git mv docs/MemoryManagment.md docs/MemoryManagement.md
```

---

## Low-risk cleanup (recommended)

### `.DS_Store` in repository root

`.DS_Store` (macOS Finder metadata) is committed in the root.  It should
be removed and `.gitignore` updated.

```bash
git rm --cached .DS_Store
echo ".DS_Store" >> .gitignore
```

---

### Consolidate `arch/x86_64/` stubs

`arch/x86_64/` contains only two header files and a `make.mk`.  The
headers (`bits/atomic.h`, `kernel/arch.h`) are needed if anyone ever
starts an x86_64 port.  The `make.mk` is a placeholder.  No action
needed unless the port is abandoned entirely — in that case, move the
headers into a `ports/` or `future/` folder and note the status clearly.

---

### `arch/arm64/make.mk` — single-file directory

`arch/arm64/` contains only `make.mk`.  Either start the port or move
the file to `ports/arm64/` and update `ROADMAP.md`.

---

## Deferred (requires more thought)

### `config.yml` — driver selection config not consumed by build

`config.yml` lists drivers with enable/disable flags but the Makefile
ignores it.  Either:
- Wire it up: write a `make/config.mk` that reads `config.yml` and sets
  `DRV` accordingly.
- Or delete it and document driver selection directly in the Makefile.

---

### `tests/snd/main.c` and `src/snd/` absence

The Makefile defines `SRC_clisnd` for a sound CLI test but
`$(topdir)/src/snd/*.c` is empty/missing.  The `cli-snd` target will
link but do nothing.  Either add the `src/snd/` stub or remove the
`cli-snd` target from the Makefile.

---

### `include/cbuild/` — host-build headers

`include/cbuild/` contains `ctype.h`, `iostm.h`, `string.h` — thin
wrappers intended for hosted (non-kernel) builds.  Their relationship to
`include/cc/` and the main `include/` headers is unclear.  Audit and
consolidate.

---

### `scripts/cfg/stage2_eltorito` — binary blob in source tree

`scripts/cfg/stage2_eltorito` is a GRUB legacy bootloader binary.  It
should either be removed (if GRUB2 is used exclusively) or documented
clearly and potentially moved to a `tools/` directory.

---

## Summary table

| Action | File(s) | Risk |
|---|---|---|
| `git rm` | `src/tasks/dlib.c` | None — empty |
| `git rm` | `src/mem/mspace.c` | None — all commented |
| Move live code + `git rm` | `src/mem/vma.c` | Low — 15 live lines |
| `git rm` | `drivers/fs/vfat/utils/format.c` | None — dead stub |
| `git rm` | `src/core/modules.c` | None — all commented |
| `git mv` | `docs/MemoryManagment.md` | None — typo fix |
| `git rm --cached` + gitignore | `.DS_Store` | None |
| Investigate | `config.yml` | Low |
| Investigate | `tests/snd/`, `src/snd/` | Low |
| Investigate | `include/cbuild/` | Low |
| Investigate | `scripts/cfg/stage2_eltorito` | Low |
