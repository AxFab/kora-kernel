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
#include <kernel/tasks.h>
#include <kernel/memory.h>
#include <kernel/vfs.h>
#include <kernel/net.h>
#include <kernel/core.h>
#include <bits/atomic.h>


void module_init();
void cpu_setup(sys_info_t *);
void arch_init();
_Noreturn void kloader();

sys_info_t sysinfo;
#ifndef _VTAG_
#define _VTAG_  "v0.0"
#endif

/* Kernel entry point, must be reach by a single CPU */
void kstart()
{
    irq_reset(false);
    memset(&sysinfo, 0, sizeof(sys_info_t));
    kprintf(-1, "\033[97mKoraOS\033[0m - " __ARCH " - " _VTAG_ "\n");
    kprintf(-1, "Build the " __DATE__ ".\n");
    cpu_setup(&sysinfo);

    kprintf(-1, "\n\033[94m  Greetings on KoraOS...\033[0m\n");

    // Kernel initialization
    clock_init(sysinfo.uptime);
    module_init();
    vfs_init();
    scheduler_init();
    net_setup();
    arch_init();

    task_start("kloader", kloader, NULL);

    sysinfo.is_ready = 1;
    irq_zero();
}

/* Kernel secondary entry point, must be reach by additional CPUs */
void kready()
{
    irq_reset(false);
    cpu_setup(&sysinfo);
    kprintf(-1, "\033[32mCpu %d ready and waiting...\033[0m\n", cpu_no());

    while (sysinfo.is_ready == 0)
        atomic_break();
    irq_zero();
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

static int kloader_open_module(const char *name, inode_t *ino)
{
    dlproc_t *proc = __mmu.kspace->proc;
    // Try to load as kernel module
    dlib_t *mlib = dlib_create(name, ino);

    if (dlib_parse(proc, mlib) != 0) {
        dlib_clean(proc, mlib);
        return -1;
    }

    // mlib->depends; // TODO -- CHECK THEY ARE ALREADY LOADED !!


    ll_append(&proc->libs, &mlib->node);

    // Resolve symbols
    if (dlib_resolve(&proc->symbols_map, mlib) != 0) {
        // Missing symbols
        return -1;
    }
    mlib->resolved = true;

    // Map section of library
    if (dlib_rebase(__mmu.kspace, &proc->symbols_map, mlib) != 0) {
        // Missing memory space
        return -1;
    }
    mlib->resolved = false;



    // Look for kernel module references
    dlsym_t *symbol;
    int mods = 0;
    for ll_each(&mlib->intern_symbols, symbol, dlsym_t, node)
    {
        if (memcmp(symbol->name, "kmodule_", 8) == 0) {
            // TODO -- Check version
            kmodule_t *mod = (kmodule_t *)symbol->address;
            mod->setup();
            mods++;
            // splock_lock(&__kmodules_lock);
            // mod->dlib = dlib;
            // ll_enqueue(&__kmodules_list, &mod->node);
            // splock_unlock(&__kmodules_lock);
        }
    }
    if (mods == 0) {
        dlib_clean(proc, mlib);
        return -1;
    }

    kprintf(KL_MSG, "Loading driver \033[90m%s\033[0m\n", name);
    // Call start !
    dlib_clean(proc, mlib);
    return -1;
}


_Noreturn void kloader()
{
    char *buffer = kalloc(256);
    char *name = kalloc(256);

    // First task is incomplete!
    __current->fsa = __vfs_share->fsanchor;
    __current->net = NULL;
    __current->user = NULL;

    // Look for bootstrap drives (ramdisk-tar)
    for (int i = 0; ; ++i) {
        snprintf(buffer, 256, "/mnt/boot%d", i);
        inode_t *ino = vfs_search_ino(__current->fsa, buffer, __current->user, true);
        if (ino == NULL)
            break;

        // Read directory
        void *ctx = vfs_opendir(__current->fsa, buffer, __current->user);
        for (;;) {
            inode_t *ino = vfs_readdir(ctx, name, 256);
            if (ino == NULL)
                break;
            int len = strlen(name);

            if (len > 3 && memcmp(&name[len - 3], ".ko", 3) == 0)
                kloader_open_module(name, ino);

            vfs_close_inode(ino);
        }

        vfs_closedir(ctx);
    }

    // Look for main drive
    int ret = vfs_mount(__current->fsa, "sdc", "iso", "/mnt/cdrom", __current->user, "no-atime");
    if (ret < 0) {
        kprintf(-1, "Could not mount cdrom drive: Halt\n");
        for (;;)
            task_stop(-1);
    }
    vfs_chdir(__current->fsa, "/mnt/cdrom", __current->user, true);
    vfs_mount(__current->fsa, NULL, "devfs", "/dev", __current->user, "");

    // Start first user program
    const char *args[] = { NULL, };
    inode_t *pipe = vfs_pipe();
    inode_t *nodes[3] = { pipe, pipe, pipe };
    task_spawn("launcher", args, nodes);
    vfs_close_inode(pipe);


    for (;;)
        task_stop(-1);
}


// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

const char *ksymbol(void *ip, char *buf, int lg)
{
    return dlib_rev_ksymbol(__mmu.kspace->proc, (size_t)ip, buf, lg);
}

extern char ksymbols_start[];
extern char ksymbols_end[];

void module_init()
{
#ifdef KORA_KRN
    if (__mmu.kspace->proc == NULL)
        __mmu.kspace->proc = dlib_proc();
    kprintf(-1, "KSymbol [%p-%p]\n", &ksymbols_start, &ksymbols_end);
    kapi_t **sym = ((kapi_t**)ksymbols_start);
    sym = (void*)ALIGN_UP((size_t)sym, 4);
    for (;sym < (kapi_t**)ksymbols_end; ++sym) {
        kapi_t *s = *sym;
        dlib_add_symbol(__mmu.kspace->proc, NULL, s->name, (size_t)s->sym);
    }
    kprintf(-1, "KSymbols END\n");
#endif
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

sys_info_t *ksys()
{
    return &sysinfo;
}

cpu_info_t *kcpu()
{
    if (sysinfo.is_ready == 0)
        return NULL;
    int no = cpu_no();
    return &sysinfo.cpu_table[no];
}

size_t mmu_read(size_t);
EXPORT_SYMBOL(mmu_read, 0);
