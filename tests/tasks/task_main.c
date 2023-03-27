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
#include "../cli.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <kora/mcrs.h>
#include <kernel/stdc.h>
#include <kernel/vfs.h>
#include <kernel/memory.h>
#include <kernel/tasks.h>
#include <setjmp.h>
#include <sys/signum.h>

struct
{
    masterclock_t *mclock;
    hmap_t usermode_tasks;
} _;


int vfs_write(inode_t *ino, const char *buf, size_t size, xoff_t off, int flags) { return -1; }
// void vfs_usage(inode_t *ino, int access, int count) {}
inode_t *vfs_open_inode(inode_t *ino) { return  ino; }
void vfs_close_inode(inode_t *ino) { }
fs_anchor_t *vfs_open_vfs(fs_anchor_t *fsanchor) { return fsanchor; }
fs_anchor_t *vfs_clone_vfs(fs_anchor_t *fsanchor) { return fsanchor; }
char *vfs_inokey(inode_t *ino, char *buf)
{
    strcpy(buf, "?");
    return buf;
}

// inode_t *vfs_search_ino(fs_anchor_t *fsanchor, const char *pathname, user_t *user, bool follow)  { return NULL;}

size_t vfs_fetch_page(inode_t *ino, xoff_t off, bool blocking)
{
    return 0;
}

int vfs_release_page(inode_t *ino, xoff_t off, size_t pg, bool dirty)
{
    return -1;
}

/* Create a memory space for a user application */
vmsp_t *vmsp_create() { return NULL; }
/* Increment memory space RCU */
vmsp_t *vmsp_open(vmsp_t *mspace) { return NULL; }
/* Decrement memory space RCU */
void vmsp_close(vmsp_t *mspace) { }
/* Copy all VMA and associated pages */
vmsp_t *vmsp_clone(vmsp_t *model) { return NULL; }

void *kmap(size_t len, void *ino, xoff_t off, int access) { return NULL; }

const char *ksymbol(size_t ptr, char *buf, size_t len) { return buf; }

size_t vmsp_map(vmsp_t *vmsp, size_t address, size_t length, void *ptr, xoff_t offset, int flags) { return 0; }

void mmu_context(vmsp_t *mspace) {}
// void mmu_context() {}

int dlib_open(vmsp_t *mm, fs_anchor_t *fsanchor, user_t *user, const char *name)
{
    return 0;
}

void irq_zero() {}


int cpu_save(cpu_state_t *jbuf) { return 0; }

// _Noreturn void cpu_restore(task_t *task) { for (;;); }
// _Noreturn void cpu_halt() { for (;;); }

_Noreturn void cpu_usermode(void *start, void *stack) { for (;;); }

_Noreturn void task_usermode(task_params_t *info);

void cpu_prepare(cpu_state_t *jbuf, void *stack, void *func, void *arg)
{
    task_t *task = itemof(jbuf, task_t, jmpbuf);
    if (func == &task_usermode) {
        task_params_t *info = arg;
        kfree(info->params);
        kfree(info);
        hmp_put(&_.usermode_tasks, (char *)&task->pid, sizeof(int), task);
    }
}

bool __is_userspace(task_t *task)
{
    task_t *saved = hmp_get(&_.usermode_tasks, (char *)&task->pid, sizeof(int));
    if (saved == NULL)
        return false;
    return true;
}


// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

void advent_register(masterclock_t *clock, advent_t *advent, long timeout, long interval);

void advent_unregister(masterclock_t *clock, advent_t *advent);

void advent_pause_task(masterclock_t *clock, advent_t *advent, long timeout);

void advent_resume_task(masterclock_t *clock, advent_t *advent);

int futex_wait(int *addr, int val, long timeout, int flags);

int futex_requeue(int *addr, int val, int val2, int *addr2, int flags);

int futex_wake(int *addr, int val, int flags);

void itimer_create(inode_t *ino, long delay, long interval);

xtime_t sleep_timer(long timeout);

masterclock_t *clock_init(xtime_t now);

void clock_adjtime(masterclock_t *clock, xtime_t now);

xtime_t clock_read(masterclock_t *clock, xtime_name_t name);

void clock_ticks(masterclock_t *clock, unsigned elapsed);

void clock_handler(masterclock_t *clock);

void scheduler_init(/*fs_anchor_t *fsanchor, void *net*/);

int resx_put(streamset_t *strms, int type, void *data, void(*close)(void *));

void *resx_get(streamset_t *strms, int type, int handle);

void resx_remove(streamset_t *strms, int handle);

size_t task_spawn(const char *program, const char **args, inode_t **nodes);

size_t task_thread(const char *name, void *entry, void *params, size_t len, int flags);

size_t task_start(const char *name, void *func, void *arg);

void task_stop(int code);

void task_raise(task_t *task, unsigned signum);


char *__clock_time_str(xtime_t time, char *buf)
{
    time_t now = USEC_TO_SEC(time);
    // struct tm tm;
    strftime(buf, 32, "%Y-%m-%dT%H:%M:%SZ", gmtime(&now));
    return buf; // ctime(&now);
}

char *__clock_duration_str(xtime_t time, char *buf)
{
    // Max is 106 751 991 days (292 471 years)
    int days = time / (24LL * 3600 * 1000000LL);
    time -= days * (24LL * 3600 * 1000000LL);
    int hours = time / (3600 * 1000000LL);
    time -= hours * (3600 * 1000000LL);
    int mins = time / (60 * 1000000LL);
    time -= mins * (60 * 1000000LL);
    int secs = time / 1000000LL;
    time -= secs * 1000000LL;
    int ms = time / 1000LL;
    time -= ms * 1000LL;

    if (days > 0)
        snprintf(buf, 32, "%dd.%02d:%02d:%02d.%03d (%03d)", days, hours, mins, secs, ms, (int)time);
    else if (hours > 1)
        snprintf(buf, 32, "%02d:%02d:%02d.%03d (%03d)", hours, mins, secs, ms, (int)time);
    else if (mins > 0)
        snprintf(buf, 32, "%02d'%02d\"%03d (%03d)", mins, secs, ms, (int)time);
    else
        snprintf(buf, 32, "%02d\"%03d (%03d)", secs, ms, (int)time);
    return buf;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#define ST_PID 1
#define ST_ADVENT 2


int do_start(void *ctx, size_t *params)
{
    char *name = (char *)params[0];
    char *store = (char *)params[1];
    size_t pid = task_start(name, NULL, NULL);
    if (pid == 0)
        return cli_error("Unable to create new task");
    if (store)
        cli_store(store, (void *)pid, ST_PID);
    return 0;
}

int do_spawn(void *ctx, size_t *params)
{
    char *name = (char *)params[0];
    char *store = (char *)params[1];
    inode_t *inodes[3] = { (inode_t*)0x1000, (inode_t *)0x2000, (inode_t *)0x3000 };
    size_t pid = task_spawn(name, NULL, inodes);
    if (pid == 0)
        return cli_error("Unable to create new task");
    if (store)
        cli_store(store, (void *)pid, ST_PID);
    return 0;
}

int do_thread(void *ctx, size_t *params)
{
    char *name = (char *)params[0];
    char *store = (char *)params[1];
    size_t pid = task_thread(name, NULL, NULL, 0, 0);
    if (pid == 0)
        return cli_error("Unable to create new task");
    if (store)
        cli_store(store, (void *)pid, ST_PID);
    return 0;
}

int do_fork(void *ctx, size_t *params)
{
    // char *name = (char *)params[0];
    char *store = (char *)params[1];
    size_t pid = 0; // task_fork(name, NULL, NULL, 0, 0);
    if (pid == 0)
        return cli_error("Unable to create new task");
    if (store)
        cli_store(store, (void *)pid, ST_PID);
    return 0;
}

int do_stop(void *ctx, size_t *params)
{
    int code = (int)params[0];
    // size_t pid = params[1];

    // task_t *task = pid == 0 ? __current : task_search(pid);
    // if (task == NULL)
        // return cli_error("No task selected");
    task_stop(code);
    return 0;
}


int do_tick(void *ctx, size_t *params)
{
    size_t count = MAX(1, (size_t)params[0]);
    size_t pid = params[1];
    while (count-- > 0) {
        clock_state(CKS_IRQ);
        clock_handler(_.mclock);
        if (__current != NULL && __is_userspace(__current))
            clock_state(CKS_USER);
    }

    if (__current == NULL)
        return cli_error("Can't find task %d on scheduler", pid);

    size_t old = __current->pid;
    while (__current->pid != pid && pid != 0) {
        clock_state(CKS_IRQ);
        clock_handler(_.mclock);
        if (__current != NULL && __is_userspace(__current))
            clock_state(CKS_USER);

        if (__current->pid == old)
            return cli_error("Can't find task %d on scheduler", pid);
    }
    printf("Execute task.%ld\n", __current->pid);
    return 0;
}

int do_yield(void *ctx, size_t *params)
{
    scheduler_switch(TS_READY);
    if (__current != NULL && __is_userspace(__current))
        clock_state(CKS_USER);
    return 0;
}


int do_uptime(void *ctx, size_t * params)
{
    char tmp[32];
    printf("System time: %s\n", __clock_time_str(_.mclock->wall, tmp));
    printf("Boot time: %s\n", __clock_duration_str(_.mclock->boot, tmp));
    printf("Monotonic: %s\n", __clock_duration_str(_.mclock->monotonic, tmp));
    return 0;
}

int do_show(void *ctx, size_t *params)
{
    static char status_mark[] = { 'Z', 'B', 'I', 'R', 'E', 'e' };
    size_t pid = params[0];

    xtime_t now = xtime_read(XTIME_CLOCK);
    printf(" PID PPID  S  CPU  NAME\n");

    task_t *task = task_search(pid);
    for (; task && pid == 0; task = task_next(task->pid)) {
        task_t *parent = task->parent;
        int cpu_time = (int)((task->elapsed_counters[CKS_USER] + task->elapsed_counters[CKS_SYS]) * 100 / (now - task->start_time));
        if (parent)
            printf(" %3ld  %3ld  %c  %2d%%  %s\n", task->pid, parent->pid, status_mark[task->status], cpu_time, task->name);
        else
            printf(" %3ld    -  %c  %2d%%  %s\n", task->pid, status_mark[task->status], cpu_time, task->name);
    }
    // NAME / PID / Desc / STATUS / THRD? / %CPU / %MEM / NICE
    // CPU: USER / KERNEL / START
    // MEM: VSZ / PSZ / SSZ / ... / PG /
    // IO / NET : READ / WRITE
    return 0;
}


int do_kill(void *ctx, size_t *params)
{
    static char *__signames[] = {
        "", "SIGHUP", "SIGINT", "SIGQUIT", "SIGILL", "SIGTRAP", "SIGABRT", "SIGBUS",
        "SIGFPE", "SIGKILL", "SIGUSR1", "SIGSEGV", "SIGUSR2", "SIGPIPE", "SIGALRM", "SIGTERM",
        "SIGSTKFLT", "SIGCHLD", "SIGCONT", "SIGSTOP", "SIGTSTP", "SIGTTIN", "SIGTTOU", "SIGURG",
        "SIGXCPU", "SIGXFSZ", "SIGVTALRM", "SIGPROF", "SIGWINCH", "SIGIO", "SIGPWR", "SIGSYS",
    };

    size_t pid = params[0];
    char *signal = (char *)params[1];
    task_t *task = pid == 0 ? __current : task_search(pid);
    if (task == NULL)
        return cli_error("No task selected");

    int signum = 0;
    if (*signal != 'S')
        signum = strtoul(signal, NULL, 10);
    else {
        for (int i = 0; i < 3; ++i) {
            if (strcmp(signal, __signames[i]) == 0) {
                signum = i;
                break;
            }
        }
    }
    if (signum == 0)
        return cli_error("Undefined signal number %s", signal);

    task_raise(task, signum);
    return 0;
}

void __dtor_advent(masterclock_t *clock, advent_t *advent)
{
    kfree(advent);
    cli_remove((char *)advent->object, ST_ADVENT);
    free(advent->object);
}

int do_wait(void *ctx, size_t *params)
{
    char *store = (char *)params[0];
    size_t timeout = params[1];

    advent_t *advent = kalloc(sizeof(advent_t));
    advent->wake = advent_resume_task;
    advent->dtor = __dtor_advent;
    advent->object = strdup(store);

    cli_store(store, advent, ST_ADVENT);
    advent_pause_task(_.mclock, advent, timeout);
    return 0;
}

int do_awake(void *ctx, size_t *params)
{
    char *store = (char *)params[0];
    advent_t *advent = cli_fetch(store, ST_ADVENT);
    if (advent == NULL)
        return cli_error("Unable to find the element");
    advent_resume_task(_.mclock, advent);
    return 0;
}

//FUTEX !!?

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

void initialize()
{
    hmp_init(&_.usermode_tasks, 8);
    xtime_t now = xtime_read(XTIME_CLOCK);
    _.mclock = clock_init(now);
    scheduler_init(NULL, NULL); // VFS NET -- TO REMOVE !!?
}

int alloc_check();

int do_quit(void *ctx, size_t *param)
{
    return alloc_check();
}

void teardown()
{
}


cli_cmd_t __commands[] = {
    { "QUIT", "", { 0, 0, 0, 0, 0 }, (void *)do_quit, 1 },

    { "START", "", { ARG_STR, ARG_STR, 0, 0, 0 }, (void *)do_start, 1 },
    { "SPAWN", "", { ARG_STR, ARG_STR, 0, 0, 0 }, (void *)do_spawn, 1 },
    { "THREAD", "", { ARG_STR, ARG_STR, 0, 0, 0 }, (void *)do_thread, 1 },
    { "FORK", "", { ARG_STR, ARG_STR, 0, 0, 0 }, (void *)do_thread, 1 },
    { "STOP", "", { ARG_INT, ARG_INT, 0, 0, 0 }, (void *)do_stop, 0 },

    { "TICK", "", { ARG_INT, ARG_INT, 0, 0, 0 }, (void *)do_tick, 0 },
    { "YIELD", "", { 0, 0, 0, 0, 0 }, (void *)do_yield, 0 },
    { "UPTIME", "", { 0, 0, 0, 0, 0 }, (void *)do_uptime, 0 },
    { "SHOW", "", { ARG_INT, 0, 0, 0, 0 }, (void *)do_show, 1 },

    { "KILL", "", { ARG_INT, ARG_STR, 0, 0, 0 }, (void *)do_kill, 2 },

    { "WAIT", "", { ARG_STR, ARG_INT, 0, 0, 0 }, (void *)do_wait, 1 },
    { "AWAKE", "", { ARG_STR, 0, 0, 0, 0 }, (void *)do_awake, 1 },

    CLI_END_OF_COMMANDS,
};

int main(int argc, char **argv)
{
    return cli_main(argc, argv, NULL, initialize, teardown);
}
