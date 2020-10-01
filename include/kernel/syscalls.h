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
#include <kernel/core.h>


typedef struct filemeta filemeta_t;
/* --------
  Tasks, Process & Sessions
--------- */

#define SPW_SHUTDOWN 0xcafe
#define SPW_REBOOT 0xbeca
#define SPW_SLEEP 0xbabe
#define SPW_HIBERNATE 0xbeaf
#define SPW_SESSION 0xdead

/* Prepare system shutdown, sleep or partial shutdown (kill session) */
long sys_power(unsigned type, long delay);

long sys_sfork(unsigned uid, const char *path, const char **args, const char **envs, int *fds);
long sys_pfork(int keep, const char *path, const char **args, const char **envs, int *fds);
long sys_tfork(int keep, void *func, void *args, int sz, const char **envs);


/* Kill all the thread of the current process */
long sys_exit(int status, int tid);
/* Fork the current task and copy some attribute */
long sys_fork(int clone);
long sys_sleep(long timeout);
long sys_wait(int what, unsigned id, long timeout);

// exec ve

int sys_futex_wait(int *addr, int val, long timeout, int flags);
int sys_futex_requeue(int *addr, int val, int val2, int *addr2, int flags);

/* --------
  Input & Output
--------- */

long sys_read(int fd, char *buf, int len);
long sys_write(int fd, const char *buf, int len);
long sys_access(int fd, CSTR path, int flags);
long sys_open(int fd, CSTR path, int flags);
long sys_seek(int fd, off_t offset, int whence);
// long sys_open(int fd, CSTR path, int flags, ftype_t type, int mode);
long sys_close(int fd);
long sys_readdir(int fd, char *buf, int len);
// lseek
// sync
// umask

// access
// ioctl
// fcntl

/* --------
  File system
--------- */

int sys_fstat(int fd, const char *path, filemeta_t *meta, int flags);
// stat (at)
// chmod
// chown
// utimes

// link
// unlink!
// rename
// mkdir
// rmdir

// dup
int sys_pipe(int *fds, int flags);
int sys_window(int ctx, int width, int height, unsigned flags);
int sys_fcntl(int fd, int cmd, void *args);

/* --------
  Network
--------- */

#define NP_IP4_TCP 1
#define NP_IP4_UDP 2

int sys_socket(int protocol, const char *address, int port);

/* --------
  Memory
--------- */

void *sys_mmap(void *addr, size_t length, unsigned flags, int fd, off_t off);
long sys_munmap(void *address, size_t length);
long sys_mprotect(void *address, size_t length, unsigned flags);

/* --------
  Signals
--------- */

// kill
// sigaction
// sigmask


/* --------
  System
--------- */
#define _MP_ "SP" // single-proc
// #define _MP_ "SMP" // serial-multi-proc
// #define _MP_ "NUMA" // non-uniform-memory-access

#define SNFO_ARCH 1
#define SNFO_GITH 2
#define SNFO_SNAME 3
#define SNFO_VERSION 4
#define SNFO_RELEASE 5
#define SNFO_OSNAME 6
#define SNFO_HOSTNAME 7
#define SNFO_DOMAIN 8
#define SNFO_USER 9
#define SNFO_USERNAME 10
#define SNFO_USERMAIL 11
#define SNFO_PWD 12
// time, pid, uid, euid, pwd, chroot,

long sys_ginfo(unsigned info, void *buf, int len);
long sys_sinfo(unsigned info, const void *buf, int len);
long sys_log(CSTR msg);
long sys_sysctl(int cmd, void *args);
long sys_copy(int out, int in, size_t size, int count);


long txt_pfork(const char *);
long txt_tfork(const char *);
long txt_sleep(const char *);
long txt_access(const char *);
long txt_open(const char *);
long txt_seek(const char *);
long txt_close(const char *);
long txt_read(const char *);
long txt_write(const char *);
long txt_readdir(const char *);
long txt_mmap(const char *);
long txt_munmap(const char *);
long txt_pipe(const char *);
long txt_exit(const char *);
long txt_fcntl(const char *);
long txt_window(const char *);
// long txt_open(const char *);
long txt_futex_wait(const char *);
long txt_futex_requeue(const char *);
long txt_ginfo(const char *);
long txt_sinfo(const char *);


