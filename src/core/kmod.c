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
#include <kernel/vfs.h>
#include <kernel/task.h>
#include <kernel/dlib.h>
#include <string.h>
#include <errno.h>


#include <kernel/net.h>
#include <kernel/drv/pci.h>

typedef struct kmnt kmnt_t;
struct kmnt {
    inode_t *root;
    llnode_t node;
    CSTR name;
};

splock_t kmod_lock;
llhead_t kmod_mounted;
llhead_t kmod_standby;
llhead_t kmod_started;
extern proc_t kproc;

void kmod_register(kmod_t *mod)
{
    splock_lock(&kmod_lock);
    mod->dlib = NULL;
    ll_enqueue(&kmod_standby, &mod->node);
    splock_unlock(&kmod_lock);
}


void kmod_mount(inode_t *root)
{
    kmnt_t *mnt;
    mnt = kalloc(sizeof(kmnt_t));
    mnt->root = vfs_open(root);
    ll_enqueue(&kmod_mounted, &mnt->node);
}


// void kmod_symbol(CSTR name, void *ptr)
// {
//     dynsym_t *symbol = kalloc(sizeof(dynsym_t));
//     symbol->name = strdup(name);
//     symbol->address = (size_t)ptr;
//     hmp_put(&kproc.symbols, name, strlen(name), symbol);
// }



void kmod_loader()
{
    kmnt_t *mnt;
    kmod_t *mod;
    splock_lock(&kmod_lock);
    for (;;) {
        mnt = ll_dequeue(&kmod_mounted, kmnt_t, node);
        if (mnt == NULL)
            break;
        splock_unlock(&kmod_lock);
        kprintf(KLOG_MSG, "Browsing module \033[90m%s\033[0m for drivers by \033[90m#%d\033[0m\n", mnt->name, kCPU.running->pid);

        inode_t *ino;
        char filename[100];
        void *ctx = vfs_opendir(mnt->root, NULL);
        while ((ino = vfs_readdir(mnt->root, filename, ctx)) != NULL) {
            dynlib_t *dlib = kalloc(sizeof(dynlib_t));
            dlib->ino = ino;
            dlib->name = strdup(filename);
            int ret = elf_parse(dlib);
            if (ret != 0) {
                // TODO -- Clean
                kfree(dlib);
                // TODO -- Clean
                continue;
            }
            dlib_rebase(&kproc, kMMU.kspace, dlib);
            if (!dlib_resolve_symbols(&kproc, dlib)) {
                // kprintf(-1, "Module %s, missing symbols\n", filename);
                // TODO -- Clean
                dlib_unload(&kproc, kMMU.kspace, dlib);
                kfree(dlib);
                // TODO -- Clean
                continue;
            }
            kprintf(-1, "Open module %s (%s)\n", filename, sztoa(ino->length));
            dlib_map(dlib, kMMU.kspace);

            // Look for kernel module references
            dynsym_t *symbol;
            for ll_each(&dlib->intern_symbols, symbol, dynsym_t, node) {
                if (memcmp(symbol->name, "kmod_info_", 10) == 0) {
                    // kprintf(-1, "Found new module info: %s at %p\n",
                    //         symbol->name, symbol->address);
                    kmod_t *mod = (kmod_t *)symbol->address;
                    // kmod_register(mod);
                    splock_lock(&kmod_lock);
                    mod->dlib = dlib;
                    ll_enqueue(&kmod_standby, &mod->node);
                    splock_unlock(&kmod_lock);
                }
            }
        }
        vfs_closedir(mnt->root, ctx);

        // mspace_display(kMMU.kspace);
        splock_lock(&kmod_lock);
    }

    for (;;) {
        mod = ll_dequeue(&kmod_standby, kmod_t, node);
        splock_unlock(&kmod_lock);
        if (mod == NULL) {
            sys_sleep(MSEC_TO_KTIME(500));
            splock_lock(&kmod_lock);
            continue;
        }
        kprintf(KLOG_MSG, "Loading driver \033[90m%s\033[0m by \033[90m#%d\033[0m\n", mod->name, kCPU.running->pid);
        mod->setup();
        splock_lock(&kmod_lock);
        ll_enqueue(&kmod_started, &mod->node);
    }
}


void kmod_dump()
{
    kmod_t *mod;
    splock_lock(&kmod_lock);
    kprintf(KLOG_MSG, "Loaded driver:\n");
    for ll_each(&kmod_started, mod, kmod_t, node) {
        kprintf(KLOG_MSG, " %-20s  %d.%d.%d   %s\n", mod->name,
                VERS32_MJ(mod->version),
                VERS32_MN(mod->version),
                VERS32_PT(mod->version),
                mod->dlib ? "LOADED" : "EMBEDED");
    }
    splock_unlock(&kmod_lock);
}


const char *ksymbol(void *eip, char *buf, int lg)
{
    dynlib_t *lib;
    dynsym_t *symbol;
    dynsym_t *best = NULL;
    proc_t *proc = &kproc;
    task_t *task = kCPU.running;
    if (task != NULL && task->usmem != NULL) {
        if (task->usmem->lower_bound <= (size_t)eip && task->usmem->upper_bound > (size_t)eip)
            proc = task->proc;
    }
    for ll_each(&proc->libraries, lib, dynlib_t, node) {
        if ((size_t)eip < lib->base || (size_t)eip > lib->base + lib->length)
            continue;
        for ll_each(&lib->intern_symbols, symbol, dynsym_t, node) {
            if (symbol->address > (size_t)eip)
                continue;
            if (best == NULL || best->address < symbol->address)
                best = symbol;
        }
    }

    if (best == NULL)
        return "???";

    if ((size_t)eip == best->address)
        snprintf(buf, lg, "%s", best->name);
    else
        snprintf(buf, lg, "%s (+%d)", best->name, (size_t)eip - best->address);
    return buf;
}

