/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2019  <Fabien Bavent>
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
#ifndef _KORA_FD_H
#define _KORA_FD_H 1

#ifndef __SYS_CALL
# error "This source file can't be used without syscalls."
#endif

#include <sys/types.h>
#include <kora/iofile.h>
#include <stdarg.h>

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

/* Format and print a string into a FILE interface. */
int printf(const char *str, ...);
int fprintf(FILE *fp, const char *str, ...);
int vfprintf(FILE *fp, const char *str, va_list ap);

/* All of those methods are bind over vfscanf
  which is implemented in another file. */
int scanf(const char *format, ...);
int fscanf(FILE *f, const char *format, ...);
int vfscanf(FILE *f, const char *format, va_list ap);



int open(const char *, int, ...);
int close(int);
int read(int fd, void *buf, size_t count);
int write(int fd, const void *buf, size_t count);
int lseek(int fd, off_t offset, unsigned int origin);

int fcntl(int fd, int cmd, ...);
FILE *fvopen(int fd, int oflags);

#define O_RDONLY  0x1
#define O_WRONLY  0x2
#define O_RDWR  0x3
#define O_CREAT  0x4
#define O_TRUNC  0x8
#define O_APPEND  0x10
#define O_EXCL  0x20
#define O_CLOEXEC  0x40

#define FD_CLOEXEC  0x1
#define F_SETFD  0x2
#define F_SETFL  0x3

#define SEEK_SET  0
#define SEEK_CUR  1
#define SEEK_END  2

#define _IOLBF  0x1000
#define _IONBF  0x2000
#define _IOFBF  0x3000

#endif  /* _KORA_FD_H */
