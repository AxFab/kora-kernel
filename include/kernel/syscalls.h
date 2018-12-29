
#include <kernel/core.h>


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
/* Kill a thread */
long sys_stop(unsigned tid, int status);
/* Kill all the thread of the current process */
long sys_exit(int status);
/* Start a thread on a new session */
long sys_start(unsigned uid, int exec, int in, int out, CSTR command);
/* Fork the current task and copy some attribute */
long sys_fork(int clone);
long sys_sleep(long timeout);
long sys_wait(int what, unsigned id, long timeout);

// exec ve

/* --------
  Input & Output
--------- */

long sys_read(int fd, char *buf, int len);
long sys_write(int fd, const char *buf, int len);
long sys_open(int fd, CSTR path, int flags);
// long sys_open(int fd, CSTR path, int flags, ftype_t type, int mode);
long sys_close(int fd);
// lseek
// sync
// umask

// access
// ioctl
// fcntl

/* --------
  File system
--------- */

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
int sys_pipe(int *fds);
int sys_window(int width, int height, unsigned features, unsigned evmask);

/* --------
  Network
--------- */

#define NP_IP4_TCP 1
#define NP_IP4_UDP 2

int sys_socket(int protocol, const char *address, int port);

/* --------
  Memory
--------- */

void *sys_mmap(void *address, size_t length, int fd, off_t off, unsigned flags);
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
// time, pid, uid, euid, pwd, chroot,

long sys_ginfo(unsigned info, void *buf, int len);
long sys_sinfo(unsigned info, const void *buf, int len);
long sys_log(CSTR msg);
long sys_sysctl(int cmd, void *args);
long sys_copy(int out, int in, size_t size, int count);
