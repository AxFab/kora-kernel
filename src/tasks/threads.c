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
#include <kernel/stdc.h>
#include <kernel/memory.h>
#include <kernel/tasks.h>
#include <kernel/vfs.h>
#include <kora/bbtree.h>
#include <kora/llist.h>

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


static task_t *task_create(scheduler_t *sch, task_t *parent, const char *name)
{
    // Allocate
    task_t *task = kalloc(sizeof(task_t));
    task->stack = kmap(PAGE_SIZE, NULL, 0, VM_RW | VM_RESOLVE | VMA_STACK);
    task->status = TS_ZOMBIE;
    task->parent = parent;
    task->name = name != NULL ? strdup(name) : NULL;

    // Register
    splock_lock(&sch->lock);
    task_t *last = bbtree_last(&sch->task_tree, task_t, node);
    task->pid = last != NULL ? last->pid + 1 : 1;
    task->node.value_ = task->pid;
    bbtree_insert(&sch->task_tree, &task->node);
    splock_unlock(&sch->lock);
    return task;
}


int task_start(const char *name, void *func, void *arg)
{
    scheduler_t *sch = &__scheduler;
    task_t *parent = __current;
    task_t *task = task_create(sch, parent, name);

    task->vfs = vfs_open_vfs(parent != NULL ? parent->vfs : sch->vfs);
    task->fset = stream_create_set();
    task->vm = mspace_create();
    task->net = sch->net;

    cpu_setjmp(&task->jmpbuf, task->stack, func, arg);
    scheduler_add(sch, task);
    return task->pid;
}


int task_fork(const char *name, void *func, void *arg, int flags)
{
    scheduler_t *sch = &__scheduler;
    task_t *parent = __current;
    assert(parent != NULL);
    task_t *task = task_create(sch, parent, name);

    task->vfs = flags & KEEP_FS ? vfs_open_vfs(parent->vfs) : vfs_clone_vfs(parent->vfs);
    task->fset = flags & KEEP_FSET ? stream_open_set(parent->fset) : stream_create_set();
    task->vm = flags & KEEP_VM ? mspace_open(task->net) : mspace_create();
    task->net = parent->net;

    cpu_setjmp(&task->jmpbuf, task->stack, func, arg);
    scheduler_add(sch, task);
    return task->pid;
}

void task_stop(task_t *task, int code)
{
    task->ret_code = code;
    // TODO - For each thread -> Aborted !
}

void task_raise(scheduler_t *sch, task_t *task, unsigned signum)
{
    assert(signum > 0 && signum < 31);

    splock_lock(&task->lock);
    task->raised |= 1 << signum;
    if (task->status == TS_INTERRUPTIBLE) {
        task->status = TS_READY;
        scheduler_add(sch, task);
    }

    splock_unlock(&task->lock);
}

void task_fatal(const char *msg, unsigned signum)
{
    task_t *task = __current;
    kprintf(KL_ERR, "Fatal error on task %d: %s\n", task->pid, msg);
    scheduler_switch(TS_ZOMBIE);
    // task_raise(sch, task, signum);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

_Noreturn void task_init()
{
    char *execname = "krish";
    char *exec_args[] = {
        execname, "-x", "-s", NULL
    };

    // Load executable image
    assert(__current->proc == NULL);
    __current->proc = dlib_process(__current->vfs, __current->vm);
    int ret = dlib_openexec(__current->proc, execname);
    if (ret != 0) {
        kprintf(-1, "Task.%d] Unable to open executable image %s \n",
            __current->pid, execname);
        for (;;);
    }
    ret = dlib_map_all(__current->proc);
    if (ret != 0) {
        kprintf(-1, "Task.%d] Error while mapping executable %s \n",
            __current->pid, execname);
        for (;;);
    }

    // Create standard files
    // stream_put(__current->fset, nd_in, VM_RD);
    // stream_put(__current->fset, nd_out, VM_WR);
    // stream_put(__current->fset, nd_err, VM_WR);

    // Prepare stack
    void *start = dlib_exec_entry(__current->proc);
    void *stack = mspace_map(__current->vm, 0, _Mib_, NULL, 0, VMA_STACK | VM_RW);
    stack = ADDR_OFF(stack, _Mib_ - sizeof(size_t));

    // Copy command arguments
    int i = 0, argc = 0;
    while (exec_args[argc] != NULL)
        ++argc;
    char **argv = ADDR_PUSH(stack, argc * sizeof(char *));
    for (i = 0; i < argc; ++i) {
        int lg = strlen(exec_args[i]) + 1;
        argv[i] = ADDR_PUSH(stack, ALIGN_UP(lg, 4));
        strcpy(argv[i], exec_args[i]);
    }

    size_t *args = ADDR_PUSH(stack, 4 * sizeof(char *));
    args[1] = argc;
    args[2] = (size_t)argv;
    args[3] = 0;

    kprintf(-1, "Task.%d] Ready to enter usermode for executable %s, start:%p, stack:%p \n",
            __current->pid, execname, start, stack);

    for (;;)
        sleep_timer(250000);
    // cpu_usermode(task->kstack, start, stack);
}


