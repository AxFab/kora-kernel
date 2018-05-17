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
#include <errno.h>


int sys_power(unsigned long action, unsigned long delay_us)
{
    return -1;
}

int sys_scall(const char *module)
{
    usr_check_cstr(module, 4096);
    return -1;
}


int sys_syslog(const char *msg)
{
    usr_check_cstr(msg, 4096);
    kprintf(KLOG_USR, "%s\n", msg);
    errno = 0;
    return 0;
}

int sys_sysinfo(unsigned long info, char *buf, size_t len)
{
    usr_check_buf(buf, len);
    if (len < 8) {
        errno = EINVAL;
        return -1;
    }
    switch (info) {
    // case SI_GET_ARCH:
    //     strncpy(buf, __ARCH, len);
    //     break;
    // case SI_GET_VTAG:
    //     strncpy(buf, _VTAG_, len);
    //     break;
    // case SI_GET_HOSTNAME:
    //     strcpy(buf, "");
    //     if (kSYS.host_name)
    //         strncpy(buf, kSYS.host_name, len);
    //     break;
    // case SI_GET_DOMAINNAME:
    //     strcpy(buf, "");
    //     if (kSYS.domain_name)
    //         strncpy(buf, kSYS.domain_name, len);
    //     break;
    // case SI_SET_HOSTNAME:
    //     if (false) {
    //         errno = EPERM;
    //         return -1;
    //     }
    //     if (kSYS.host_name)
    //         kfree(kSYS.host_name);
    //     kprintf(KLOG_MSG, "\e[32mChange of hostname by UID: \e[33mHOSTNAME\e[0m\n");
    //     kSYS.host_name = strndup(buf, len);
    //     break;
    // case SI_SET_DOMAINNAME:
    //     if (false) {
    //         errno = EPERM;
    //         return -1;
    //     }
    //     if (kSYS.host_name)
    //         kfree(kSYS.host_name);
    //     kprintf(KLOG_MSG, "\e[32mChange of domain name by UID: \e[33mDOMAINNAME\e[0m\n");
    //     kSYS.host_name = strndup(buf, len);
    default:
        errno = EINVAL;
        return -1;
    }
    errno = 0;
    return 0;
}
