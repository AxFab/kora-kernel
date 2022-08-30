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
#ifndef _KERNEL_TASKS_H
#define _KERNEL_TASKS_H 1

#include <kernel/stdc.h>
#include <kora/llist.h>
#include <kora/bbtree.h>
#include <kora/splock.h>
#include <kernel/vfs.h>
#include <kernel/memory.h>
#include <kernel/arch.h>
#include <kernel/dlib.h>

#define FUTEX_CREATE 1
#define FUTEX_SHARED 2

#define KEEP_FS 1
#define KEEP_FSET 2
#define KEEP_VM 4


#define RESX_FILE 1
#define RESX_DIR 1

enum task_status {
    TS_ZOMBIE = 0,  /* The task structure is not used. */
    TS_BLOCKED,  /* The task is paused and listen only for its own events. */
    TS_INTERRUPTIBLE,  /* The task is paused and wait for any events. */
    TS_READY,  /* The task is waiting for cpu time. */
    TS_RUNNING,  /* Task is currently running on a cpu. */
    TS_ABORTED,  /* Task should be aborded, but is currently running. */
};


#define CKS_NONE 0
#define CKS_IDLE 1
#define CKS_USER 2
#define CKS_SYS 3
#define CKS_IRQ 4
#define CKS_MAX 5

typedef struct task task_t;
typedef enum task_status task_status_t;
typedef struct scheduler scheduler_t;
typedef void *sig_handler_t;

typedef struct task_params task_params_t;
typedef struct masterclock masterclock_t;
typedef struct advent advent_t;
typedef struct streamset streamset_t;


struct scheduler {
    bbtree_t task_tree;
    splock_t lock;

    splock_t sch_lock;
    llhead_t sch_queue;
    llhead_t sch_zombie;

    // fs_anchor_t *vfs;
    // void *net;
};


struct masterclock {
    llhead_t list;
    splock_t lock;
    splock_t tree_lock;
    bbtree_t tree;
    xtime_t wall;
    xtime_t boot;
    xtime_t monotonic;
    xtime_t next_advent;

    int last_state;
    xtime_t last_time;
    xtime_t elapsed_counters[CKS_MAX];
};

struct task {
    size_t pid;
    char *name;
    void *stack;
    cpu_state_t jmpbuf;
    task_status_t status;
    bbnode_t node;
    splock_t lock;

    task_t *parent;
    int err_no;
    int ret_code;

    uint32_t raised;
    sig_handler_t shandler[32];

    char sc_log[64];
    int sc_len;

    llhead_t advent_list;

    llnode_t sch_node;

    mspace_t *vm;
    fs_anchor_t *fsa;
    streamset_t *fset;
    void *net;
    user_t *user;
    // proc_t *proc;
    xtime_t elapsed_counters[CKS_MAX];
    xtime_t start_time;
    xtime_t kill_time;
};

struct task_params
{
    void *func;
    size_t *params;
    size_t len;
    bool start;
};

struct advent
{
    llnode_t tm_node;
    xtime_t until;
    unsigned repeat;
    bool did_timeout;
    task_t *task;

    void *object;
    llnode_t node;

    void (*wake)(masterclock_t *, advent_t *);
    void (*dtor)(masterclock_t *, advent_t *);
};

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
extern scheduler_t __scheduler;
extern task_t *__current; // TODO - Per CPU
extern masterclock_t __clock;

// void cpu_setjmp(cpu_state_t *buf, void *stack, void *func, void *arg);
// void cpu_prepare(cpu_state_t *buf, void *stack, void *func, void *arg);
void cpu_prepare(task_t *task, void *func, void *arg);
//int cpu_save(cpu_state_t *buf);
//_Noreturn void cpu_restore(cpu_state_t *buf);
int cpu_save(task_t *task);
_Noreturn void cpu_restore(task_t *task);
_Noreturn void cpu_halt();
_Noreturn void cpu_usermode(void *start, void *stack);
int cpu_no();


// int task_fork(const char *name, void *func, void *arg, int flags);
task_t *task_search(size_t pid);
task_t *task_next(size_t pid);
size_t task_start(const char *name, void *func, void *arg);
void task_raise(scheduler_t *sch, task_t *task, unsigned signum);
void task_stop(task_t *task, int code);
// void task_fatal(const char *msg, unsigned signum);

size_t task_spawn(const char *program, const char **args, inode_t **nodes);
size_t task_thread(const char *name, void *entry, void *params, size_t len, int flags);

// _Noreturn void task_firstinit();
// _Noreturn void task_init(task_info_t *info);
// _Noreturn void task_thread(task_thread_t *info);

void scheduler_add(scheduler_t *sch, task_t *task);
void scheduler_rm(scheduler_t *sch, task_t *task, int status);
void scheduler_switch(int status);


void scheduler_init();
// void scheduler_init(fs_anchor_t *fsanchor, void *net);
streamset_t *stream_create_set();
streamset_t *stream_open_set(streamset_t *strms);
void stream_close_set(streamset_t *strms);



int resx_put(streamset_t *strms, int type, void *data, void(*close)(void *));
void *resx_get(streamset_t *strms, int type, int handle);
void resx_remove(streamset_t *strms, int handle);


int futex_wait(int *addr, int val, long timeout, int flags);
int futex_requeue(int *addr, int val, int val2, int *addr2, int flags);
int futex_wake(int *addr, int val, int flags);

void itimer_create(inode_t *ino, long delay, long interval);
xtime_t sleep_timer(long timeout);


masterclock_t *clock_init(xtime_t now);
void clock_adjtime(masterclock_t *clock, xtime_t now);
xtime_t clock_read(masterclock_t *clock, xtime_name_t name);
void clock_ticks(masterclock_t *clock, unsigned elapsed);


typedef struct file file_t;
file_t *file_from_inode(inode_t *ino, int flags);
void file_close(file_t *);


void clock_state(int state);


#endif  /* _KERNEL_TASKS_H */
