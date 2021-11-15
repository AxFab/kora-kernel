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
#include <kernel/stdc.h>
#include <kernel/memory.h>
#include <kernel/tasks.h>
#include <kernel/vfs.h>
#include <kora/bbtree.h>
#include <kora/llist.h>


struct task_params {
    void *func;
    size_t *params;
    size_t len;
    bool needmap;
};

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

static _Noreturn void task_usermode(task_params_t *info)
{
    assert(info != NULL);

    if (info->needmap) {
        int ret = dlib_map_all(__current->proc);
        if (ret != 0) {
            kprintf(-1, "Task.%d] Error while mapping executable\n",
                    __current->pid);
            task_fatal("Unable to map the program", -1);
            for (;;);
        }
        info->func = dlib_exec_entry(__current->proc);
    }

    // Prepare stack
    void *start = info->func;
    void *stack = mspace_map(__current->vm, 0, _Mib_, NULL, 0, VMA_STACK | VM_RW);
    stack = ADDR_OFF(stack, _Mib_ - sizeof(size_t));
    size_t *args = ADDR_PUSH(stack, ALIGN_UP(info->len, sizeof(void *)));
    memcpy(args, info->params, info->len);
    if (info->needmap) {
        args[1] = (size_t)&args[3];
        int i, argc = args[0];
        for (i = 0; i < argc; ++i)
            args[i + 3] = (size_t)&args[args[i + 3]];
    }
    ADDR_PUSH(stack, sizeof(void *));

    // Free info
    kfree(info->params);
    kfree(info);

    kprintf(-1, "Task.%d] Going usermode\n", __current->pid);
    mspace_display(NULL);
    cpu_usermode(start, stack);
}

static task_t *task_create(scheduler_t *sch, task_t *parent, const char *name, int flags)
{
    // Allocate
    task_t *task = kalloc(sizeof(task_t));
    task->stack = kmap(KSTACK_PAGES * PAGE_SIZE, NULL, 0, VM_RW | VM_RESOLVE | VMA_STACK);
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

    // Setup
    if (parent == NULL) {
        task->vfs = vfs_open_vfs(sch->vfs);
        task->fset = stream_create_set();
        task->vm = mspace_create();
        task->net = sch->net;
    } else {
        task->vfs = flags & KEEP_FS ? vfs_open_vfs(parent->vfs) : vfs_clone_vfs(parent->vfs);
        task->fset = flags & KEEP_FSET ? stream_open_set(parent->fset) : stream_create_set();
        task->vm = flags & KEEP_VM ? mspace_open(parent->vm) : mspace_create();
        task->net = parent->net;
    }

    kprintf(-1, "Alloc task %d with stack %p-%p\n", task->pid, task->stack, (char *)task->stack + KSTACK_PAGES * PAGE_SIZE);
    return task;
}


int task_spawn(const char *program, const char **args, fsnode_t **nodes)
{
    const char *name = strrchr(program, '/');
    if (name == NULL)
        name = program;
    task_t *task = task_create(&__scheduler, __current, name, 0);
    task->proc = dlib_process(task->vfs, task->vm);

    // Init stream
    int flags = 0;
    fsnode_t *null = vfs_search(task->vfs, "/dev/null", NULL, true);
    stream_put(task->fset, nodes[0] != NULL ? nodes[0] : null, flags | VM_RD);
    stream_put(task->fset, nodes[1] != NULL ? nodes[1] : null, flags | VM_WR);
    stream_put(task->fset, nodes[2] != NULL ? nodes[2] : null, flags | VM_WR);
    vfs_close_fsnode(null);

    // Load image
    int ret = dlib_openexec(task->proc, program);
    if (ret != 0) {
        kprintf(-1, "Task.%d] Unable to open executable image %s \n",
                __current->pid, program);
        return 0;
    }

    // Save args
    task_params_t *info = kalloc(sizeof(task_params_t));
    info->needmap = true;
    int i, count = 1;
    int len = ALIGN_UP(strlen(program) + 1, 4);
    for (i = 0; args[i]; ++i) {
        count++;
        len += ALIGN_UP(strlen(args[i]) + 1, 4);
    }

    len += sizeof(void *) * (count + 3);
    info->len = len;
    info->params = kalloc(len);
    info->params[0] = count;
    info->params[1] = 0; // &info->params[4];
    info->params[2] = 0; // environ

    len = 3 + count;
    info->params[3] = len; //(size_t)(&info->params[len]);
    strcpy((char *)&info->params[len], program);
    len += ALIGN_UP(strlen(program) + 1, 4) / 4;

    for (i = 0; args[i]; ++i) {
        info->params[i + 4] = len; //(size_t)(&info->params[len]);
        strcpy((char *)&info->params[len], args[i]);
        len += ALIGN_UP(strlen(args[i]) + 1, 4) / 4;
    }

    // Schedule
    cpu_prepare(&task->jmpbuf, task->stack, task_usermode, info);
    scheduler_add(&__scheduler, task);
    return task->pid;
}

int task_thread(const char *name, void *entry, void *params, size_t len, int flags)
{
    task_t *task = task_create(&__scheduler, __current, name, flags);
    task->proc = __current->proc;

    // Save args
    task_params_t *info = kalloc(sizeof(task_params_t));
    info->func = entry;
    info->needmap = false;
    info->params = memdup(params, len);
    info->len = len;

    // Schedule
    cpu_prepare(&task->jmpbuf, task->stack, task_usermode, info);
    scheduler_add(&__scheduler, task);
    return task->pid;
}


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


int task_start(const char *name, void *func, void *arg)
{
    scheduler_t *sch = &__scheduler;
    task_t *parent = __current;
    task_t *task = task_create(sch, parent, name, 0);

    cpu_prepare(&task->jmpbuf, task->stack, func, arg);
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
    kprintf(KL_ERR, "Task.%d] \033[31mFatal error %d: %s\033[0m\n", task->pid, signum, msg);
    stackdump(20);
    scheduler_switch(TS_ZOMBIE);
    // task_raise(sch, task, signum);
}

// _Noreturn void task_init()
// {
//     fsnode_t *node;
//     // Try to mount image
//     for (;;) {
//         node = vfs_mount(__current->vfs, "sdC", "iso", "/mnt/cdrom", "");
//         if (node != NULL)
//             break;
//         sleep_timer(250000);
//     }
//     vfs_close_fsnode(node);

//     // Change root
//     vfs_chdir(__current->vfs, "/mnt/cdrom", true);

//     // Re-mount devfs
//     vfs_mount(__current->vfs, NULL, "devfs", "/dev", "");

//     // Wait for /dev/fb0
//     for (;;) {
//         node = vfs_search(__current->vfs, "/dev/fb0", NULL, true);
//         if (node != NULL)
//             break;
//         sleep_timer(250000);
//     }
//     vfs_close_fsnode(node);


//     task_info_t *info = kalloc(sizeof(task_info_t));
//     info->program = strdup("krish");
//     info->args = kalloc(sizeof(void *) * 4);
//     info->args[0] = strdup("krish");
//     info->args[1] = strdup("-x");
//     info->args[2] = strdup("-s");
//     info->args[3] = NULL;

//     fsnode_t *stream = vfs_search(__current->vfs, "/dev/null", NULL, true);
//     info->streams[0] = stream;
//     info->streams[1] = vfs_open_fsnode(stream);
//     info->streams[2] = vfs_open_fsnode(stream);

//     task_init(info);
// }

// _Noreturn void task_init(task_info_t *info)
// {
//     // Load executable image
//     assert(info != NULL);
//     assert(__current->proc == NULL);
//     __current->proc = dlib_process(__current->vfs, __current->vm);
//     int ret = dlib_openexec(__current->proc, info->program);
//     if (ret != 0) {
//         kprintf(-1, "Task.%d] Unable to open executable image %s \n",
//             __current->pid, info->program);
//         for (;;);
//     }
//     ret = dlib_map_all(__current->proc);
//     if (ret != 0) {
//         kprintf(-1, "Task.%d] Error while mapping executable %s \n",
//             __current->pid, info->program);
//         for (;;);
//     }

//     // Create standard files
//     int flags = 0;
//     stream_put(__current->fset, info->streams[0], flags | VM_RD);
//     stream_put(__current->fset, info->streams[1], flags | VM_WR);
//     stream_put(__current->fset, info->streams[2], flags | VM_WR);
//     vfs_close_fsnode(info->streams[0]);
//     vfs_close_fsnode(info->streams[1]);
//     vfs_close_fsnode(info->streams[2]);

//     // Prepare stack
//     void *start = dlib_exec_entry(__current->proc);
//     void *stack = mspace_map(__current->vm, 0, _Mib_, NULL, 0, VMA_STACK | VM_RW);
//     stack = ADDR_OFF(stack, _Mib_ - sizeof(size_t));

//     // Copy command arguments
//     int i = 0, argc = 0;
//     while (info->args[argc] != NULL)
//         ++argc;
//     char **argv = ADDR_PUSH(stack, argc * sizeof(char *));
//     for (i = 0; i < argc; ++i) {
//         int lg = strlen(info->args[i]) + 1;
//         argv[i] = ADDR_PUSH(stack, ALIGN_UP(lg, 4));
//         strcpy(argv[i], info->args[i]);
//         kfree(info->args[i]);
//     }
//     kfree(info->args);
//     kfree(info->program);
//     kfree(info); // TODO -- Merge with __current->proc

//     size_t *args = ADDR_PUSH(stack, 4 * sizeof(char *));
//     args[1] = argc;
//     args[2] = (size_t)argv;
//     args[3] = 0;

//     kprintf(-1, "Task.%d] Ready to enter usermode for executable %s, start:%p, stack:%p \n",
//             __current->pid, info->program, start, stack);

//     cpu_usermode(start, stack);
// }
