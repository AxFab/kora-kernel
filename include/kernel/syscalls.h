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
#ifndef _KERNEL_SYSCALLS_H
#define _KERNEL_SYSCALLS_H 1


#include <kernel/stdc.h>

struct dirent {
    int d_ino;
    int d_off;
    unsigned short int d_reclen;
    unsigned char d_type;
    char d_name[256];
};

struct filemeta {
    int ino;
    int dev;
    int block;
    int ftype;

    int64_t size;
    int64_t rsize;

    uint64_t ctime;
    uint64_t mtime;
    uint64_t atime;
    uint64_t btime;
};

enum sys_vars {
    SNFO_NONE = 0,
    SNFO_ARCH,
    SNFO_SNAME,
    SNFO_OSNAME,
    SNFO_PWD,
// SNFO_ARCH
// SNFO_GITH
// SNFO_SNAME
// SNFO_VERSION
// SNFO_RELEASE
// SNFO_OSNAME
// SNFO_HOSTNAME
// SNFO_DOMAIN
// SNFO_USER
// SNFO_USERNAME
// SNFO_USERMAIL
// SNFO_PWD
};

enum syscall_no {
    SYS_EXIT = 0,
    SYS_SLEEP,

    SYS_FUTEX_WAIT,
    SYS_FUTEX_REQUEUE,
    SYS_FUTEX_WAKE,

    SYS_SPAWN,
    SYS_THREAD,

    SYS_MMAP,
    SYS_MUNMAP,
    SYS_MPROTECT,

    SYS_GINFO, // 10
    SYS_SINFO,

    SYS_OPEN,
    SYS_CREATE,
    SYS_CLOSE,
    SYS_OPENDIR,
    SYS_READDIR,
    SYS_SEEK,
    SYS_READ, // 16
    SYS_WRITE,
    SYS_ACCESS,
    SYS_FCNTL, // 19

    SYS_PIPE,
    SYS_WINDOW,
    SYS_FSTAT,

    SYS_XTIME,
};

// #define SPW_SHUTDOWN 0xcafe
// #define SPW_REBOOT 0xbeca
// #define SPW_SLEEP 0xbabe
// #define SPW_HIBERNATE 0xbeaf
// #define SPW_SESSION 0xdead


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
// Tasks, Process

void sys_exit(int code);
// long sys_wait(int pid, int *status, int option);
long sys_sleep(xtime_t *timeout, xtime_t *remain);
long sys_futex_wait(int *addr, int val, xtime_t *timeout, int flags);
long sys_futex_requeue(int *addr, int val, int val2, int *addr2, int flags);
long sys_futex_wake(int *addr, int val);

long sys_spawn(const char *program, const char **args, const char **envs, int *streams, int flags);
long sys_thread(const char *name, void *entry, void *params, size_t len, int flags);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
// Memory
void *sys_mmap(void *addr, size_t length, unsigned flags, int fd, size_t off);
long sys_munmap(void *addr, size_t length);
long sys_mprotect(void *addr, size_t length, unsigned flags);
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

long sys_ginfo(unsigned info, char *buf, size_t len);
long sys_sinfo(unsigned info, const char *buf, size_t len);
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
// File system
long sys_open(int dirfd, const char *path, int flags, int mode);
long sys_close(int fd);
long sys_readdir(int fd, char *buf, size_t len);
long sys_seek(int fd, xoff_t offset, int whence);
long sys_read(int fd, char *buf, int len);
long sys_write(int fd, const char *buf, int len);
long sys_access(int dirfd, const char *path, int flags);
long sys_fcntl(int fd, int cmd, void **args);
long sys_fstat(int dirfd, const char *path, struct filemeta *meta, int flags);

// #define SYS_WINDOW  18
// #define SYS_PIPE  19
long sys_pipe(int *fds, int flags);


int sys_xtime(int name, xtime_t *ptime);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

// Signals
// #define SYS_SIGRAISE  15
// #define SYS_SIGACTION  16
// #define SYS_SIGRETURN  17










































// /* --------
//   Tasks, Process & Sessions
// --------- */


// /* Prepare system shutdown, sleep or partial shutdown (kill session) */
// long sys_power(unsigned type, long delay);

// long sys_sfork(unsigned uid, const char *path, const char **args, const char **envs, int *fds);
// long sys_pfork(int keep, const char *path, const char **args, const char **envs, int *fds);
// long sys_tfork(int keep, void *func, void *args, int sz, const char **envs);


// /* Kill all the thread of the current process */
// long sys_exit(int status, int tid);
// /* Fork the current task and copy some attribute */
// long sys_fork(int clone);
// long sys_sleep(long timeout);
// long sys_wait(int what, unsigned id, long timeout);

// // exec ve

// int sys_futex_wait(int *addr, int val, long timeout, int flags);
// int sys_futex_requeue(int *addr, int val, int val2, int *addr2, int flags);

// /* --------
//   Input & Output
// --------- */

// long sys_read(int fd, char *buf, int len);
// long sys_write(int fd, const char *buf, int len);
// long sys_access(int fd, const char *path, int flags);
// long sys_open(int fd, const char *path, int flags);
// long sys_seek(int fd, off_t offset, int whence);
// // long sys_open(int fd, const char * path, int flags, ftype_t type, int mode);
// long sys_close(int fd);
// long sys_readdir(int fd, char *buf, int len);
// // lseek
// // sync
// // umask

// // access
// // ioctl
// // fcntl

// /* --------
//   File system
// --------- */

// int sys_fstat(int fd, const char *path, filemeta_t *meta, int flags);
// // stat (at)
// // chmod
// // chown
// // utimes

// // link
// // unlink!
// // rename
// // mkdir
// // rmdir

// // dup
// int sys_pipe(int *fds, int flags);
// int sys_window(int ctx, int width, int height, unsigned flags);
// int sys_fcntl(int fd, int cmd, void *args);

// /* --------
//   Network
// --------- */

// #define NP_IP4_TCP 1
// #define NP_IP4_UDP 2

// int sys_socket(int protocol, const char *address, int port);

// /* --------
//   Memory
// --------- */

// void *sys_mmap(void *addr, size_t length, unsigned flags, int fd, off_t off);
// long sys_munmap(void *address, size_t length);
// long sys_mprotect(void *address, size_t length, unsigned flags);

// /* --------
//   Signals
// --------- */

// // kill
// // sigaction
// // sigmask


// /* --------
//   System
// --------- */
// #define _MP_ "SP" // single-proc
// // #define _MP_ "SMP" // serial-multi-proc
// // #define _MP_ "NUMA" // non-uniform-memory-access

// // time, pid, uid, euid, pwd, chroot,

// long sys_ginfo(unsigned info, void *buf, int len);
// long sys_sinfo(unsigned info, const void *buf, int len);
// long sys_log(const char *msg);
// long sys_sysctl(int cmd, void *args);
// long sys_copy(int out, int in, size_t size, int count);


// long txt_pfork(const char *);
// long txt_tfork(const char *);
// long txt_sleep(const char *);
// long txt_access(const char *);
// long txt_open(const char *);
// long txt_seek(const char *);
// long txt_close(const char *);
// long txt_read(const char *);
// long txt_write(const char *);
// long txt_readdir(const char *);
// long txt_mmap(const char *);
// long txt_munmap(const char *);
// long txt_pipe(const char *);
// long txt_exit(const char *);
// long txt_fcntl(const char *);
// long txt_window(const char *);
// // long txt_open(const char *);
// long txt_futex_wait(const char *);
// long txt_futex_requeue(const char *);
// long txt_ginfo(const char *);
// long txt_sinfo(const char *);


#endif /* _KERNEL_SYSCALLS_H */
