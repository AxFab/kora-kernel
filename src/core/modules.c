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
#include <kernel/mods.h>
#include <kernel/irq.h>
#include <kernel/vfs.h>
#include <kernel/net.h>
#include <kernel/memory.h>
#include <kernel/dlib.h>
#include <kernel/tasks.h>

proc_t *__kernel_proc;
dynlib_t __kernel_lib;
// splock_t __kmodules_lock;
// llist_t __kmodules_list;


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


const char *ksymbol(void *ip, char *buf, int lg)
{
    dynlib_t *lib;
    dynsym_t *symbol;
    dynsym_t *best = NULL;
    proc_t *proc = __kernel_proc;
    task_t *task = __current;
    if (task != NULL && task->vm != NULL) {
        if (task->vm->lower_bound <= (size_t)ip && task->vm->upper_bound > (size_t)ip)
            proc = task->proc;
    }
    for ll_each(&proc->libraries, lib, dynlib_t, node) {
        if ((size_t)ip < lib->base || (size_t)ip > lib->base + lib->length)
            continue;
        for ll_each(&lib->intern_symbols, symbol, dynsym_t, node) {
            if (symbol->address > (size_t)ip)
                continue;
            if (best == NULL || best->address < symbol->address)
                best = symbol;
        }
    }

    if (best == NULL)
        return "???";

    if ((size_t)ip == best->address)
        snprintf(buf, lg, "%s", best->name);
    else
        snprintf(buf, lg, "%s (+%d)", best->name, (size_t)ip - best->address);
    return buf;
}



void module_symbol(const char *name, void *ptr)
{
    // splock_lock(&__kernel_proc->lock);
    dynsym_t *symbol = kalloc(sizeof(dynsym_t));
    symbol->name = strdup(name);
    symbol->address = (size_t)ptr;
    ll_append(&__kernel_lib.intern_symbols, &symbol->node);
    hmp_put(&__kernel_proc->symbols, name, strlen(name), symbol);
    // splock_unlock(&__kernel_proc->lock);
}


int *__errno_location();

void module_init(vfs_t *vfs, mspace_t *vm)
{
    // splock_init(&__kmodules_lock);
    // ll_init(&__kmodules_list);

    __kernel_proc = dlib_process(vfs, vm);
    memset(&__kernel_lib, 0, sizeof(__kernel_lib));
    ll_append(&__kernel_proc->libraries, &__kernel_lib.node);

    // Kernel routines
    kernel_export_symbol(__errno_location);
    kernel_export_symbol(kalloc);
    kernel_export_symbol(kfree);
    kernel_export_symbol(kmap);
    kernel_export_symbol(kunmap);
    kernel_export_symbol(kprintf);
    kernel_export_symbol(__assert_fail);
    kernel_export_symbol(rand8);
    kernel_export_symbol(rand16);
    kernel_export_symbol(rand32);
    kernel_export_symbol(xtime_read);
    kernel_export_symbol(sztoa_r);
    kernel_export_symbol(mmu_read); // !?

    // String.h
    kernel_export_symbol(memchr);
    kernel_export_symbol(memcmp);
    kernel_export_symbol(memcpy);
    kernel_export_symbol(memmove);
    kernel_export_symbol(memset);

    kernel_export_symbol(strcat);
    kernel_export_symbol(strchr);
    kernel_export_symbol(strcmp);
    kernel_export_symbol(strcpy);
    kernel_export_symbol(strlen);
    kernel_export_symbol(strncat);
    kernel_export_symbol(strncmp);
    kernel_export_symbol(strncpy);
    kernel_export_symbol(strnlen);
    kernel_export_symbol(strrchr);
    // kernel_export_symbol(strstr);
    kernel_export_symbol(strtok_r);
    kernel_export_symbol(strdup);

    kernel_export_symbol(memcpy32);
    kernel_export_symbol(memset32);

    // Formating
    kernel_export_symbol(atoi);
    kernel_export_symbol(atol);
    kernel_export_symbol(strtoul);
    kernel_export_symbol(strtol);
    // kernel_export_symbol(sscanf);
    // kernel_export_symbol(vsscanf);
    kernel_export_symbol(snprintf);
    kernel_export_symbol(vsnprintf);

    // Time
    kernel_export_symbol(mktime);
    kernel_export_symbol(gmtime_r);
    // kernel_export_symbol(asctime_r);

    // VFS
    kernel_export_symbol(vfs_inode);
    kernel_export_symbol(vfs_open_inode);
    kernel_export_symbol(vfs_close_inode);
    kernel_export_symbol(vfs_mkdev);
    kernel_export_symbol(vfs_rmdev);
    kernel_export_symbol(vfs_addfs);
    kernel_export_symbol(vfs_rmfs);
    kernel_export_symbol(vfs_read);
    kernel_export_symbol(vfs_write);
    kernel_export_symbol(vfs_fcntl);

    // IRQ
    kernel_export_symbol(irq_enable);
    kernel_export_symbol(irq_disable);
    kernel_export_symbol(irq_register);
    kernel_export_symbol(irq_unregister);
    kernel_export_symbol(irq_ack);

    // Memory
    // Tasks
    kernel_export_symbol(itimer_create);
    kernel_export_symbol(sleep_timer);

    // NET
    // kernel_export_symbol(net_alloc);
    // kernel_export_symbol(net_skb_recv);

}


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


int module_load(fsnode_t *file)
{
    dynlib_t *dlib = kalloc(sizeof(dynlib_t));
    dlib->ino = vfs_open_inode(file->ino);
    dlib->name = strdup(file->name);
    int ret = elf_parse(dlib);
    if (ret != 0) {
        kfree(dlib);
        // TODO -- Clean dynlib!
        return -1;
    }

    dlib_rebase(__kernel_proc, __kernel_proc->mspace, dlib);
    if (!dlib_resolve_symbols(__kernel_proc, dlib)) {
        // kprintf(-1, "Module %s, missing symbols\n", filename);
        // TODO -- Clean
        dlib_unload(__kernel_proc, __kernel_proc->mspace, dlib);
        kfree(dlib);
        // TODO -- Clean dynlib!
        kprintf(KL_MSG, "Driver load fails \033[90m%s\033[0m, unresolved symbols, by \033[90m#%d\033[0m\n", dlib->name, __current->pid);
        return -1;
    }
    // kprintf(-1, "Open module %s (%s)\n", filename, sztoa(file->ino->length));
    dlib_map(dlib, __kernel_proc->mspace);

    // Look for kernel module references
    dynsym_t *symbol;
    int mods = 0;
    for ll_each(&dlib->intern_symbols, symbol, dynsym_t, node) {
        if (memcmp(symbol->name, "kmodule_", 8) == 0) {
            // TODO -- Check version
            kmodule_t *mod = (kmodule_t *)symbol->address;
            kprintf(KL_MSG, "Loading driver \033[90m%s\033[0m by \033[90m#%d\033[0m\n", mod->name, __current->pid);
            mod->setup();
            mods++;
            // splock_lock(&__kmodules_lock);
            // mod->dlib = dlib;
            // ll_enqueue(&__kmodules_list, &mod->node);
            // splock_unlock(&__kmodules_lock);
        }
    }

    if (mods == 0)
        kprintf(KL_MSG, "Driver load fails \033[90m%s\033[0m, no module found, by \033[90m#%d\033[0m\n", dlib->name, __current->pid);
    return mods != 0 ? 0 : -1;
}



/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

typedef struct mtask mtask_t;
struct mtask {
    void *ptr;
    int type;
    llnode_t node;
};

llhead_t mtask_queue = INIT_LLHEAD;
splock_t mtask_lock = INIT_SPLOCK;
atomic_int mtask_step1 = 0;
atomic_int mtask_step2 = 0;

void module_new_task(int type, void *ptr)
{
    splock_lock(&mtask_lock);
    mtask_t *mt = kalloc(sizeof(mtask_t));
    mt->ptr = ptr;
    mt->type = type;
    ll_enqueue(&mtask_queue, &mt->node);
    splock_unlock(&mtask_lock);
}

void module_do_dir(fsnode_t *directory)
{
    kprintf(-1, "Kernel task %d reading directory (cpu:%d) \n", __current->pid, cpu_no());
    void *ctx = vfs_opendir(directory, NULL);
    for (;;) {
        fsnode_t *file = vfs_readdir(directory, ctx);
        if (file == NULL)
            break;
        // kprintf(-1, "New file: '%s'\n", file->name);
        module_new_task(2, file);
    }
    vfs_closedir(directory, ctx);
    vfs_close_fsnode(directory);
}

void module_do_file(fsnode_t *file)
{
    kprintf(-1, "Kernel task %d loading module (cpu:%d) \n", __current->pid, cpu_no());
    module_load(file);
    vfs_close_fsnode(file);
}

void module_do_proc(char *cmd)
{
    kprintf(-1, "Kernel task %d starting process (cpu:%d) \n", __current->pid, cpu_no());
    fsnode_t *root = vfs_mount(__current->vfs, "sdC", "iso", "/mnt/cdrom", "");
    vfs_close_fsnode(root);
    vfs_chdir(__current->vfs, "/mnt/cdrom", true);
    vfs_mount(__current->vfs, NULL, "devfs", "/dev", "");

    // Start first user program
    const char *args[] = { NULL, };
    fsnode_t *nodes[3] = { NULL };
    task_spawn(cmd, args, nodes);
    kfree(cmd);
}

int module_predefined_tasks()
{
    int val;
    // Check presence of initrd module
    val = atomic_xchg(&mtask_step1, 1);
    if (val == 0) {
        fsnode_t *directory = vfs_search(__current->vfs, "/mnt/boot0", NULL, true);
        if (directory == NULL)
            kprintf(KL_MSG, "Unable to find kernel modules");
        else
            module_new_task(1, directory);
        return 0;
    }

    val = atomic_xchg(&mtask_step2, 1);
    if (val == 0) {
        module_new_task(3, strdup("krish"));
        return 0;
    }

    return -1;
}

_Noreturn void module_loader()
{
    for (;;) {
        splock_lock(&mtask_lock);
        mtask_t *mt = ll_dequeue(&mtask_queue, mtask_t, node);
        splock_unlock(&mtask_lock);
        if (mt == NULL) {
            if (module_predefined_tasks() == 0)
                continue;
            kprintf(-1, "Sleeping kernel task %d (cpu:%d) \n", __current->pid, cpu_no());
            sleep_timer(500);
            continue;
        } else if (mt->type == 1)
            module_do_dir(mt->ptr);
        else if (mt->type == 2)
            module_do_file(mt->ptr);
        else if (mt->type == 3)
            module_do_proc(mt->ptr);
        else
            kprintf(-1, "Unknown system task [%d]\n", mt->type);
        kfree(mt);
    }
}
