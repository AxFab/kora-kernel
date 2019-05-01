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
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <kora/mcrs.h>
#include <bits/libio.h>


/* All of those methods are bind over vfscanf
  which is implemented in another file. */
int vfscanf(FILE *f, const char *format, va_list ap);

/* Read from a string streaming */
static int _sread(FILE *fp, char *buf, size_t length)
{
    size_t lg = MIN(length, (size_t)(fp->rbf_.end_ - fp->rbf_.pos_));
    memcpy(buf, fp->rbf_.pos_, lg);
    fp->rbf_.pos_ += lg;
    return (length > lg) ? EOF : (int)lg;
}



/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#ifndef __NO_SYSCALL
/* Read and parse standard input */
int scanf(const char *format, ...)
{
    int ret;
    va_list ap;
    va_start(ap, format);
    ret = vfscanf(stdin, format, ap);
    va_end(ap);
    return ret;
}


/* Read and parse standard input */
int vscanf(const char *format, va_list ap)
{
    return vfscanf(stdin, format, ap);
}


/* Read and parse an open file */
int fscanf(FILE *f, const char *format, ...)
{
    int ret;
    va_list ap;
    va_start(ap, format);
    ret = vfscanf(f, format, ap);
    va_end(ap);
    return ret;
}

#endif

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */

/* Read and parse a string */
int sscanf(const char *str, const char *format, ...)
{
    int ret;
    va_list ap;
    FILE fp;

    fp.rbf_.pos_ = (char *)str;
    fp.rbf_.end_ = (char *)~0;
    fp.read = _sread;
    fp.lock_ = -1;

    va_start(ap, format);
    ret = vfscanf(&fp, format, ap);
    va_end(ap);
    return ret;
}


/* Read and parse a string */
int vsscanf(const char *str, const char *format, va_list ap)
{
    FILE fp;
    fp.rbf_.pos_ = (char *)str;
    fp.rbf_.end_ = (char *)~0;
    fp.read = _sread;
    fp.lock_ = -1;
    return vfscanf(&fp, format, ap);
}
