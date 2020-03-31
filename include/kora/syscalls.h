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
#ifndef _KORA_SYSCALLS_H
#define _KORA_SYSCALLS_H 1

#include <stddef.h>

#define SYS_POWER  12
#define SYS_SCALL  13
#define SYS_SYSLOG  14
#define SYS_GINFO  25
#define SYS_SINFO  26

#define SYS_YIELD  0
#define SYS_EXIT  1
#define SYS_WAIT  2
#define SYS_EXEC  11
#define SYS_CLONE  21

#define SYS_SIGRAISE  15
#define SYS_SIGACTION  16
#define SYS_SIGRETURN  17

#define SYS_MMAP  3
#define SYS_MUNMAP  4
#define SYS_MPROTECT  5

#define SYS_ACCESS 33
#define SYS_OPEN  6
#define SYS_CLOSE  7
#define SYS_READ  8
#define SYS_WRITE  9
#define SYS_SEEK  10
#define SYS_READDIR  33

#define SYS_WINDOW  18
#define SYS_PIPE  19
#define SYS_FCNTL  20

#define SYS_FUTEX_WAIT 22
#define SYS_FUTEX_REQUEUE 23


#define SYS_START 31
#define SYS_STOP 32

#define SYS_SLEEP 30
#define SYS_SFORK 27
#define SYS_PFORK 28
#define SYS_TFORK 29

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

//struct iovec {
//    char *buffer;
//    size_t length;
//};

struct image {
    int width, height, pitch, format;
};

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


// /* System */
// int sys_power(unsigned long action, unsigned long delay_us);
// int sys_scall(const char *module);
// int sys_syslog(const char *msg);
// int sys_sysinfo(unsigned long info, char *buf, size_t len);
// /* Task */
// int sys_yield(unsigned long flags);
// long sys_exit(int status);
// void sys_stop(long status, unsigned long type);
// int sys_wait(unsigned long cause, long param, unsigned long timeout_us);
// int sys_exec(const char *exec, char **args, char **env, unsigned long flags);
// int sys_clone(unsigned long flags);
// /* Signals */
// int sys_sigraise(unsigned long signum, long pid);
// int sys_sigaction(unsigned long signum, void *sigaction);
// void sys_sigreturn();
// /* Memory */
// void *sys_mmap(void *addr, size_t length, unsigned flags, int fd, off_t off);
// int sys_munmap(size_t addr, size_t len);
// int sys_mprotect(size_t addr, size_t len, unsigned int flags);
// /* Stream */
// int sys_open(int fd, const char *path, unsigned long flags, unsigned long mode);
// int sys_close(int fd);
// int sys_read(int fd, const struct iovec *iovec, unsigned long count);
// int sys_write(int fd, const struct iovec *iovec, unsigned long count);
// int sys_seek(int fd, off_t off, unsigned long whence);
// /* - */
// int sys_window(int ctx, int width, int height, unsigned flags);
// int sys_pipe(int *fds);


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


#endif  /* _KORA_SYSCALLS_H */
