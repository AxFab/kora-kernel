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
#ifndef _KORA_IOFILE_H
#define _KORA_IOFILE_H 1

#include <kora/stddef.h>

#define EOF -1

#define BUFSIZ 512

#define FF_EOF  (1 << 1)
#define FF_ERR  (1 << 2)

typedef unsigned long fpos_t;
typedef struct _IO_FILE FILE;

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
};


#define FLOCK(f) ((void)0)
#define FUNLOCK(f) ((void)0)
#define FRMLOCK(f) ((void)0)

#define R_OK 4
#define W_OK 2
#define X_OK 1
#define F_PERM 7

/* Parse the character base mode for opening file and return binary mode */
int oflags(const char *mode);
/* Allocate a new FILE structure form an open file descriptor. */
FILE *fvopen(int fd, int oflags);

#endif  /* _KORA_IOFILE_H */
