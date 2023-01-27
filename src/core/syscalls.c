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
#include <kernel/irq.h>
#include <kernel/syscalls.h>
#include <kernel/core.h>
#include <kora/llist.h>
#include <sys/signum.h>

extern sys_info_t sysinfo;



syscall_info_t __syscalls_info[] = {
    [SYS_EXIT] = SCALL_ENTRY(exit, ARG_INT, 0, 0, 0, 0, 0, 1),
    [SYS_SPAWN] = SCALL_ENTRY(spawn, ARG_STR, ARG_PTR, ARG_PTR, ARG_PTR, ARG_FLG, ARG_INT, 5),
    [SYS_THREAD] = SCALL_ENTRY(thread, ARG_STR, ARG_PTR, ARG_PTR, ARG_LEN, ARG_FLG, ARG_INT, 5),
    [SYS_SLEEP] = SCALL_ENTRY(sleep, ARG_PTR, ARG_PTR, 0, 0, 0, ARG_INT, 1),

    [SYS_FUTEX_WAIT] = SCALL_ENTRY(futex_wait, ARG_PTR, ARG_INT, ARG_INT, ARG_FLG, 0, ARG_INT, 4),
    [SYS_FUTEX_REQUEUE] = SCALL_ENTRY(futex_requeue, ARG_PTR, ARG_INT, ARG_INT, ARG_PTR, ARG_FLG, ARG_INT, 5),
    [SYS_FUTEX_WAKE] = SCALL_ENTRY(futex_wake, ARG_PTR, ARG_INT, 0, 0, 0, ARG_INT, 2),

    [SYS_MMAP] = SCALL_ENTRY(mmap, ARG_PTR, ARG_LEN, ARG_FLG, ARG_FD, ARG_LEN, ARG_PTR, 5),
    [SYS_MUNMAP] = SCALL_ENTRY(munmap, ARG_PTR, ARG_LEN, 0, 0, 0, ARG_INT, 2),
    // [SYS_MPROTECT] = SCALL_ENTRY(mprotect, ),

    [SYS_GINFO] = SCALL_ENTRY(ginfo, ARG_INT, ARG_PTR, ARG_LEN, 0, 0, ARG_INT, 1),
    [SYS_SINFO] = SCALL_ENTRY(sinfo, ARG_INT, ARG_PTR, ARG_LEN, 0, 0, ARG_INT, 1),

    [SYS_OPEN] = SCALL_ENTRY(open, ARG_FD, ARG_STR, ARG_FLG, ARG_FLG, 0, ARG_FD, 4),
    [SYS_CLOSE] = SCALL_ENTRY(close, ARG_FD, 0, 0, 0, 0, ARG_INT, 1),
    [SYS_READDIR] = SCALL_ENTRY(readdir, ARG_FD, ARG_PTR, ARG_LEN, 0, 0, ARG_INT, 1),
    [SYS_SEEK] = SCALL_ENTRY(seek, ARG_FD, ARG_LEN, ARG_LEN, ARG_INT, 0, ARG_INT, 4),
    [SYS_READ] = SCALL_ENTRY(read, ARG_FD, ARG_PTR, ARG_LEN, 0, 0, ARG_INT, 1),
    [SYS_WRITE] = SCALL_ENTRY(write, ARG_FD, ARG_PTR, ARG_LEN, 0, 0, ARG_INT, 1),
    // [SYS_ACCESS] = SCALL_ENTRY(access, ),
    // [SYS_FCNTL] = SCALL_ENTRY(fcntl, ARG_FD, ARG_INT, ARG_PTR, 0, 0, ARG_INT, 3),

    [SYS_PIPE] = SCALL_ENTRY(pipe, ARG_PTR, ARG_FLG, 0, 0, 0, ARG_INT, 2),
    [SYS_FSTAT] = SCALL_ENTRY(fstat, ARG_FD, ARG_STR, ARG_PTR, ARG_FLG, 0, ARG_INT, 2),

    // [SYS_SIGNAL] = SCALL_ENTRY(signal, ),
    // [SYS_RAISE] = SCALL_ENTRY(raise, ),
    // [SYS_SIGRET] = SCALL_ENTRY(sigret, ),

    [SYS_XTIME] = SCALL_ENTRY(xtime, ARG_INT, ARG_PTR, 0, 0, 0, ARG_INT, 1),

    [SYS_MOUNT] = SCALL_ENTRY(mount, ARG_STR, ARG_STR, ARG_STR, ARG_STR, ARG_INT, ARG_INT, 5),
    [SYS_MKFS] = SCALL_ENTRY(mkfs, ARG_STR, ARG_STR, ARG_STR, ARG_INT, 0, ARG_INT, 4),
};

static void scall_log_arg(task_t *task, char type, long arg)
{
    char tmp[16];
    switch (type) {
    case ARG_STR:
        strncat(task->sc_log, "\"", 64);
        strncat(task->sc_log, (char *)arg, 64);
        strncat(task->sc_log, "\"", 64);
        break;
    case ARG_INT:
        snprintf(tmp, 16, "%d", arg);
        strncat(task->sc_log, tmp, 64);
        break;
    case ARG_FLG:
        snprintf(tmp, 16, "0%o", arg);
        strncat(task->sc_log, tmp, 64);
        break;
    case ARG_LEN:
        snprintf(tmp, 16, "0x%x", arg);
        strncat(task->sc_log, tmp, 64);
        break;
    case ARG_FD:
        snprintf(tmp, 16, "%d<->", arg);
        strncat(task->sc_log, tmp, 64);
        break;
    default:
    case ARG_PTR:
        snprintf(tmp, 16, "@%08x", (void *)arg);
        strncat(task->sc_log, tmp, 64);
        break;
    }
}


long irq_syscall(unsigned no, long a1, long a2, long a3, long a4, long a5)
{
    int i;
    assert(irq_ready());

    task_t *task = __current;
    syscall_info_t *info = NULL;
    // kprintf(-1, "Task.%d] syscall %d\n", task->pid, no);
    if (no < (sizeof(__syscalls_info) / sizeof(syscall_info_t)))
        info = &__syscalls_info[no];

    if (info == NULL || info->scall == NULL) {
        kprintf(-1, "Task.%d] Unknown syscall %d\n", task->pid, no);
        task_stop(-1);
        scheduler_switch(TS_ZOMBIE);
        return -1;
    }

#ifndef NDEBUG
    bool strace = no != SYS_READ && no != SYS_WRITE;
    // Prepare strace log for debug
    long args[] = { a1, a2, a3, a4, a5};
    if (strace) {
        irq_disable();
        strncpy(task->sc_log, info->name, 64);
        strncat(task->sc_log, "(", 64);
        for (i = 0; i < info->split; ++i) {
            if (info->args[i] == 0)
                break;
            if (i != 0)
                strncat(task->sc_log, ", ", 64);
            scall_log_arg(task, info->args[i], args[i]);
        }

        if (info->args[5] == 0) {
            strncat(task->sc_log, ")", 64);
            kprintf(-1, "Task.%d] \033[96m%s\033[0m\n", task->pid, task->sc_log);
            task->sc_log[0] = '\0';
        }
        irq_enable();
    }
#endif

    assert(irq_ready());
    long ret = info->scall(a1, a2, a3, a4, a5);
    assert(irq_ready());

#ifndef NDEBUG
    // Complete strace log
    if (strace) {
        irq_disable();
        if (task->sc_log[0] == '\0')
            strncat(task->sc_log, " .....", 64);
        for (; i < 5; ++i) {
            if (info->args[i] == 0)
                break;
            if (i != 0)
                strncat(task->sc_log, ", ", 64);
            scall_log_arg(task, info->args[i], args[i]);
        }

        strncat(task->sc_log, ") = ", 64);
        scall_log_arg(task, info->args[5], ret);
        kprintf(-1, "Task.%d] \033[96m%s\033[0m\n", task->pid, task->sc_log);
        task->sc_log[0] = '\0';
        irq_enable();
    }
#endif

    return ret;
}

