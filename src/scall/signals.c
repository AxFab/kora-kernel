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
#include <kernel/vfs.h>
#include <errno.h>


int sys_sigraise(unsigned long signum, long pid)
{
    task_t *task = pid == 0 ? kCPU.running : task_search(pid);
    if (task == NULL) {
        return ENOENT;
        return -1;
    }
    return -1;
    // return task_kill(task, signum);
}

int sys_sigaction(unsigned long signum, void* sigaction)
{
    if (signum >= 32) {
        errno = EINVAL;
        return -1;
    }
    kCPU.running->shandler[signum].type = sigaction;
    errno = 0;
    return 0;
}

void sys_sigreturn()
{

}
