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
#include <kernel/sdk.h>

dynlib_t klib;

void kmod_symbol(CSTR name, void *ptr)
{
    dynsym_t *symbol = kalloc(sizeof(dynsym_t));
    symbol->name = strdup(name);
    symbol->address = (size_t)ptr;
    ll_append(&klib.intern_symbols, &symbol->node);
    hmp_put(&kproc.symbols, name, strlen(name), symbol);
}


void kmod_symbols(kdk_api_t *kapi)
{
    for (; kapi->address; ++kapi)
        kmod_symbol(kapi->name, kapi->address);
}



kdk_api_t base_kapi[] = {
    /* Kernel base*/
    KAPI(kalloc),
    KAPI(kfree),
    KAPI(kmap),
    KAPI(kunmap),
    KAPI(kprintf),
#ifdef KORA_KRN
    KAPI(__errno_location),
#endif
    KAPI(__assert_fail),
    KAPI(rand8),
    KAPI(rand16),
    KAPI(rand32),
    KAPI(rand64),
    KAPI(sztoa),

    KAPI(irq_enable),
    KAPI(irq_disable),
    KAPI(irq_ack),
    KAPI(irq_register),
    KAPI(irq_unregister),

    /* STD string.h */
    KAPI(memcpy),
    KAPI(memcmp),
    KAPI(memset),
    KAPI(memchr),
    KAPI(memmove),
    KAPI(strcat),
    KAPI(strchr),
    KAPI(strcmp),
    KAPI(strcpy),
    // KAPI(strcspn),
    // KAPI(stricmp),
    KAPI(strlen),
    KAPI(strnlen),
    KAPI(strncat),
    KAPI(strncmp),
    KAPI(strncpy),
    // KAPI(strpbrk),
    KAPI(strrchr),
    // KAPI(strspn),
    // KAPI(strstr),
    KAPI(strtok),
    KAPI(strdup),
    KAPI(strndup),
    // KAPI(strlwr),
    // KAPI(strupr),
    // KAPI(strnicmp),
    // KAPI(strrev),
    // KAPI(strset),
    // KAPI(strnset),
    KAPI(strtok_r),

    /* STD time.h */
    KAPI(mktime),
    KAPI(gmtime),
    KAPI(gmtime_r),
    KAPI(kclock),


    KAPI(memcpy32),
    KAPI(memset32),
    KAPI(itimer_create),

    /* VFS */
    KAPI(vfs_inode),
    KAPI(vfs_mkdev),
    KAPI(vfs_open),
    KAPI(vfs_close),
    KAPI(vfs_rmdev),
    KAPI(vfs_read),
    KAPI(vfs_write),

    KAPI(register_fs),
    KAPI(unregister_fs),

    KAPI(pipe_create),
    KAPI(pipe_read),
    KAPI(pipe_write),
    KAPI(blk_create),
    KAPI(blk_fetch),
    KAPI(blk_sync),
    KAPI(blk_release),
    KAPI(blk_destroy),
    KAPI(gfx_create),
    KAPI(gfx_clear),
    KAPI(bio_create),
    KAPI(bio_create2),
    KAPI(bio_destroy),
    KAPI(bio_access),
    KAPI(bio_sync),
    KAPI(bio_clean),

    /* NETWORK */
    KAPI(net_recv),
    // KAPI(net_device),

    /* SYSCALLS */
    // KAPI(sys_power),
    // KAPI(sys_stop),
    KAPI(sys_exit),
    // KAPI(sys_start),
    // KAPI(sys_fork),
    KAPI(sys_sleep),
    // KAPI(sys_wait),
    KAPI(sys_read),
    KAPI(sys_write),
    KAPI(sys_open),
    KAPI(sys_close),
    KAPI(sys_pipe),
    KAPI(sys_window),
    KAPI(sys_socket),
    // KAPI(sys_mmap),
    // KAPI(sys_munmap),
    // KAPI(sys_mprotect),
    KAPI(sys_ginfo),
    // KAPI(sys_sinfo),
    // KAPI(sys_log),
    // KAPI(sys_sysctl),
    // KAPI(sys_copy),

    /* ARCH DEPENDENT ROUTINES */
    KAPI(mmu_read),

    { NULL, 0, NULL },
};


KMODULE(dev);
void kmod_init()
{
    hmp_init(&kproc.symbols, 32);
    memset(&klib, 0, sizeof(klib));
    klib.base = 0;
    klib.length = 4 * _Mib_; // TODO arch specific!
    ll_append(&kproc.libraries, &klib.node);
    kmod_symbols(base_kapi);
}

