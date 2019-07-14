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
#include <kernel/task.h>
#include <kernel/vfs.h>
#include <kernel/memory.h>
#include <stdatomic.h>
#include <sys/signum.h>
#include <string.h>
#include <assert.h>
#include <errno.h>


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

// TODO -- Put that on a config header !
#define MIN_PID 1
#define MAX_PID 99999
#define TASK_DEFAULT_PRIO 20


unsigned AUTO_PID = MIN_PID;
bool pid_init = false;
bbtree_t pid_tree;
splock_t tsk_lock;

static task_t *task_search_unkocked(pid_t pid)
{
    if (!pid_init) {
        bbtree_init(&pid_tree);
        pid_init = true;
    }

    task_t *task = bbtree_search_eq(&pid_tree, pid, task_t, bnode);
    return task;
}


static task_t *task_allocat()
{
    task_t *task = (task_t *)kalloc(sizeof(task_t));
    task->rcu = 2;
    task->kstack = (size_t *)kmap(KSTACK, NULL, 0, VMA_STACK_RW | VMA_RESOLVE);
    task->kstack_len = KSTACK;

    task->status = TS_ZOMBIE;
    task->prio = TASK_DEFAULT_PRIO;

    splock_lock(&tsk_lock);
    do {
        task->pid = AUTO_PID++;
        if (AUTO_PID > MAX_PID)
            AUTO_PID = MIN_PID;
    } while (task_search_unkocked(task->pid) != NULL);
    task->bnode.value_ = task->pid;
    bbtree_insert(&pid_tree, &task->bnode);
    splock_unlock(&tsk_lock);
    return task;
}

task_t *task_search(pid_t pid)
{
    splock_lock(&tsk_lock);
    task_t *task = task_search_unkocked(pid);
    if (task != NULL) {
        atomic_inc(&task->rcu);
        // kprintf(-1, "TASK OPEN %s\n", task->name);
    }
    splock_unlock(&tsk_lock);
    return task;
}


task_t *task_kernel_thread(void *func, void *param)
{
    // assert(kSYS.sys_usr != NULL);
    // assert(kSYS.sys_rxfs != NULL);
    task_t *task = task_open(NULL, NULL, kSYS.init_fs, NULL);
    // task_t *task = task_open(NULL, kSYS.sys_usr, kSYS.sys_rxfs, NULL);
    task_setup(task, func, param);
    return task;
}

task_t *task_create(void *func, void *param, CSTR name)
{
    task_t *task = task_kernel_thread(func, param);
    task->name = strdup(name);
    return task;
}


task_t *task_fork(task_t *parent, int keep, const char **envs)
{
    task_t *fork = task_allocat();
    fork->parent = parent;
    // fork->envs = keep & KEEP_ENVIRON ? env_open(parent->envs) : env_create(envs);
    fork->resx = keep & KEEP_FILES ? resx_open(parent->resx) : resx_create();
    fork->resx_fs = resx_fs_create();
    fork->fs = keep & KEEP_FS ? rxfs_open(parent->fs) : rxfs_clone(parent->fs);
    fork->usmem = keep & KEEP_VM ? mspace_open(parent->usmem) : mspace_create();
    // fork->usr = usr_open(parent->usr);
    return fork;
}

task_t *task_open(task_t *parent, usr_t *usr, rxfs_t *fs, const char **envs)
{
    task_t *fork = task_allocat();
    fork->parent = parent;
    // fork->envs = env_create(envs);
    fork->resx = resx_create();
    fork->resx_fs = resx_fs_create();
    fork->fs = rxfs_open(fs);
    fork->usmem = NULL; // mspace_create();
    // fork->usr = usr_open(usr);
    return fork;
}


void task_setup(task_t *task, void *entry, void *param)
{
    cpu_stack(task, (size_t)entry, (size_t)param);
    scheduler_add(task);
}


void task_close(task_t *task)
{
    // kprintf(-1, "TASK CLOSE %s\n", task->name);
    if (atomic_fetch_sub(&task->rcu, 1) != 1)
        return;

    // kprintf(-1, "TASK DESTROY %s\n", task->name);
    assert(task->status == TS_ZOMBIE);
    splock_lock(&task->lock);

    if (task->usmem)
        mspace_close(task->usmem);
    resx_close(task->resx);
    resx_fs_close(task->resx_fs);

    kunmap(task->kstack, task->kstack_len);

    bbtree_remove(&pid_tree, task->pid);
    splock_unlock(&task->lock);
    kfree(task->name);
    kfree(task);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


void task_core(task_t *task)
{
    kprintf(KLOG_DBG, "Dump core - Task #%d =========================\n", task->pid);
    stackdump(10);
    if (task->usmem)
        mspace_display(task->usmem);
    mspace_display(kMMU.kspace);
    kprintf(KLOG_DBG, "Dump core - Task #%d =========================\n", task->pid);
}




void task_show_all()
{
    static char *status = "ZBWRE???????";
    static char buf1[10];
    static char buf2[10];
    splock_lock(&tsk_lock);
    task_t *task = bbtree_first(&pid_tree, task_t, bnode);
    kprintf(-1, "  PID PPID USER    PR ST %%CPU %%MEM   V.MEM    P.MEM   UP TIME  NAME\n");
    for (; task; task = bbtree_next(&task->bnode, task_t, bnode)) {
        // PID / USER / PRIO / VIRT / RES / SHR / ST / %CPU %MEM  TIME+ CMD
        if (task->usmem != NULL)
            kprintf(-1, " %4d %4d %8s %2d  %c  0.0  %3d  %s  %s  00:00:00 %s\n",
                    task->pid, task->parent->pid, "no-user", 0,
                    status[task->status],
                    task->usmem->p_size * 100 / kMMU.pages_amount,
                    task->usmem ? sztoa_r(task->usmem->v_size, buf1) : "      -",
                    task->usmem ? sztoa_r(task->usmem->p_size * PAGE_SIZE, buf2) : "      -",
                    task->name);
        else if (task->parent != NULL)
            kprintf(-1, " %4d %4d %8s %2d  %c  0.0  0.0        -        -  00:00:00 %s\n",
                    task->pid, task->parent->pid, "no-user", 0,
                    status[task->status], task->name);
        else
            kprintf(-1, " %4d    - %8s %2d  %c  0.0  0.0        -        -  00:00:00 %s\n",
                    task->bnode.value_, "no-user", 0,
                    status[task->status], task->name);
    }
    splock_unlock(&tsk_lock);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */




int task_stop(task_t *task, int code)
{
    splock_lock(&task->lock);
    if (task == kCPU.running) {
        // Nothing to do !?
    } else if (task->status == TS_READY)
        scheduler_rm(task, TS_ZOMBIE);
    else if (task->status == TS_RUNNING) {
        task->status = TS_ABORTED;
        task->retcode = code;
        splock_unlock(&task->lock);
        return 0;
    } else if (task->status == TS_ZOMBIE) {
        splock_unlock(&task->lock);
        return -1;
    } else if (task->status == TS_BLOCKED) {
        task->status = TS_ZOMBIE;
        splock_unlock(&task->lock);
    } else
        assert(false);
    task->status = TS_ZOMBIE;
    task->retcode = code;
    task_t *parent = task->parent;
    // TODO - All children become orphans
    splock_unlock(&task->lock);
    if (parent)
        task_kill(parent, SIGCHLD);
    // event_trigger(EV_TASK_DIE, task);

    if (task == kCPU.running) {
        task_close(task);
        kCPU.running = NULL;
        scheduler_switch(TS_ZOMBIE, 0);
    }
    task_close(task);
    return 0;
}

int task_resume(task_t *task)
{
    splock_lock(&task->lock);
    if (task->status <= TS_ZOMBIE || task->status >= TS_READY) {
        splock_unlock(&task->lock);
        return -1;
    }

    task->status = TS_READY;
    scheduler_add(task);
    splock_unlock(&task->lock);
    return 0;
}


int task_kill(task_t *task, unsigned signum)
{
    if (signum > 31) {
        errno = EINVAL;
        return -1;
    }

    splock_lock(&task->lock);
    task->recieved_signal |= 1 << signum;
    //     if (task->status == TS_INTERRUPTIBLE) {
    //         task->status = TS_READY;
    //         scheduler_add(task);
    //     }
    splock_unlock(&task->lock);
    return 0;
}


// _Noreturn int task_pause(int status)
// {
//     assert (status > TS_ZOMBIE && status < TS_READY);
//     task_t *task = kCPU.running;
//     splock_lock(&task->lock);
//     if (task->status == TS_ABORTED) {
//         task_stop(task, task->retcode);
//         scheduler_next();
//     }
//     // TODO
//     task->status = status;
//     scheduler_rm(task);
//     splock_unlock(&task->lock);
//     scheduler_next();
// }

// /* Handle signal of the current task */
// void task_signals()
// {
//     task_t *task = kCPU.running;
//     if (!cpu_task_return_uspace(task) || task->sig_regs != NULL) {
//         return;
//     }
//     // handle signals
//     while (task->recieved_signal != 0) {
//         int sn = 0;
//         while (((task->recieved_signal >> sn) & 1) == 0) {
//             sn++;
//         }
//         task->recieved_signal &= ~(1 << sn);
//         sig_handler_t *sig = &task->shandler[sn];
//         if (sn == SIGKILL) {
//             kprintf(KLOG_TSK, "[TSK ] Task %d recived SIGKILL\n", task->pid);
//             task_stop(task, -2);
//             scheduler_next();
//         } else if (sig->type == SIG_IGN) {
//             continue;
//         } else if (sig->type == SIG_DFL) {
//             if (sn == SIGHUP || sn == SIGINT || sn == SIGQUIT || sn == SIGSEGV) {
//                 kprintf(KLOG_TSK, "[TSK ] Task %d recived SIG #%d\n", task->pid, sn);
//                 task_core(task);
//                 task_stop(task, -2);
//                 scheduler_next();
//                 break;
//             }
//         } else {
//             // TODO -- Ensure sig->type is placed on user memory space.
//             kprintf(KLOG_TSK, "[TSK ] Signal custum routine PID:%d\n", kCPU.running->pid);
//             cpu_setup_signal(task, (size_t)sig->type, sn);
//         }
//     }
// }

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


// /**
//  * Indicate that current task is enter into Kernel code.
//  */
// void task_enter_sys(regs_t *regs, bool kernel)
// {
//     task_t *task = kCPU.running;
//     if (task) {
//         splock_lock(&task->lock);
//         task->regs/*[++task->rp]*/ = regs;
//         splock_unlock(&task->lock);
//         if (kernel) {
//             task->sys_elapsed += clock_elapsed(&task->last);
//             kCPU.sys_elapsed += clock_elapsed(&kCPU.last);
//         } else {
//             task->user_elapsed += clock_elapsed(&task->last);
//             kCPU.user_elapsed += clock_elapsed(&kCPU.last);
//         }
//     } else {
//         kCPU.sys_elapsed += clock_elapsed(&kCPU.last);
//     }
// }

// void task_leave_sys()
// {
//     task_t *task = kCPU.running;
//     kCPU.sys_elapsed += clock_elapsed(&kCPU.last);
//     if (task) {
//         splock_lock(&task->lock);
//         // --task->rp;
//         splock_unlock(&task->lock);
//         task->sys_elapsed += clock_elapsed(&task->last);
//     }
// }

_Noreturn void task_fatal(CSTR error, unsigned signum)
{
    if (kCPU.running != NULL) {
        kprintf(KLOG_ERR, "Fatal error on CPU.%d Task #%d: \033[91m%s\033[0m\n", cpu_no(), kCPU.running->pid, error);
        task_core(kCPU.running);
        // task_kill(kCPU.running, signum);
        task_stop(kCPU.running, -1);
    }
    kprintf(KLOG_ERR, "Unrecoverable error on CPU.%d: \033[91m%s\033[0m\n", cpu_no(), error);
    // Disable scheduler
    // Play dead screen
    for (;;); // scheduler_switch(0, -1);
}

