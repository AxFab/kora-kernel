#include <kernel/sdk.h>

void kmod_symbol(CSTR name, void *ptr)
{
    dynsym_t *symbol = kalloc(sizeof(dynsym_t));
    symbol->name = strdup(name);
    symbol->address = (size_t)ptr;
    hmp_put(&kproc.symbols, name, strlen(name), symbol);
}


void kmod_symbols(kdk_api_t* kapi)
{
    for (;kapi->address;++kapi)
        kmod_symbol(kapi->name, kapi->address);
}



kdk_api_t base_kapi[] = {
    /* Kernel base*/
    KAPI(kalloc),
    KAPI(kfree),
    KAPI(kmap),
    KAPI(kunmap),
    KAPI(kprintf),
    KAPI(__errno_location),
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
    KAPI(strset),
    KAPI(strnset),
    KAPI(strtok_r),

    /* STD time.h */
    KAPI(mktime),
    KAPI(gmtime),
    KAPI(gmtime_r),
    KAPI(time64),

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
    KAPI(map_create),
    KAPI(map_fetch),
    KAPI(map_sync),
    KAPI(map_release),
    KAPI(map_destroy),
    KAPI(vds_create_empty),
    KAPI(vds_fill),
    KAPI(bio_create),
    KAPI(bio_create2),
    KAPI(bio_destroy),
    KAPI(bio_access),
    KAPI(bio_sync),
    KAPI(bio_clean),

    /* NETWORK */
    KAPI(net_recv),
    KAPI(net_device),

    /* SYSCALLS */
    // KAPI(sys_power),
    KAPI(sys_stop),
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


    { NULL, 0, NULL },
};


KMODULE(dev);
void kmod_init()
{
    hmp_init(&kproc.symbols, 32);
    kmod_symbols(base_kapi);
    kmod_register(&kmod_info_dev);
}
