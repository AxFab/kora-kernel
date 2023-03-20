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



/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

_Noreturn void task_usermode(task_params_t *info)
{
    assert(info != NULL);
    if (info->start) {
        // TODO -- Find info->func (need remap?)
    }

    // Prepare stack
    void *start = info->func;
    void *stack = (void *)vmsp_map(__current->vmsp, 0, _Mib_, NULL, 0, VMA_STACK | VM_RW);
    stack = ADDR_OFF(stack, _Mib_ - sizeof(size_t));
    size_t *args = ADDR_PUSH(stack, ALIGN_UP(info->len, sizeof(void *)));
    memcpy(args, info->params, info->len);
    if (info->start) {
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
    // vmsp_display(NULL);
    cpu_usermode(start, stack);
}

static task_t *task_create(scheduler_t *sch, task_t *parent, const char *name, int flags)
{
    // Allocate
    task_t *task = kalloc(sizeof(task_t));
    task->stack = kmap(KSTACK_PAGES * PAGE_SIZE, NULL, 0, VM_RW | VM_RESOLVE | VMA_STACK);
    task->status = TS_ZOMBIE;
    task->parent = parent;
    task->name = name != NULL ? kstrdup(name) : NULL;

    // Register
    splock_lock(&sch->lock);
    task_t *last = bbtree_last(&sch->task_tree, task_t, node);
    task->pid = last != NULL ? last->pid + 1 : 1;
    task->node.value_ = task->pid;
    bbtree_insert(&sch->task_tree, &task->node);
    splock_unlock(&sch->lock);

    // Setup
    if (parent == NULL) {
        task->fsa = NULL; // vfs_open_vfs(sch->vfs);
        task->fset = stream_create_set();
        task->vmsp = vmsp_create();
        task->net = NULL; // sch->net;
    } else {
        task->fsa = flags & KEEP_FS ? vfs_open_vfs(parent->fsa) : vfs_clone_vfs(parent->fsa);
        task->fset = flags & KEEP_FSET ? stream_open_set(parent->fset) : stream_create_set();
        task->vmsp = flags & KEEP_VM ? vmsp_open(parent->vmsp) : vmsp_create();
        task->net = parent->net;
    }

    kprintf(-1, "Alloc task %d with stack %p\n", task->pid, (char *)task->stack + KSTACK_PAGES * PAGE_SIZE);
    return task;
}

task_t *task_search(size_t pid)
{
    scheduler_t *sch = &__scheduler;
    splock_lock(&sch->lock);
    task_t *task;
    if (pid == 0)
        task = bbtree_first(&sch->task_tree, task_t, node);
    else
        task = bbtree_search_eq(&sch->task_tree, pid, task_t, node);
    splock_unlock(&sch->lock);
    return task;
}

task_t *task_next(size_t pid)
{
    scheduler_t *sch = &__scheduler;
    splock_lock(&sch->lock);
    task_t *task = bbtree_search_eq(&sch->task_tree, pid, task_t, node);
    if (task != NULL)
        task = bbtree_next(&task->node, task_t, node);
    splock_unlock(&sch->lock);
    return task;
}


size_t task_spawn(const char *program, const char **args, inode_t **nodes)
{
    const char *name = strrchr(program, '/');
    if (name == NULL)
        name = program;
    task_t *task = task_create(&__scheduler, __current, name, 0);
    // task->proc = dlib_process(task->vfs, task->vm);

    // Init stream
    int flags = 0;
    if (nodes == NULL || nodes[0] == NULL || nodes[1] == NULL || nodes[2] == NULL)
        return -1;
    resx_put(task->fset, RESX_FILE, file_from_inode(nodes[0], flags | VM_RD), (void *)file_close);
    resx_put(task->fset, RESX_FILE, file_from_inode(nodes[1], flags | VM_WR), (void *)file_close);
    resx_put(task->fset, RESX_FILE, file_from_inode(nodes[2], flags | VM_WR), (void *)file_close);

    // Load image
    int ret = dlib_open(task->vmsp, task->fsa, task->user, program);
    if (ret != 0) {
        kprintf(-1, "Task.%d] Unable to open executable image %s \n", __current->pid, program);
        // TODO -- Task destroy !!
        return 0;
    }

    // Save args
    task_params_t *info = kalloc(sizeof(task_params_t));
    info->start = true;
    int i, count = 1;
    int len = ALIGN_UP(strlen(program) + 1, 4);
    if (args) {
        for (i = 0; args[i]; ++i) {
            count++;
            len += ALIGN_UP(strlen(args[i]) + 1, 4);
        }
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

    if (args) {
        for (i = 0; args[i]; ++i) {
            info->params[i + 4] = len; //(size_t)(&info->params[len]);
            strcpy((char *)&info->params[len], args[i]);
            len += ALIGN_UP(strlen(args[i]) + 1, 4) / 4;
        }
    }

    // Schedule
    cpu_prepare(&task->jmpbuf, task->stack, task_usermode, info);
    scheduler_add(&__scheduler, task);
    return task->pid;
}

size_t task_thread(const char *name, void *entry, void *params, size_t len, int flags)
{
    task_t *task = task_create(&__scheduler, __current, name, flags);

    // Save args
    task_params_t *info = kalloc(sizeof(task_params_t));
    info->func = entry;
    info->start = false;
    info->params = kalloc(sizeof(len));
    memcpy(info->params, params, len);
    info->len = len;

    // Schedule
    cpu_prepare(&task->jmpbuf, task->stack, task_usermode, info);
    scheduler_add(&__scheduler, task);
    return task->pid;
}


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

size_t task_start(const char *name, void *func, void *arg)
{
    scheduler_t *sch = &__scheduler;
    task_t *parent = __current;
    task_t *task = task_create(sch, parent, name, 0);

    cpu_prepare(&task->jmpbuf, task->stack, func, arg);
    scheduler_add(sch, task);
    return task->pid;
}
EXPORT_SYMBOL(task_start, 0);

void task_stop(int code)
{
    // splock_lock(&__scheduler.lock);
    task_t *task = __current;
    task->ret_code = code;
    //if (task->status >= TS_RUNNING)
    //    task->status = TS_ABORTED;
    //else {
    //    if (task->status == TS_READY)
    //        ll_remove(&__scheduler.sch_queue, &task->sch_node);
    //    if (task->status != TS_ZOMBIE) {
    //        task->status = TS_ZOMBIE;
    //        ll_append(&__scheduler.sch_queue, &task->sch_node);
    //        while (task->advent_list.count_ > 0) {
    //            advent_t *adv = ll_dequeue(&task->advent_list, advent_t, node);
    //            adv->dtor(&__clock, adv);
    //        }
    //    }
    //}

    //splock_unlock(&__scheduler.lock);
    scheduler_switch(TS_ZOMBIE);
}

void task_raise(task_t *task, unsigned signum)
{
    scheduler_t *sch = &__scheduler;
    assert(signum > 0 && signum < 31);
    if (task == NULL)
        return;

    splock_lock(&task->lock);
    task->raised |= 1 << signum;
    if (task->status == TS_INTERRUPTIBLE) {
        task->status = TS_READY;
        scheduler_add(sch, task);
    }

    splock_unlock(&task->lock);
}

