/*
 *      This file is part of the KoraOs project.
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
#include <kernel/sys/inode.h>
#include <errno.h>

int sys_scall(const char *sc_vec)
{
    errno = ENOSYS;
    return -1;
}

void sys_exit(int code)
{
    task_stop(kCPU.running, code);
    scheduler_next();
}

int sys_exec(const char *exec, const char *cmdline)
{
    task_t *parent = kCPU.running;
    inode_t *ino = vfs_search(parent->root, parent->pwd, exec);
    if (ino == NULL) {
        return -1;
    }

    task_t *task = task_create(NULL, parent->root, TSK_USER_SPACE);
    if (elf_open(task, ino) != 0) {
        return -1;
    }

    errno = 0;
    return 0;
}

int sys_kill(pid_t pid, int code)
{
    task_t *task = task_search(pid);
    if (task == NULL) {
        return ENOENT;
        return -1;
    }
    return task_kill(task, code);
}

int sys_wait(int flags, uint32_t timeout)
{
    asm("int $0x41");
    errno = 0;
    return 0;
}

int sys_sigaction(int signum, void *handler)
{
    if (signum < 0 || signum >= 32) {
        errno = EINVAL;
        return -1;
    }
    kCPU.running->shandler[signum].type = handler;
    errno = 0;
    return 0;
}
