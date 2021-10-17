/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2021  <Fabien Bavent>
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
#ifndef __UNISTD_H
#define __UNISTD_H 1

#include <stddef.h>
#include <sys/types.h>


// #define SEEK_SET 0 /* Seek from beginning of file.  */
// #define SEEK_CUR 1  Seek from current position.
// #define SEEK_END 2 /* Seek from end of file.  */

int open(const char *path, int flags, ...);

int read(int fd, void *buf, int len);
int write(int fd, const void *buf, int len);

int lseek(int fd, off_t off, int whence);
void close(int fd);



static inline void *memrchr(const void *s, int c, size_t n)
{
    const char *p = ((char *)s) + n;
    while (p > (const char *)s) {
        p--;
        if (*p == c)
            return (void *)p;
    }
    return (void *)s;
}

#endif  /* __UNISTD_H */
