# Wanna contribute to Kora?

> This is a hobby kernel developed by a single author.  Contributions,
> bug reports, and experiments are welcome.  Join the
> [Discord](https://discord.gg/hdvV4zcgxk) channel for discussion.

## Raising an issue

Open a GitHub/GitLab issue describing: the observed behaviour, the
expected behaviour, and steps to reproduce.  Attach any relevant output
from the CLI test programs or QEMU serial log.

## Helping with current development

Read `ROADMAP.md` for prioritised issues.  Read `CLAUDE.md` for a full
description of the codebase, build instructions, and testing approach.

## Requesting a new feature

Open an issue tagged `feature-request` with a description of the
use-case and any design ideas.

## Development guidelines

### Coding conventions

- C99/C11 with GCC extensions.  No C++ or external libraries.
- All kernel types end in `_t`.
- Use `kalloc()` / `kfree()` for heap; `kmap()` / `kunmap()` for page-mapped memory.
- Use `kprintf(KL_*, fmt, ...)` for logging, not `printf`.
- Protect shared data with `splock_t` (spinlock + IRQ mask) or `mtx_t`
  for sleeping contexts.
- Return 0 on success, −1 on error, and set `errno`.

### Proposing a contribution

1. Fork the repository.
2. Create a branch named `feature/...` or `fix/...`.
3. Run `make check` before submitting — all tests must pass.
4. Open a merge/pull request with a description of the change and
   which ROADMAP item it addresses (if any).

