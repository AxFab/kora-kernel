/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2018  <Fabien Bavent>
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
#include <string.h>
#include <errno.h>
#include "dlib.h"


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
            dlib->io = bio_create(ino, VMA_FILE_RO, PAGE_SIZE, 0);
            int ret = elf_parse(dlib);
            if (ret != 0) {
                // TODO -- Clean
                bio_destroy(dlib->io);
                kfree(dlib);
                // TODO -- Clean
                continue;
            }
            dlib_rebase(&kproc, kMMU.kspace, dlib);
            if (!dlib_resolve_symbols(&kproc, dlib)) {
                // kprintf(-1, "Module %s, missing symbols\n", filename);
                // TODO -- Clean
                dlib_unload(&kproc, kMMU.kspace, dlib);
                bio_destroy(dlib->io);
                kfree(dlib);
                // TODO -- Clean
                continue;
            }
            kprintf(-1, "Open module %s (%s)\n", filename, sztoa(ino->length));

            dynsec_t *sec;
            for ll_each(&dlib->sections, sec, dynsec_t, node) {

                // kprintf(-1, "Section %4x - %4x - %4x - %4x - %4x , %o\n", sec->lower, sec->upper,
                //         sec->start, sec->end, sec->offset, sec->rights);

                // Copy sections
                void *sbase = (void *)(dlib->base + sec->lower + sec->offset);
                size_t slen = sec->upper - sec->lower;
                memset(sbase, 0, slen);
                int i, n = (sec->upper - sec->lower) / PAGE_SIZE;
                for (i = 0; i < n; ++i) {
                    uint8_t *page = bio_access(dlib->io, i + sec->lower / PAGE_SIZE);
                    size_t start = i == 0 ? sec->start : 0;
                    void *src = ADDR_OFF(page, start);
                    void *dst = (void *)(dlib->base + sec->lower + sec->offset + start + i * PAGE_SIZE);
                    int lg = (i + 1 == n ? (sec->end & (PAGE_SIZE - 1)) : PAGE_SIZE) - start;
                    memcpy(dst, src, lg);
                }

                // kprintf(-1, "Section : %p - %x\n", sbase, slen);
                // kdump(sbase, slen);
            }

            // Relocations
            dynrel_t *reloc;
            // dynsym_t *symbol;
            for ll_each(&dlib->relocations, reloc, dynrel_t, node) {

                // kprintf(-1, "R: %06x  %x  %p  %s \n", reloc->address, reloc->type, reloc->symbol == NULL ? NULL : (void*)reloc->symbol->address, reloc->symbol == NULL ? "-" : reloc->symbol->name);
                switch (reloc->type) {
                case 6:
                case 7:
                    *((size_t *)(dlib->base + reloc->address)) = reloc->symbol->address;
                    break;
                case 1:
                    *((size_t *)(dlib->base + reloc->address)) += reloc->symbol->address;
                    break;
                case 8:
                    *((size_t *)(dlib->base + reloc->address)) += dlib->base;
                    break;
                }

                // kprintf(-1, " -> %s at %p\n", symbol->name, symbol-> );
                // hmp_put(&proc->symbols, symbol->name, strlen(symbol->name), symbol);
                // TODO - Do not replace first occurence of a symbol.
            }

            // Change map access rights
            for ll_each(&dlib->sections, sec, dynsec_t, node) {
                size_t sbase = dlib->base + sec->lower + sec->offset;
                size_t slen = sec->upper - sec->lower;
                mspace_protect(kMMU.kspace, sbase, slen, sec->rights & 7);
            }


            // Look for kernel module references
            dynsym_t *symbol;
            for ll_each(&dlib->intern_symbols, symbol, dynsym_t, node) {
                if (memcmp(symbol->name, "kmod_info_", 10) == 0) {
                    // kprintf(-1, "Found new module info: %s at %p\n",
                    //         symbol->name, symbol->address);
                    kmod_t *mod = (kmod_t *)symbol->address;
                    kmod_register(mod);
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
            async_wait(NULL, NULL, 50000);
            splock_lock(&kmod_lock);
            continue;
        }
        kprintf(KLOG_MSG, "Loading driver \033[90m%s\033[0m by \033[90m#%d\033[0m\n", mod->name, kCPU.running->pid);
        mod->setup();
        splock_lock(&kmod_lock);
        ll_enqueue(&kmod_started, &mod->node);
    }
}
