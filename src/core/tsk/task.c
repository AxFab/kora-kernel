#include <kernel/core.h>
#include <kernel/task.h>
#include <kernel/vfs.h>
#include <kernel/memory.h>
#include <kernel/sys/inode.h>
#include <kora/atomic.h>
#include <bits/signum.h>
#include <errno.h>
#include <assert.h>


void task_core(task_t *task)
{
    kprintf(0, "Task Core %d =========================\n", task->pid);
    mspace_display(0, task->usmem);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

/**
 * Indicate that current task is enter into Kernel code.
 */
void task_enter_sys(regs_t *regs, bool kernel)
{
    task_t *task = kCPU.running;
    if (task) {
        splock_lock(&task->lock);
        task->regs/*[++task->rp]*/ = regs;
        splock_unlock(&task->lock);
        if (kernel) {
            task->sys_elapsed += cpu_elapsed(&task->last);
            kCPU.sys_elapsed += cpu_elapsed(&kCPU.last);
        } else {
            task->user_elapsed += cpu_elapsed(&task->last);
            kCPU.user_elapsed += cpu_elapsed(&kCPU.last);
        }
    } else {
        kCPU.sys_elapsed += cpu_elapsed(&kCPU.last);
    }
}

void task_leave_sys()
{
    task_t *task = kCPU.running;
    kCPU.sys_elapsed += cpu_elapsed(&kCPU.last);
    if (task) {
        splock_lock(&task->lock);
        // --task->rp;
        splock_unlock(&task->lock);
        task->sys_elapsed += cpu_elapsed(&task->last);
    }
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


void task_start(task_t *task, size_t entry, long args)
{
    if (task->state != TS_ZOMBIE) {
        kprintf(-1, "[TASK] Try to grab a used task.");
        return;
    }

    if (task->usmem && !task->ustack) {
        // TODO -- Map at the end
        task->ustack_len = 1 * _Mib_;
        task->ustack = mspace_map(task->usmem, 0, task->ustack_len, NULL, 0,
                                  0, VMA_FG_STACK);
    }

    cpu_setup_task(task, entry, args);
    scheduler_add(task);
}

int task_stop(task_t *task, int code)
{
    splock_lock(&task->lock);
    if (task == kCPU.running || task->state == TS_READY) {
        scheduler_rm(task);
    } else if (task->state == TS_RUNNING) {
        task->state = TS_ABORTED;
        task->exit_code = code;
        splock_unlock(&task->lock);
        return 0;
    } else if (task->state == TS_ZOMBIE) {
        splock_unlock(&task->lock);
        return -1;
    }

    task->state = TS_ZOMBIE;
    task->exit_code = code;

    task_t *parent = task->parent;
    // TODO - All children become orphans
    splock_unlock(&task->lock);

    if (parent) {
        task_kill(parent, SIGCHLD);
    }

    // event_trigger(EV_TASK_DIE, task);

    // Usage counter !?
    return 0;
}


int task_kill(task_t *task, int signum)
{
    if (signum < 0 || signum > 31) {
        errno = EINVAL;
        return -1;
    }

    splock_lock(&task->lock);
    task->recieved_signal |= 1 << signum;
    if (task->state == TS_INTERRUPTIBLE) {
        task->state = TS_READY;
        scheduler_add(task);
    }
    splock_unlock(&task->lock);
    return 0;
}


_Noreturn int task_pause(int state)
{
    assert (state > TS_ZOMBIE && state < TS_READY);
    task_t *task = kCPU.running;
    splock_lock(&task->lock);
    if (task->state == TS_ABORTED) {
        task_stop(task, task->exit_code);
        scheduler_next();
    }

    // TODO
    task->state = state;
    scheduler_rm(task);
    splock_unlock(&task->lock);
    scheduler_next();
}

int task_resume(task_t *task)
{
    splock_lock(&task->lock);
    if (task->state <= TS_ZOMBIE || task->state >= TS_READY) {
        return -1;
    }

    task->state = TS_READY;
    scheduler_add(task);
    splock_unlock(&task->lock);
    return 0;
}

/* Handle signal of the current task */
void task_signals()
{
    task_t *task = kCPU.running;
    if (!cpu_task_return_uspace(task) || task->sig_regs != NULL) {
        return;
    }
    // handle signals
    while (task->recieved_signal != 0) {
        int sn = 0;
        while (((task->recieved_signal >> sn) & 1) == 0) {
            sn++;
        }
        task->recieved_signal &= ~(1 << sn);
        sig_handler_t *sig = &task->shandler[sn];

        if (sn == SIGKILL) {
            kprintf(0, "[TSK ] Task %d recived SIGKILL\n", task->pid);
            task_stop(task, -2);
            scheduler_next();
        } else if (sig->type == SIG_IGN) {
            continue;
        } else if (sig->type == SIG_DFL) {
            if (sn == SIGHUP || sn == SIGINT || sn == SIGQUIT || sn == SIGSEGV) {
                kprintf(0, "[TSK ] Task %d recived killed %d\n", task->pid, sn);
                task_core(task);
                task_stop(task, -2);
                scheduler_next();
                break;
            }
        } else {
            // TODO -- Ensure sig->type is placed on user memory space.
            kprintf(0, "[TSK ] Signal custum routine PID:%d\n", kCPU.running->pid);
            cpu_setup_signal(task, (size_t)sig->type, sn);
        }
    }
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

// TODO -- Put that on a config header !
#define MIN_PID 1
#define MAX_PID 99999
#define TASK_DEFAULT_PRIO 20

unsigned AUTO_PID = MIN_PID;
bool pid_init = false;
bbtree_t pid_tree;

static task_t *task_allocat()
{
    task_t *task = (task_t *)kalloc(sizeof(task_t));
    task->kstack = (size_t *)kmap(PAGE_SIZE, NULL, 0, VMA_FG_STACK | VMA_RESOLVE);
    task->kstack_len = PAGE_SIZE;

    task->state = TS_ZOMBIE;
    task->prio = TASK_DEFAULT_PRIO;

    task->last = cpu_elapsed(NULL);

    return task;
}


static pid_t task_new_pid()
{
    pid_t pid;
    // TODO -- Lock
    do {
        pid = AUTO_PID++;
        if (AUTO_PID > MAX_PID) {
            AUTO_PID = MIN_PID;
        }
    } while (task_search(pid) != NULL);
    return pid;
}

task_t *task_create(user_t *user, inode_t *root, int flags)
{
    task_t *task = task_allocat();
    task->pid = task_new_pid();
    task->user = user;

    task->root = root ? vfs_open(root) : NULL;
    task->pwd = root ? vfs_open(root) : NULL;
    task->resx = resx_create();

    if (flags & TSK_USER_SPACE)
        task->usmem = mspace_create();
    task->bnode.value_ = task->pid;
    if (kCPU.running != NULL) {
        task->parent = kCPU.running;
    }
    bbtree_insert(&pid_tree, &task->bnode);
    return task;
}


task_t *task_clone(task_t *model, int clone, int flags)
{
    task_t *task = task_allocat();
    task->pid = task_new_pid();

    // Keep file descriptor
    if (clone & CLONE_FILES) {
        task->resx = resx_rcu(model->resx, 1);
    } else {
        task->resx = resx_create();
    }

    // Keep FS informations
    if (clone & CLONE_FS) {
        task->root = vfs_open(model->root);
        task->pwd = vfs_open(model->pwd);
    } else {
        task->root = NULL; // TODO - default root !?
        task->pwd = NULL;
    }

    if (clone & CLONE_PARENT) {
        task->parent = model->parent;
    } else {
        if (kCPU.running != NULL) {
            task->parent = kCPU.running;
        }
    }


    if (clone & CLONE_TLS) {
    } else {
    }


    if (clone & CLONE_SIGNAL) {
    } else {
    }


    if (clone & CLONE_SCALL) {
    } else {
    }


    if (clone & CLONE_THREAD) {
        task->usmem = mspace_rcu(model->usmem, 1);
    } else {
        if (clone & CLONE_MSPACE) {
        task->usmem = mspace_clone(model->usmem);
        } else {
            task->usmem = flags & TSK_USER_SPACE ? mspace_create() : NULL;
        }
    }

    if (clone & CLONE_USER) {
        task->user = model->user;
    } else {
        task->user = NULL;
    }

    task->bnode.value_ = task->pid;
    bbtree_insert(&pid_tree, &task->bnode);
    return task;
}

void task_destroy(task_t *task)
{
    splock_lock(&task->lock);
    vfs_close(task->root);
    vfs_close(task->pwd);

    // TODO Close all open FD
    if (task->usmem)
        mspace_rcu(task->usmem, 0);
    if (task->resx)
        resx_rcu(task->resx, 0);

    bbtree_remove(&pid_tree, task->pid);
    irq_disable();
    splock_unlock(&task->lock);
    kfree(task);
    irq_enable();
}

task_t *task_search(pid_t pid)
{
    if (!pid_init) {
        bbtree_init(&pid_tree);
        pid_init = true;
    }

    return bbtree_search_eq(&pid_tree, pid, task_t, bnode);
}
