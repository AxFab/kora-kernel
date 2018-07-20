/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015  <Fabien Bavent>
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
#include <bits/atomic.h>
#include <bits/signum.h>
#include <string.h>
#include <assert.h>
#include <errno.h>


void task_core(task_t *task)
{
    kprintf(KLOG_DBG, "Task Core %d =========================\n", task->pid);
    mspace_display(task->usmem);
}

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
//             task->sys_elapsed += time_elapsed(&task->last);
//             kCPU.sys_elapsed += time_elapsed(&kCPU.last);
//         } else {
//             task->user_elapsed += time_elapsed(&task->last);
//             kCPU.user_elapsed += time_elapsed(&kCPU.last);
//         }
//     } else {
//         kCPU.sys_elapsed += time_elapsed(&kCPU.last);
//     }
// }

// void task_leave_sys()
// {
//     task_t *task = kCPU.running;
//     kCPU.sys_elapsed += time_elapsed(&kCPU.last);
//     if (task) {
//         splock_lock(&task->lock);
//         // --task->rp;
//         splock_unlock(&task->lock);
//         task->sys_elapsed += time_elapsed(&task->last);
//     }
// }

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


void task_start(task_t *task, size_t entry, long args)
{
    if (task->status != TS_ZOMBIE) {
        kprintf(KLOG_ERR, "[TASK] Try to grab a used task.");
        return;
    }

    if (task->usmem && !task->ustack) {
        // TODO -- Map at the end
        task->ustack_len = 1 * _Mib_;
        task->ustack = mspace_map(task->usmem, 0, task->ustack_len, NULL, 0,
                                  0, VMA_STACK_RW);
    }

    cpu_stack(task, entry, args);
    scheduler_add(task);
}

// int task_stop(task_t *task, int code)
// {
//     splock_lock(&task->lock);
//     if (task == kCPU.running || task->status == TS_READY) {
//         scheduler_rm(task);
//     } else if (task->status == TS_RUNNING) {
//         task->status = TS_ABORTED;
//         task->retcode = code;
//         splock_unlock(&task->lock);
//         return 0;
//     } else if (task->status == TS_ZOMBIE) {
//         splock_unlock(&task->lock);
//         return -1;
//     }

//     task->status = TS_ZOMBIE;
//     task->retcode = code;

//     task_t *parent = task->parent;
//     // TODO - All children become orphans
//     splock_unlock(&task->lock);

//     if (parent) {
//         task_kill(parent, SIGCHLD);
//     }

//     // event_trigger(EV_TASK_DIE, task);

//     // Usage counter !?
//     return 0;
// }


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

int task_resume(task_t *task)
{
    splock_lock(&task->lock);
    if (task->status <= TS_ZOMBIE || task->status >= TS_READY)
        return -1;

    task->status = TS_READY;
    scheduler_add(task);
    splock_unlock(&task->lock);
    return 0;
}

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

// TODO -- Put that on a config header !
#define MIN_PID 1
#define MAX_PID 99999
#define TASK_DEFAULT_PRIO 20

unsigned AUTO_PID = MIN_PID;
bool pid_init = false;
bbtree_t pid_tree;
splock_t tsk_lock;

static task_t *task_allocat()
{
    task_t *task = (task_t *)kalloc(sizeof(task_t));
    task->kstack = (size_t *)kmap(PAGE_SIZE, NULL, 0, VMA_STACK_RW | VMA_RESOLVE);
    task->kstack_len = PAGE_SIZE;

    task->status = TS_ZOMBIE;
    task->prio = TASK_DEFAULT_PRIO;

    time_elapsed(&task->last);

    return task;
}


static pid_t task_new_pid()
{
    pid_t pid;
    // TODO -- Lock
    do {
        pid = AUTO_PID++;
        if (AUTO_PID > MAX_PID)
            AUTO_PID = MIN_PID;
    } while (task_search(pid) != NULL);
    return pid;
}

/* */
void task_switch(int status, int retcode)
{
    assert(kCPU.irq_semaphore == 1);
    assert(status >= TS_ZOMBIE && status <= TS_READY);
    task_t *task = kCPU.running;
    if (task) {
        // kprintf(-1, "Leaving Task %d\n", task->pid);
        splock_lock(&task->lock);
        task->retcode = retcode;
        if (cpu_save(task->state) != 0)
            return;
        // kprintf(-1, "Saved Task %d\n", task->pid);

        // TODO Stop task chrono
        if (task->status == TS_ABORTED) {
            if (status == TS_BLOCKED) {
                // TODO - We have advent structure to clean
            }
            status = TS_ZOMBIE;
        }
        if (status == TS_ZOMBIE) {
            /* Quit the task */
            advent_awake(&task->wlist, 0);
            // task_zombie(task);
        } else if (status == TS_READY)
            scheduler_add(task);
        task->status = status;
        splock_unlock(&task->lock);
    }

    task = scheduler_next();
    kCPU.running = task;
    irq_reset(false);
    if (task == NULL)
        cpu_halt();
    // TODO Start task chrono!
    if (task->usmem)
        mmu_context(task->usmem);
    // kprintf(-1, "Start Task %d\n", task->pid);
    cpu_restore(task->state, 1);
}

task_t *task_create(user_t *user, inode_t *root, int flags, CSTR name)
{
    task_t *task = task_allocat();
    task->pid = task_new_pid();
    task->user = user;

    kprintf(KLOG_TSK, "Create %s task #%d, %s\n",
            flags & TSK_USER_SPACE ? "user" : "kernel", task->pid, name);
    task->name = strdup(name);
    task->root = root ? vfs_open(root) : NULL;
    task->pwd = root ? vfs_open(root) : NULL;
    task->resx = resx_create();

    if (flags & TSK_USER_SPACE)
        task->usmem = mspace_create();
    task->bnode.value_ = task->pid;
    if (kCPU.running != NULL)
        task->parent = kCPU.running;
    bbtree_insert(&pid_tree, &task->bnode);
    return task;
}


// task_t *task_clone(task_t *model, int clone, int flags)
// {
//     task_t *task = task_allocat();
//     task->pid = task_new_pid();

//     // Keep file descriptor
//     if (clone & CLONE_FILES) {
//         task->resx = resx_rcu(model->resx, 1);
//     } else {
//         task->resx = resx_create();
//     }

//     // Keep FS informations
//     if (clone & CLONE_FS) {
//         task->root = vfs_open(model->root);
//         task->pwd = vfs_open(model->pwd);
//     } else {
//         task->root = NULL; // TODO - default root !?
//         task->pwd = NULL;
//     }

//     if (clone & CLONE_PARENT) {
//         task->parent = model->parent;
//     } else {
//         if (kCPU.running != NULL) {
//             task->parent = kCPU.running;
//         }
//     }


//     if (clone & CLONE_TLS) {
//     } else {
//     }


//     if (clone & CLONE_SIGNAL) {
//     } else {
//     }


//     if (clone & CLONE_SCALL) {
//     } else {
//     }


//     if (clone & CLONE_THREAD) {
//         task->usmem = mspace_open(model->usmem);
//     } else {
//         if (clone & CLONE_MSPACE) {
//         task->usmem = mspace_clone(model->usmem);
//         } else {
//             task->usmem = flags & TSK_USER_SPACE ? mspace_create() : NULL;
//         }
//     }

//     if (clone & CLONE_USER) {
//         task->user = model->user;
//     } else {
//         task->user = NULL;
//     }

//     task->bnode.value_ = task->pid;
//     bbtree_insert(&pid_tree, &task->bnode);
//     return task;
// }

void task_destroy(task_t *task)
{
    // assert(task->status == TS_ZOMBIE);

    splock_lock(&task->lock);
    vfs_close(task->root);
    vfs_close(task->pwd);

    // TODO Close all open FD
    if (task->usmem)
        mspace_close(task->usmem);
    if (task->resx)
        resx_rcu(task->resx, 0);

    kunmap(task->kstack, task->kstack_len);

    bbtree_remove(&pid_tree, task->pid);
    splock_unlock(&task->lock);
    kfree(task->name);
    kfree(task);
}

task_t *task_search(pid_t pid)
{
    splock_lock(&tsk_lock);
    if (!pid_init) {
        bbtree_init(&pid_tree);
        pid_init = true;
    }

    task_t *task = bbtree_search_eq(&pid_tree, pid, task_t, bnode);

    splock_unlock(&tsk_lock);
    return task;
}


void task_show_all()
{
    static char *status = "ZBWRE???????";
    splock_lock(&tsk_lock);
    task_t *task = bbtree_first(&pid_tree, task_t, bnode);
    // kprintf(-1, "  PID");
    for (; task; task = bbtree_next(&task->bnode, task_t, bnode)) {
        // PID / USER / PRIO / VIRT / RES / SHR / ST / %CPU %MEM  TIME+ CMD
        kprintf(-1, " %4d %8s %2d  %c  0.0  0.0  00:00:00 %s\n",
                task->bnode.value_, "no-user", 0, status[task->status], task->name);
    }
    splock_unlock(&tsk_lock);
}
