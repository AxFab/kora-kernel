#ifndef __STDC_LIBIO_H
#define __STDC_LIBIO_H 1

// #include <wchar.h>
#include <stddef.h>
#include <stdarg.h>
#include <bits/io.h>

#define EOF (-1)
#define BUFSIZ 512

#define FF_EOF  (1 << 1)
#define FF_ERR  (1 << 2)


typedef long fpos_t;
typedef struct _IO_FILE FILE;
typedef struct _IO_FILE __FILE;
typedef struct _IO_FILE _IO_FILE;


struct _IO_BUF {
    char *base_;
    char *pos_;
    char *end_;
};

struct _IO_FILE {
    int fd_;
    int oflags_;
    int flags_;
    int lbuf_;
    int lock_;  /* -1: no lock */
    size_t count_;
    size_t fpos_;

    struct _IO_BUF rbf_;
    struct _IO_BUF wbf_;

    int (*read)(FILE *, char *, size_t);
    int (*write)(FILE *, const char *, size_t);
    int (*seek)(FILE *, long, int);
    int (*close)(FILE *);
};



// struct _IO_FILE _IO_stdin_;
// struct _IO_FILE _IO_stdout_;
// struct _IO_FILE _IO_stderr_;

extern struct _IO_FILE *stdin;
extern struct _IO_FILE *stdout;
extern struct _IO_FILE *stderr;


#define R_OK 4
#define W_OK 2
#define X_OK 1
#define F_PERM 7


#define FLOCK(f) ((void)0)
#define FUNLOCK(f) ((void)0)
#define FRMLOCK(f) ((void)0)

/*
typedef __ssize_t __io_read_fn (void *__cookie, char *__buf, size_t __nbytes);

typedef __ssize_t __io_write_fn (void *__cookie, const char *__buf,
     size_t __n);

typedef int __io_seek_fn (void *__cookie, __off64_t *__pos, int __w);
typedef int __io_close_fn (void *__cookie);

extern int __underflow (_IO_FILE *);
extern int __uflow (_IO_FILE *);
extern int __overflow (_IO_FILE *, int);

extern int _IO_getc (_IO_FILE *__fp);
extern int _IO_putc (int __c, _IO_FILE *__fp);
extern int _IO_feof (_IO_FILE *__fp) __attribute__ ((__nothrow__ , __leaf__));
extern int _IO_ferror (_IO_FILE *__fp) __attribute__ ((__nothrow__ , __leaf__));

extern int _IO_peekc_locked (_IO_FILE *__fp);





extern void _IO_flockfile (_IO_FILE *) __attribute__ ((__nothrow__ , __leaf__));
extern void _IO_funlockfile (_IO_FILE *) __attribute__ ((__nothrow__ , __leaf__));
extern int _IO_ftrylockfile (_IO_FILE *) __attribute__ ((__nothrow__ , __leaf__));


extern int _IO_vfscanf (_IO_FILE * __restrict, const char * __restrict,
   __gnuc_va_list, int *__restrict);
extern int _IO_vfprintf (_IO_FILE *__restrict, const char *__restrict,
    __gnuc_va_list);
extern __ssize_t _IO_padn (_IO_FILE *, int, __ssize_t);
extern size_t _IO_sgetn (_IO_FILE *, void *, size_t);

extern __off64_t _IO_seekoff (_IO_FILE *, __off64_t, int, int);
extern __off64_t _IO_seekpos (_IO_FILE *, __off64_t, int);

extern void _IO_free_backup_area (_IO_FILE *) __attribute__ ((__nothrow__ , __leaf__));
*/

#endif  /* __STDC_LIBIO_H */
