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
#ifndef _KERNEL_SCALL_H
#define _KERNEL_SCALL_H 1

typedef long (*scall_handler)(long a1, long a2, long a3, long a4, long a5);

typedef struct scall scall_t;
struct scall {
    scall_handler handler;
    char name[12];
    uint8_t args[5];
};

enum {
    SC_NOARG,
    SC_SIGNED,
    SC_UNSIGNED,
    SC_OCTAL,
    SC_HEX,
    SC_STRING,
    SC_FD,
    SC_STRUCT,
    SC_OFFSET,
    SC_POINTER,
};


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

struct iovec {
    char *buffer;
    size_t length;
};

struct image
{
    int width, height, pitch, format;
};


#define PW_SHUTDOWN  1
#define PW_REBOOT  2
#define PW_SLEEP  3
#define PW_HIBERNATE  4




/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int sys_scall(const char *sc_vec);
void sys_exit(int code);
int sys_exec(const char *exec, const char *cmdline);
int sys_kill(pid_t pid, int code);
int sys_wait(int flags, uint32_t timeout);
int sys_sigaction(int signum, void *handler);
int sys_sigreturn();

int sys_open(const char *name, int flags, int mode);
int sys_close(int fd);
int sys_read(int fd, const struct iovec *vec, unsigned vlen);
int sys_write(int fd, const struct iovec *vec, unsigned vlen);
int sys_seek(int fd, off_t offset, int whence);

int sys_pipe(int *fd, size_t size);

int sys_power(int cmd, unsigned int delay);

void *sys_mmap(size_t address, size_t length, int fd, off_t offset, int flags);
int sys_mprotect(size_t address, size_t length, int flags);
int sys_munmap(size_t address, size_t length);

int sys_window(struct image *img, int features, int events);




#endif  /* _KERNEL_SCALL_H */
