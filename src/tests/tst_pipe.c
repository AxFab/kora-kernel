#include <kernel/core.h>
#include <kernel/files.h>
#include <threads.h>
#include <assert.h>
#include <string.h>
#include "check.h"

void futex_init();

void async_wait() {}
void async_raise() {}


pipe_t *pipe_create();
void pipe_destroy(pipe_t *pipe);
int pipe_resize(pipe_t *pipe, size_t size);
int pipe_erase(pipe_t *pipe, size_t len);
int pipe_reset(pipe_t *pipe);
int pipe_write(pipe_t *pipe, const char *buf, size_t len, int flags);
int pipe_read(pipe_t *pipe, char *buf, size_t len, int flags);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

TEST_CASE(tst_pipe_01)
{
    int i, n = PAGE_SIZE / 26 + 1;
    char buf[32];
    pipe_t *pp = pipe_create();
    ck_ok(26 == pipe_write(pp, "abcdefghijklmnopqrstuvwxyz", 26, 0));
    ck_ok(26 == pipe_read(pp, buf, 26, 0));
    ck_ok(memcmp(buf, "abcdefghijklmnopqrstuvwxyz", 26) == 0);

    for (i = 0; i < n; ++i) {
        ck_ok(26 == pipe_write(pp, "abcdefghijklmnopqrstuvwxyz", 26, 0));
    }

    for (i = 0; i < n; ++i) {
        ck_ok(26 == pipe_read(pp, buf, 26, 0));
        ck_ok(memcmp(buf, "abcdefghijklmnopqrstuvwxyz", 26) == 0);
    }

    ck_ok(0 == pipe_resize(pp, PAGE_SIZE));

    ck_ok(26 == pipe_write(pp, "abcdefghijklmnopqrstuvwxyz", 26, 0));
    ck_ok(26 == pipe_read(pp, buf, 26, 0));
    ck_ok(memcmp(buf, "abcdefghijklmnopqrstuvwxyz", 26) == 0);

    for (i = 0; i < n; ++i) {
        ck_ok(26 == pipe_write(pp, "abcdefghijklmnopqrstuvwxyz", 26, 0));
    }

    ck_ok(-1 == pipe_resize(pp, PAGE_SIZE));

    for (i = 0; i < n; ++i) {
        ck_ok(26 == pipe_read(pp, buf, 26, 0));
        ck_ok(memcmp(buf, "abcdefghijklmnopqrstuvwxyz", 26) == 0);
    }

    pipe_destroy(pp);
}

TEST_CASE(tst_pipe_02)
{
    int i, n = PAGE_SIZE / 26 + 1;
    char buf[32];
    pipe_t *pp = pipe_create();
    ck_ok(26 == pipe_write(pp, "abcdefghijklmnopqrstuvwxyz", 26, IO_ATOMIC | IO_NO_BLOCK));
    ck_ok(26 == pipe_read(pp, buf, 26, IO_ATOMIC | IO_NO_BLOCK));
    ck_ok(memcmp(buf, "abcdefghijklmnopqrstuvwxyz", 26) == 0);

    // for (i = 0; i < n; ++i) {
    //     ck_ok(26 == pipe_write(pp, "abcdefghijklmnopqrstuvwxyz", 26, IO_ATOMIC));
    // }

    // for (i = 0; i < n; ++i) {
    //     ck_ok(26 == pipe_read(pp, buf, 26, IO_ATOMIC));
    //     ck_ok(memcmp(buf, "abcdefghijklmnopqrstuvwxyz", 26) == IO_ATOMIC);
    // }

    ck_ok(-1 == pipe_read(pp, buf, 26, IO_ATOMIC | IO_NO_BLOCK));
    // ck_ok(errno == EWOULDBLOCK);



    pipe_destroy(pp);
}


jmp_buf __tcase_jump;
SRunner runner;

int main()
{
    kSYS.cpus = calloc(sizeof(struct kCpu), 0x500);
    futex_init();

    suite_create("Pipe");
    fixture_create("Non blocking operation");
    tcase_create(tst_pipe_01);

    tcase_create(tst_pipe_02);

    free(kSYS.cpus);
    return summarize();
}


