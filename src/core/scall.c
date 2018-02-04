#include <kernel/core.h>
#include <kernel/scall.h>
#include <kernel/task.h>
#include <stdio.h>
#include <errno.h>

scall_t scalls[] = {
    { (scall_handler)NULL, "scall", { SC_NOARG, SC_NOARG, SC_NOARG, SC_NOARG, SC_NOARG } },
    { (scall_handler)sys_exit, "exit", { SC_SIGNED, SC_NOARG, SC_NOARG, SC_NOARG, SC_NOARG } },
    { (scall_handler)sys_exec, "exec", { SC_STRING, SC_STRING, SC_NOARG, SC_NOARG, SC_NOARG } },
    { (scall_handler)sys_kill, "kill", { SC_UNSIGNED, SC_SIGNED, SC_NOARG, SC_NOARG, SC_NOARG } },
    { (scall_handler)sys_wait, "wait", { SC_UNSIGNED, SC_UNSIGNED, SC_NOARG, SC_NOARG, SC_NOARG } },
    { (scall_handler)sys_write, "write_dbg", { SC_FD, SC_STRING, SC_SIGNED, SC_NOARG, SC_NOARG } },
    { (scall_handler)sys_sigaction, "sigaction", { SC_SIGNED, SC_POINTER, SC_NOARG, SC_NOARG, SC_NOARG } },
    // EXEC
    // EXIT GROUP
    // GET PROCESS STAT (PID / MEM / )
    // CLONE / FORK
    // CHDIR
    // CHROOT
    // { "yield", SC_NOARG, SC_NOARG, SC_NOARG, SC_NOARG, SC_NOARG },

    // WAIT PID
    // ALARM
    // SIGNAL
    // SIGACTION
    // SIGSUSPEND / SIGPENDING
    // NANOSLEEP
    // ITIMER

    { (scall_handler)sys_open, "open", { SC_STRING, SC_HEX, SC_OCTAL, SC_NOARG, SC_NOARG } },
    { (scall_handler)sys_close, "close", { SC_FD, SC_NOARG, SC_NOARG, SC_NOARG, SC_NOARG } },
    { (scall_handler)sys_read, "read", { SC_FD, SC_STRUCT, SC_UNSIGNED, SC_NOARG, SC_NOARG } },
    { (scall_handler)sys_write, "write", { SC_FD, SC_STRUCT, SC_UNSIGNED, SC_NOARG, SC_NOARG } },
    { (scall_handler)sys_seek, "seek", { SC_FD, SC_OFFSET, SC_NOARG, SC_SIGNED, SC_NOARG } },
    // DUP
    { (scall_handler)sys_pipe, "pipe", { SC_POINTER, SC_UNSIGNED, SC_NOARG, SC_NOARG, SC_NOARG } },
    // READ AHEAD

    { (scall_handler)sys_power, "power", { SC_SIGNED, SC_UNSIGNED, SC_NOARG, SC_NOARG, SC_NOARG } },
    // TIME / CLOCK
    // SET TIME - NTP - ADJTIMEX
    // UNAME
    // GET / SET STRING (HOSTNAME / DOMAIN ...)

    // SYSLOG
    // FUTEX / MUTEX
    // KERNEL MODULE (INIT / DELETE)

    // USE LIB

    // SWAP ON / OFF
    // MMAP / UNMAP
    // MPROTECT
    // MSYNC
    // MLOCK / MUNLOCK

    // SOCKET (SEND / RECEIVE)
};

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

long kernel_scall(long no, long a1, long a2, long a3, long a4, long a5)
{
    int i;
    kprintf(0, "[SYSC] PID:%d, No %d.\n", kCPU.running->pid, no);

    scall_t *sc = &scalls[no];
    long *args = &a1;
    char *buf = (char *)kalloc(1024);
    int lg = sprintf(buf, "sys_%s(", sc->name);
    for (i = 0; i < 5; ++i) {
        if (i != 0 && sc->args[i - 1] != SC_NOARG) {
            lg += sprintf(&buf[lg], ", ");
        }
        switch (sc->args[i]) {
        case SC_NOARG:
            break;
        case SC_SIGNED:
            lg += sprintf(&buf[lg], "%d", args[i]);
            break;
        case SC_UNSIGNED:
            lg += sprintf(&buf[lg], "%u", args[i]);
            break;
        case SC_OCTAL:
            lg += sprintf(&buf[lg], "0%o", args[i]);
            break;
        case SC_HEX:
            lg += sprintf(&buf[lg], "0x%x", args[i]);
            break;
        case SC_STRING:
            lg += sprintf(&buf[lg], "\"%s\"", (char *)args[i]);
            break;
        case SC_FD:
            lg += sprintf(&buf[lg], "%d:...", args[i]);
            break;
        case SC_STRUCT:
            lg += sprintf(&buf[lg], "0x%x", args[i]);
            break;
        case SC_OFFSET:
            lg += sprintf(&buf[lg], "%d", args[i]);
            break;
        case SC_POINTER:
            lg += sprintf(&buf[lg], "%p", args[i]);
            break;
        }
    }
    int ret = sc->handler(a1, a2, a3, a4, a5);
    lg += sprintf(&buf[lg], ") = %d [%d]\n", ret, errno);
    kprintf(0, buf);
    kfree(buf);
    return 0;
}

