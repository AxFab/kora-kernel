/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2019  <Fabien Bavent>
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
#include <kernel/task.h>
#include <kora/mcrs.h>
#include <errno.h>
#include "../check.h"

struct kSys kSYS;

int counter[10];


void task_count(long arg)
{
    for (;;) {
        async_wait(NULL, NULL, MSEC_TO_KTIME(50));
        counter[arg]++;
    }
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void cpu_loop(int loop)
{
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 1000000;

    kCPU.flags |= CPU_NO_TASK;

    while (loop-- > 0) {
        irq_reset(true);
        nanosleep(&ts, NULL);
        irq_disable();
        assert(kCPU.irq_semaphore == 1);
        clock_ticks(0);
        kCPU.io_elapsed += clock_elapsed(&kCPU.last);
        if (kCPU.running)
            kCPU.running->other_elapsed += clock_elapsed(&kCPU.running->last);
        assert(kCPU.irq_semaphore == 1);
    }
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


START_TEST(test_01)
{
    cpu_setup();
    clock_init();

    task_t *task;
    task = task_create(task_count, (void *)0, "T1");
    task_close(task);
    task = task_create(task_count, (void *)1, "T2");
    task_close(task);

    cpu_loop(500);
    task_show_all();

    task = task_search(1);
    if (task) {
        task_stop(task, 0);
        task_close(task);
    }

    task = task_search(2);
    if (task) {
        task_stop(task, 0);
        task_close(task);
    }

    assert(counter[0] > 1 && counter[1] > 1);

    // TODO --- Join all other threads
    int retry = 3;
    while(retry--) {
        struct timespec ts;
        ts.tv_sec = 1;
        ts.tv_nsec = 0;
        nanosleep(&ts, NULL);
    }
}
END_TEST


START_TEST(test_02)
{
}
END_TEST


void fixture_starts(Suite *s)
{
    TCase *tc;

    tc = tcase_create("Task start/stop");
    tcase_add_test(tc, test_01);
    // tcase_add_test(tc, test_02);
    suite_add_tcase(s, tc);
}


