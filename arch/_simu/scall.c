#include <kernel/syscalls.h>
#include <kora/syscalls.h>
#include <kernel/core.h>
#include <string.h>

long txt_access(const char *line)
{
    int arg1, arg3, arg4, exret;
    char arg2[256];
    sscanf(line, "access (%d, %s %d) = %d", &arg1, arg2, &arg3, &exret);
    strchr(&arg2[1], '\"')[0] = '\0';
    int ret = irq_syscall(SYS_ACCESS, arg1, &arg2[1], arg3, 0, 0);
    return ret;
}


long txt_open(const char *line)
{
    int arg1, arg3, arg4, exret;
    char arg2[256];
    sscanf(line, "open (%d, %s %d, %d) = %d", &arg1, arg2, &arg3, &arg4, &exret);
    strchr(&arg2[1], '\"')[0] = '\0';
    int ret = irq_syscall(SYS_OPEN, arg1, &arg2[1], arg3, arg4, 0);
    return ret;
}

long txt_close(const char *line)
{
    return irq_syscall(SYS_CLOSE, 0, 0, 0, 0, 0);
}

long txt_read(const char *line)
{
    return irq_syscall(SYS_READ, 0, 0, 0, 0, 0);
}

long txt_write(const char *line)
{
    return irq_syscall(SYS_WRITE, 0, 0, 0, 0, 0);
}

long txt_mmap(const char *line)
{
    return irq_syscall(SYS_MMAP, 0, 0, 0, 0, 0);
}

long txt_munmap(const char *line)
{
    return irq_syscall(SYS_MUNMAP, 0, 0, 0, 0, 0);
}

long txt_exit(const char *line)
{
    return irq_syscall(SYS_EXIT, 0, 0, 0, 0, 0);
}

long txt_fcntl(const char *line)
{
    return irq_syscall(SYS_FCNTL, 0, 0, 0, 0, 0);
}

long txt_window(const char *line)
{
    return irq_syscall(SYS_WINDOW, 0, 0, 0, 0, 0);
}

long txt_futex_wait(const char *line)
{
    return irq_syscall(SYS_FUTEX_WAIT, 0, 0, 0, 0, 0);
}

long txt_futex_requeue(const char *line)
{
    return irq_syscall(SYS_FUTEX_REQUEUE, 0, 0, 0, 0, 0);
}

long txt_ginfo(const char *line)
{
    return irq_syscall(SYS_GINFO, 0, 0, 0, 0, 0);
}

long txt_sinfo(const char *line)
{
    return irq_syscall(SYS_SINFO, 0, 0, 0, 0, 0);
}

long txt_sfork(const char *line)
{
    return irq_syscall(SYS_SFORK, 0, 0, 0, 0, 0);
}

long txt_pfork(const char *line)
{
    return irq_syscall(SYS_PFORK, 0, 0, 0, 0, 0);
}

long txt_tfork(const char *line)
{
    return irq_syscall(SYS_TFORK, 0, 0, 0, 0, 0);
}
long txt_sleep(const char *line)
{
    return irq_syscall(SYS_SLEEP, 0, 0, 0, 0, 0);
}

