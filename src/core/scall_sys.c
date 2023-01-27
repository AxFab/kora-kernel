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

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

static int copystr(bool access, const char *value, char *buf, size_t len)
{
    if (!access || len < strlen(value) + 1)
        return -1;
    strncpy(buf, value, len);
    return 0;
}

long sys_ginfo(unsigned info, char *buf, size_t len)
{
    if (mspace_check(__current->vm, buf, len, VM_WR) != 0)
        return -1;

    switch (info) {
    case SNFO_ARCH:
        return copystr(true, __ARCH, buf, len);
    // case SNFO_GITH:
    //    return copystr(true, _GITH_, buf, len);
    case SNFO_SNAME:
        return copystr(true, "Kora", buf, len);
    // case SNFO_VERSION:
    //     return copystr(true, _MP_ " Kora core " _VTAG_ "-" _GITH_ " ("_DATE_")", buf, len);
    // case SNFO_RELEASE:
    //     return copystr(true, _VTAG_"-"__ARCH, buf, len);
    case SNFO_OSNAME:
        return copystr(true, "KoraOS", buf, len);
    // case SNFO_USER:
    case SNFO_PWD:
        return vfs_readpath(__current->fsa, ".", __current->user, buf, len, false);
    // case SNFO_USERNAME:
        // case SNFO_USERMAIL:
        // case SNFO_HOSTNAME:
    // case SNFO_DOMAIN:
    default:
        errno = EINVAL;
        return -1;
    }
}

long sys_sinfo(unsigned info, const char *buf, size_t len)
{
    if (mspace_check(__current->vm, buf, len, VM_RD) != 0)
        return -1;

    switch (info) {
    // case SNFO_HOSTNAME:
    // case SNFO_DOMAIN:
    case SNFO_PWD:
        return vfs_chdir(__current->fsa, buf, NULL, false);
    default:
        errno = EINVAL;
        return -1;
    }
}

long sys_syslog(const char *log)
{
    kprintf(KL_USR, "Task.%d] %s\n", __current->pid, log);
    return 0;
}



int sys_xtime(int name, xtime_t *ptime)
{
    *ptime = xtime_read(name);
    return 0;
}
