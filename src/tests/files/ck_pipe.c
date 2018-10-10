/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2018  <Fabien Bavent>
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
#include <kernel/files.h>
#include <fcntl.h>
#include "../check.h"


#if defined _WIN32
int open(const char *name, int flags);
#else
#define O_BINARY 0
#endif
int read(int fd, char *buf, int len);
int write(int fd, const char *buf, int len);
int lseek(int fd, off_t off, int whence);
void close(int fd);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

START_TEST(test_pipe)
{
    int bytes;
    char *buf;
    pipe_t *pipe = pipe_create();
    ck_assert(pipe != NULL && errno == 0);

    bytes = pipe_write(pipe, "Hello world!\n\n", 14, IO_NO_BLOCK);
    ck_assert(bytes == 14 && errno == 0);

    int fd = open("../../../src/tests/lorem.txt", O_RDONLY | O_BINARY);
    if (fd == -1)
        return;
    buf = kalloc(4096);
    int lg = read(fd, buf, 4096);

    bytes = pipe_write(pipe, buf, lg, IO_NO_BLOCK);
    ck_assert(bytes == lg && errno == 0);

    bytes = pipe_write(pipe, buf, lg, IO_NO_BLOCK);
    ck_assert(bytes == -1 && errno == EWOULDBLOCK);


}
END_TEST


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


void fixture_pipe(Suite *s)
{
    TCase *tc;

    tc = tcase_create("Pipe");
    tcase_add_test(tc, test_pipe);
    suite_add_tcase(s, tc);
}
