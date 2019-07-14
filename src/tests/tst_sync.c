#include <kernel/core.h>
#include <threads.h>
#include <assert.h>
#include "check.h"

typedef long long tick_t;

const int th_N = 5;
atomic_int test_ck1;
cnd_t test_cnd1;
mtx_t test_mtx1;

void futex_init();
void scheduler_init();

static void tst_sync_01_waiter()
{
    test_ck1++;
    // printf("A %d\n", cpu_no());
    mtx_lock(&test_mtx1);
    // printf("B %d\n", cpu_no());
    usleep(MSEC_TO_USEC(100));
    // printf("C %d\n", cpu_no());
    mtx_unlock(&test_mtx1);
    // printf("D %d\n", cpu_no());
    usleep(MSEC_TO_USEC(100));
    // printf("E %d\n", cpu_no());
    mtx_lock(&test_mtx1);
    // printf("F %d\n", cpu_no());
    usleep(MSEC_TO_USEC(100));
    // printf("G %d\n", cpu_no());
    mtx_unlock(&test_mtx1);
    // printf("H %d\n", cpu_no());
}

TEST_CASE(tst_sync_01)
{
    int i;
    thrd_t threads[th_N];
    mtx_init(&test_mtx1, mtx_plain);
    test_ck1 = 0;

    tick_t start = clock_read(CLOCK_MONOTONIC);
    for (i = 0; i < th_N; ++i)
        thrd_create(&threads[i], (thrd_start_t)tst_sync_01_waiter, NULL);
    usleep(MSEC_TO_USEC(100));
    ck_ok(test_ck1 == th_N);

    for (i = 0; i < th_N; ++i)
        thrd_join(threads[i], NULL);

    tick_t elapsed = clock_read(CLOCK_MONOTONIC) - start;
    ck_ok(elapsed >= th_N * MSEC_TO_USEC(200));
    ck_ok(elapsed <= th_N * MSEC_TO_USEC(300));
}

static void tst_sync_02_waiter()
{
    mtx_lock(&test_mtx1);
    test_ck1++;
    cnd_wait(&test_cnd1, &test_mtx1);
    mtx_unlock(&test_mtx1);
    usleep(MSEC_TO_USEC(100));
}

TEST_CASE(tst_sync_02)
{
    int i;
    thrd_t threads[th_N];
    mtx_init(&test_mtx1, mtx_plain);
    cnd_init(&test_cnd1);
    test_ck1 = 0;

    for (i = 0; i < th_N; ++i)
        thrd_create(&threads[i], (thrd_start_t)tst_sync_02_waiter, NULL);
    usleep(MSEC_TO_USEC(10));
    ck_ok(test_ck1 == th_N);

    tick_t start = clock_read(CLOCK_MONOTONIC);
    usleep(MSEC_TO_USEC(100));
    cnd_broadcast(&test_cnd1);
    for (i = 0; i < th_N; ++i)
        thrd_join(threads[i], NULL);

    tick_t elapsed = clock_read(CLOCK_MONOTONIC) - start;
    ck_ok(elapsed >= MSEC_TO_USEC(100));
    ck_ok(elapsed < th_N * MSEC_TO_USEC(100));
}

static void tst_sync_03_task()
{
    struct timespec tp;
    tp.tv_sec = 0;
    tp.tv_nsec = 500 * 1000 * 1000;
    int ret = mtx_timedlock(&test_mtx1, &tp);
    ck_ok(ret == thrd_timedout);
}

TEST_CASE(tst_sync_03)
{
    thrd_t thread;
    mtx_init(&test_mtx1, mtx_timed);
    mtx_lock(&test_mtx1);

    tick_t start = clock_read(CLOCK_MONOTONIC);
    thrd_create(&thread, (thrd_start_t)tst_sync_03_task, NULL);
    thrd_join(thread, NULL);
    tick_t elapsed = clock_read(CLOCK_MONOTONIC) - start;
    ck_ok(elapsed >= MSEC_TO_USEC(500));
}


jmp_buf __tcase_jump;
SRunner runner;

int main()
{
    kSYS.cpus = calloc(sizeof(struct kCpu), 0x500);
    futex_init();
    scheduler_init();

    suite_create("Synchronization primitives");
    fixture_create("Mutex");
    tcase_create(tst_sync_01);
    tcase_create(tst_sync_03);
    fixture_create("Condition variable");
    tcase_create(tst_sync_02);

    free(kSYS.cpus);
    return summarize();
}


