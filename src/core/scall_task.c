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
#include <kernel/tasks.h>
#include <kernel/memory.h>
#include <kernel/vfs.h>
#include <errno.h>
#include <kernel/syscalls.h>

#include <bits/mman.h>
#include <bits/io.h>

// Signals
// #define SYS_SIGRAISE  15
// #define SYS_SIGACTION  16
// #define SYS_SIGRETURN  17

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void sys_exit(int code)
{
    task_stop(code);
    for (;;)
        scheduler_switch(TS_ZOMBIE);
}

// long sys_wait(int pid, int *status, int option)
// {
//     for (;;) {
//         // TODO - Does a child did terminate ?
//         scheduler_switch(TS_BLOCKED);
//     }
// }


long sys_sleep(xtime_t *timeout, xtime_t *remain)
{
    // TODO - Check vmsp_check(__current->vmsp, remain, sizeof(long));
    if (*timeout <= 0)
        scheduler_switch(TS_READY);
    else {
        xtime_t rest = sleep_timer(*timeout);
        if (*remain)
            *remain = rest;
    }
    errno = 0;
    return 0;
}


long sys_futex_wait(int *addr, int val, xtime_t *timeout, int flags)
{
    // if (!vmsp_check(__current->vmsp, addr, sizeof(int), VM_RW) return -1;
    // TODO - Check vmsp_check(__current->vmsp, addr, sizeof(int));
    return futex_wait(addr, val, *timeout, flags);
}

long sys_futex_requeue(int *addr, int val, int val2, int *addr2, int flags)
{
    // TODO - Check vmsp_check(__current->vmsp, addr, sizeof(int));
    return futex_requeue(addr, val, val2, addr2, flags);
}

long sys_futex_wake(int *addr, int val)
{
    // TODO - Check vmsp_check(__current->vmsp, addr, sizeof(int));
    return futex_wake(addr, val, 0);
}


long sys_spawn(const char *program, const char **args, const char **envs, int *streams, int flags)
{
    if (vmsp_check_str(__current->vmsp, program, 4096) != 0)
        return -1;
    if (vmsp_check_strarray(__current->vmsp, args) != 0)
        return -1;
    if (vmsp_check_strarray(__current->vmsp, envs) != 0)
        return -1;
    if (vmsp_check(__current->vmsp, streams, 3 * sizeof(int), VM_RD) != 0)
        return -1;

    file_t *file;
    inode_t *inodes[3];

    // Lock fset..
    for (int i = 0; i < 3; ++i) {
        file = resx_get(__current->fset, RESX_FILE, streams[i]);
        if (file != NULL)
            inodes[i] = vfs_open_inode(file->ino);
    }
    // Unlock fset

    size_t pid = task_spawn(program, args, inodes);

    for (int i = 0; i < 3; ++i)
        vfs_close_inode(inodes[i]);

    return (long)pid;
}

long sys_thread(const char *name, void *entry, void *params, size_t len, int flags)
{
    if (name != NULL && vmsp_check_str(__current->vmsp, name, 4096) != 0)
        return -1;
    if (vmsp_check(__current->vmsp, params, len, VM_RD) != 0)
        return -1;

    return task_thread(name, entry, params, len, KEEP_FS | KEEP_FSET | KEEP_VM);
}

