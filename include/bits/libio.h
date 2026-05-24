/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015  <Fabien Bavent>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   - - - - - - - - - - - - - - -
 */
#ifndef __STDC_LIBIO_H
#define __STDC_LIBIO_H 1

// #include <wchar.h>
#include <stddef.h>
#include <stdarg.h>
#include <bits/io.h>
#include <bits/atomic.h>

#define EOF (-1)
#define BUFSIZ 512

#define _IOFBF 0
#define _IOLBF 1
#define _IONBF 2

#define F_PERM 1
#define F_NORD 4
#define F_NOWR 8
#define F_EOF 16
#define F_ERR 32
#define F_SVB 64
#define F_APP 128


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
    int mode;
    int oflags_;
    int flags_;
    int lbuf_;
    atomic_int lock_;  /* -1: no lock */
    size_t count_;
    size_t fpos_; // TODO __off_t

    struct _IO_BUF rbf_;
    struct _IO_BUF wbf_;

    int (*read)(FILE *, char *, size_t);
    int (*write)(FILE *, const char *, size_t);
    int (*seek)(FILE *, long, int);
    int (*close)(FILE *);
};



extern struct _IO_FILE *stdin;
extern struct _IO_FILE *stdout;
extern struct _IO_FILE *stderr;


#define R_OK 4
#define W_OK 2
#define X_OK 1


#define FLOCK(f)  __lock(&(f)->lock_)
#define FUNLOCK(f) __unlock(&(f)->lock_)
#define FRMLOCK(f) __lock(&(f)->lock_)


void __fstr_open_rw(FILE *fp, void *buf, int len);
void __fstr_open_ro(FILE *fp, const void *buf, int len);

int __fgets_ifeqi(FILE *fp, const char *str);

int fgetc_unlocked(FILE *fp);
int fputc_unlocked(int c, FILE *fp);
int fflush_unlocked(FILE *fp);
int ungetc_unlocked(int c, FILE *fp);


size_t fread_unlocked(void *restrict ptr, size_t size, size_t n, FILE *restrict fp);
size_t fwrite_unlocked(const void *restrict ptr, size_t size,  size_t n, FILE *restrict fp);

void clearerr(FILE *fp);
int feof(FILE *fp);
int ferror(FILE *fp);
int fileno(FILE *fp);


#endif  /* __STDC_LIBIO_H */
