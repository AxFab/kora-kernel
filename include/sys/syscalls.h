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
#ifndef __SYS_SYSCALLS
#define __SYS_SYSCALLS 1

#include <stddef.h>


#define SYS_EXIT        0
#define SYS_MMAP        1
#define SYS_MUNMAP      2
#define SYS_MPROTECT    3

#define SYS_OPEN    3
#define SYS_CLOSE    3
#define SYS_READ    3
#define SYS_WRITE    3
#define SYS_LSEEK    3



/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int __syscall(int no, ...);

#define sz(i)   ((size_t)(i))
#define syscall(no,...)                 syscall_x(no, __VA_ARGS__, 5, 4, 3, 2, 1, 0)
#define syscall_x(a,b,c,d,e,f,g,...)    syscall_ ## g (a,b,c,d,e,f,g)
#define syscall_0(a,b,c,d,e,f,g)        __syscall(a)
#define syscall_1(a,b,c,d,e,f,g)        __syscall(a,sz(b))
#define syscall_2(a,b,c,d,e,f,g)        __syscall(a,sz(b),sz(c))
#define syscall_3(a,b,c,d,e,f,g)        __syscall(a,sz(b),sz(c),sz(d))
#define syscall_4(a,b,c,d,e,f,g)        __syscall(a,sz(b),sz(c),sz(d),sz(e))
#define syscall_5(a,b,c,d,e,f,g)        __syscall(a,sz(b),sz(c),sz(d),sz(e),sz(f))

#endif /* __SYS_SYSCALLS */
