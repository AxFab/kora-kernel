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
#ifndef _KERNEL_TASK_H
#define _KERNEL_TASK_H 1

#include <kernel/core.h>
#include <kernel/cpu.h>
#include <kernel/memory.h>
#include <kora/bbtree.h>
#include <kora/splock.h>
#include <kora/rwlock.h>

typedef struct regs regs_t;
typedef struct user user_t;
typedef struct sig_handler sig_handler_t;

typedef struct stream stream_t;
typedef struct resx resx_t;
typedef struct resx_fs resx_fs_t;

struct user {
    uint8_t id[16];
};

#define FORK_THREAD  (1 << 2)
#define FORK_VM  (1 << 3)  /* Fork the memory address space */
#define FORK_FILES  (1 << 0)  /* Fork open files */
#define FORK_FS  (1 << 1)  /* Fork file system information */
// PARENT, TLS, SIGNAL, SCALL


enum TS_TaskState {
    TS_ZOMBIE = 0,  /* The task structure is not used. */
    TS_BLOCKED,  /* The task is paused and listen only for its own events. */
    TS_INTERRUPTIBLE,  /* The task is paused and wait for any events. */
    TS_READY,  /* The task is waiting for cpu time. */
    TS_RUNNING,  /* Task is currently running on a cpu. */
    TS_ABORTED,  /* Task should be aborded, but is currently running. */
};


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


struct stream {
    inode_t *ino;
    off_t off;
    int flags;
    bbnode_t node; // TODO -- Is BBTree the best data structure !?
    rwlock_t lock; // TODO -- Usage
};

struct resx {
    bbtree_t tree; // TODO -- Is BBTree the best data structure !?
    rwlock_t lock;
    atomic_int users;
};

struct resx_fs {
    inode_t *root;  /* Inode used as a root for this task */
    inode_t *pwd;
    int umask;
    atomic_int users;
};

struct sig_handler {
    int num;
    void *type;
};


struct advent {
    int type;
    ftx_t *futex;
    task_t *task;
    llnode_t node;
    llnode_t anode;
    llnode_t tnode;
    clock_t until;
    pipe_t *pipe;
    long interval;
    void (*dtor)(advent_t*);
};


struct task {
    size_t *kstack;  /* Kernel stack base address */
    size_t *ustack;  /* User space stack base address */
    size_t kstack_len;  /* Kernel stack length */
    size_t ustack_len;  /* User space stack length */
    uint8_t status;  /* Task state (see enum TS_TaskState) */
    int prio;
    int retcode;
    // int rp;
    char *name;
    cpu_state_t state;
    // regs_t *regs/*[8]*/;
    // regs_t *sig_regs;
    splock_t lock;
    atomic_int rcu;

    sig_handler_t shandler[32];

    uint32_t cpu; /* The cpu which run the task. */
    int err_no;
    uint32_t recieved_signal;
    bbnode_t bnode;

    task_t *parent;

    /* Scheduler entity */
    llnode_t node;

    // unsigned long stack_canary;

    // struct timespec start_time;
    uint64_t last;  /* Register to compute elpased time. Unit is platform dependant. */
    uint64_t user_elapsed;  /* Time spend into user space code */
    uint64_t sys_elapsed;  /* Time spend into kernel space code */
    uint64_t other_elapsed;  /* Time spend on other task */

    user_t *user;
    proc_t *proc;
    usr_t *usr;
    rxfs_t *fs;
    char *envs;

    resx_t *resx;  /* Open files */
    resx_fs_t *resx_fs;  /* File system information */
    mspace_t *usmem;  /* User space memory */

    /* Thread Local Storage */
    /* Signal handling */
    /* System calls */

    /* Thread group */
    pid_t pid;

    /* Memory address space */


    llhead_t alist;
};

#define TSK_USER_SPACE  0x001

#define KEEP_ENVIRON  1
#define KEEP_FILES  2
#define KEEP_FS  4
#define KEEP_VM  8


task_t *task_fork(task_t *parent, int keep, const char **envs);
task_t *task_open(task_t *parent, usr_t *usr, rxfs_t *fs, const char **envs);


int task_stop(task_t *task, int code);
int task_kill(task_t *task, unsigned signum);
int task_resume(task_t *task);
void task_close(task_t *task);

_Noreturn int task_pause(int state);
void task_signals();

task_t *task_create(void *entry, void *param, CSTR name);
task_t *task_clone(task_t *model, int clone, int flags);

void task_setup(task_t *task, void *entry, void *param);
task_t *task_search(pid_t pid);
void task_show_all();

_Noreturn void cpu_halt();

void scheduler_add(task_t *item);
void scheduler_rm(task_t *item, int status);
void scheduler_switch(int status, int retcode);

char *env_open(char *);
char *env_create(const char **);

rxfs_t *rxfs_create(inode_t *ino);
rxfs_t *rxfs_open(rxfs_t *);
rxfs_t *rxfs_clone(rxfs_t *);
void rxfs_chroot(rxfs_t *fs, inode_t *ino);
void rxfs_chdir(rxfs_t *fs, inode_t *ino);

usr_t *usr_open(usr_t *);


/* Wait for an event to be emited */
int async_wait(splock_t *lock, emitter_t *emitter, long timeout_us);
int async_wait_rd(rwlock_t *lock, emitter_t *emitter, long timeout_us);
int async_wait_wr(rwlock_t *lock, emitter_t *emitter, long timeout_us);
void async_cancel(task_t *task);
void async_raise(emitter_t *emitter, int err);
void async_timesup();



void itimer_create(pipe_t *pipe, long delay, long interval);

int elf_open(task_t *task, inode_t *ino);

void cpu_setup_task(task_t *task, size_t entry, long args);
void cpu_setup_signal(task_t *task, size_t entry, long args);
void cpu_return_signal(task_t *task, regs_t *regs);

bool cpu_task_return_uspace(task_t *task);

void cpu_stack(task_t *task, size_t entry, size_t param);
int cpu_save(cpu_state_t state);
void cpu_restore(cpu_state_t state);


void clock_ticks();
void clock_init();


void sleep_timer(long timeout);


resx_fs_t *resx_fs_create();
resx_fs_t *resx_fs_open(resx_fs_t *resx);
void resx_fs_close(resx_fs_t *resx);
inode_t *resx_fs_root(resx_fs_t *resx);
inode_t *resx_fs_pwd(resx_fs_t *resx);
void resx_fs_chroot(resx_fs_t *resx, inode_t *ino);
void resx_fs_chpwd(resx_fs_t *resx, inode_t *ino);


resx_t *resx_create();
resx_t *resx_open(resx_t *resx);
void resx_close(resx_t *resx);
stream_t *resx_get(resx_t *resx, int fd);
stream_t *resx_set(resx_t *resx, inode_t *ino);
int resx_rm(resx_t *resx, int fd);

struct rxfs {
    inode_t *root;
    inode_t *pwd;
    int umask;
    atomic_int rcu;
};


typedef struct proc_start {
    char *path;
    char **argv;
    int argc;
    inode_t *stdout[3];
} proc_start_t;

typedef struct task_start {
    char *func;
    char *args;
    int sz;
} task_start_t;

void exec_kloader();
void exec_init();
void exec_process(proc_start_t *info);
void exec_thread(task_start_t *info);


#endif  /* _KERNEL_TASK_H */
