/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015  <Fabien Bavent>
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
#include <kernel/scall.h>
#include <kernel/task.h>
#include <kora/syscalls.h>
#include <errno.h>


scall_t scalls[] = {
    /* System */
    [SYS_POWER] = {
        (scall_handler)sys_power, "power", true,
        { SC_UNSIGNED, SC_UNSIGNED, SC_NOARG, SC_NOARG, SC_NOARG }
    },
    [SYS_SCALL] = {
        (scall_handler)sys_scall, "scall", true,
        { SC_STRING, SC_NOARG, SC_NOARG, SC_NOARG, SC_NOARG }
    },
    [SYS_SYSLOG] = {
        (scall_handler)sys_syslog, "syslog", true,
        { SC_STRING, SC_NOARG, SC_NOARG, SC_NOARG, SC_NOARG }
    },
    [SYS_SYSINFO] = {
        (scall_handler)sys_sysinfo, "sysinfo", true,
        { SC_UNSIGNED, SC_POINTER, SC_UNSIGNED, SC_NOARG, SC_NOARG }
    },
    // TIME / CLOCK | SET TIME - NTP - ADJTIMEX
    // KERNEL MODULE (INIT / DELETE)

    /* Task */
    [SYS_YIELD] = {
        (scall_handler)sys_yield, "yield", false,
        { SC_OCTAL, SC_NOARG, SC_NOARG, SC_NOARG, SC_NOARG }
    },
    [SYS_EXIT] = {
        (scall_handler)sys_exit, "exit", false,
        { SC_SIGNED, SC_UNSIGNED, SC_NOARG, SC_NOARG, SC_NOARG }
    },
    [SYS_WAIT] = {
        (scall_handler)sys_wait, "wait", false,
        { SC_UNSIGNED, SC_SIGNED, SC_UNSIGNED, SC_NOARG, SC_NOARG }
    },
    [SYS_EXEC] = {
        (scall_handler)sys_exec, "exec", true,
        { SC_STRING, SC_POINTER, SC_POINTER, SC_OCTAL, SC_NOARG }
    },
    [SYS_CLONE] = {
        (scall_handler)sys_clone, "clone", true,
        { SC_OCTAL, SC_NOARG, SC_NOARG, SC_NOARG, SC_NOARG }
    },
    // GET PROCESS STAT (PID / MEM / ) |  CHDIR |  CHROOT | GET/SET UID
    // NANOSLEEP | ITIMER | PRIORITY / RT
    // USE LIB

    /* Signals */
    [SYS_SIGRAISE] = {
        (scall_handler)sys_sigraise, "sigraise", true,
        { SC_UNSIGNED, SC_SIGNED, SC_NOARG, SC_NOARG, SC_NOARG }
    },
    [SYS_SIGACTION] = {
        (scall_handler)sys_sigaction, "sigaction", true,
        { SC_UNSIGNED, SC_POINTER, SC_NOARG, SC_NOARG, SC_NOARG }
    },
    [SYS_SIGRETURN] = {
        (scall_handler)sys_sigreturn, "sigreturn", false,
        { SC_NOARG, SC_NOARG, SC_NOARG, SC_NOARG, SC_NOARG }
    },
    // SIGSUSPEND / SIGPENDING

    /* Memory */
    [SYS_MMAP] = {
        (scall_handler)sys_mmap, "mmap", true,
        { SC_HEX, SC_HEX, SC_FD, SC_OFFSET, SC_OCTAL }
    },
    [SYS_UNMAP] = {
        (scall_handler)sys_munmap, "munmap", true,
        { SC_HEX, SC_HEX, SC_NOARG, SC_NOARG, SC_NOARG }
    },
    [SYS_MPROTECT] = {
        (scall_handler)sys_mprotect, "mprotect", true,
        { SC_HEX, SC_HEX, SC_OCTAL, SC_NOARG, SC_NOARG }
    },
    // SWAP ON / OFF |  MSYNC |  MLOCK / MUNLOCK

    /* Stream */
    [SYS_OPEN] = {
        (scall_handler)sys_open, "open", true,
        { SC_FD, SC_STRING, SC_OCTAL, SC_OCTAL, SC_NOARG }
    },
    [SYS_CLOSE] = {
        (scall_handler)sys_close, "close", true,
        { SC_FD, SC_NOARG, SC_NOARG, SC_NOARG, SC_NOARG }
    },
    [SYS_READ] =  {
        (scall_handler)sys_read, "read", true,
        { SC_FD, SC_POINTER, SC_UNSIGNED, SC_NOARG, SC_NOARG }
    },
    [SYS_WRITE] = {
        (scall_handler)sys_write, "write", true,
        { SC_FD, SC_POINTER, SC_UNSIGNED, SC_NOARG, SC_NOARG }
    },
    [SYS_SEEK] = {
        (scall_handler)sys_seek, "seek", true,
        { SC_FD, SC_OFFSET, SC_SIGNED, SC_NOARG, SC_NOARG }
    },
    // DUP |  READ AHEAD

    /* - */
    [SYS_WINDOW] = {
        (scall_handler)sys_window, "window", true,
        { SC_POINTER, SC_FD, SC_POINTER, SC_OCTAL, SC_OCTAL }
    },
    [SYS_PIPE] = {
        (scall_handler)sys_pipe, "pipe", true,
        { SC_POINTER, SC_UNSIGNED, SC_NOARG, SC_NOARG, SC_NOARG }
    },
    // SOCKET (SEND / RECEIVE) |  FUTEX / MUTEX


};

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


void usr_check_cstr(const char *str, unsigned len)
{
    // vma_t *vma = mspace_search_vma(kCPU.current->mspace);
    // CHECK POINTER / NULL TERM / VALID UTF-8
}

void usr_check_buf(const char *buf, unsigned len)
{
    // CHECK POINTER AND SIZE!
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

long irq_syscall(long no, long a1, long a2, long a3, long a4, long a5)
{
    int i;
    // kprintf(KLOG_DBG, "[SYSC] PID:%d, No %d.\n", kCPU.running->pid, no);

    scall_t *sc = &scalls[no];
    long *args = &a1;
    char *buf = (char *)kalloc(1024);
    int lg = sprintf(buf, "sys_%s(", sc->name);
    for (i = 0; i < 5; ++i) {
        if (i != 0 && sc->args[i - 1] != SC_NOARG)
            lg += sprintf(&buf[lg], ", ");
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

    if (!sc->retrn) {
        lg += sprintf(&buf[lg], ")\n");
        kprintf(KLOG_SYC, buf);
    }

    int ret = sc->handler(a1, a2, a3, a4, a5);
    lg += sprintf(&buf[lg], ") = %d [%d]\n", ret, errno);
    kprintf(KLOG_SYC, buf);
    kfree(buf);
    return ret;
}

