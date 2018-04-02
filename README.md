

## Dependancies

The kernel doesn't have any dependency. However a standlone kernel won't get
you far.


The source of kernel of KoraOS contains all the headers required with two
exceptions: `stddef.h` and `stdarg.h`. Those are usually provided by the
compilor.

The kernel must be compiled with a _freestanding_ compilor, which should
provide those headers. In case your compilor do not, the kernel comes with
two really basic implementation placed on the `hooks/` directory.

