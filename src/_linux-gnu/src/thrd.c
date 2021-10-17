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
// #include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <errno.h>
// #include <unistd.h>
// #include "check.h"
#include <threads.h>

#include <kernel/task.h>
#include <kernel/futex.h>

void cpu_sweep();

struct impl_thrd_param {
    thrd_start_t func;
    void *arg;
};

void *impl_thrd_routine(void *p)
{
    struct impl_thrd_param pack = *((struct impl_thrd_param *)p);
    free(p);
    cpu_setup();
    void *ret = (void *)(size_t)pack.func(pack.arg);
    cpu_sweep();
    return ret;
}


int thrd_create(thrd_t *thr, thrd_start_t func, void *arg)
{
    struct impl_thrd_param *pack;
    if (!thr)
        return thrd_error;
    pack = malloc(sizeof(struct impl_thrd_param));
    if (!pack)
        return thrd_nomem;
    pack->func = func;
    pack->arg = arg;
    if (pthread_create(thr, NULL, impl_thrd_routine, pack) != 0) {
        free(pack);
        return thrd_error;
    }
    return thrd_success;
}

int thrd_detach(thrd_t thr)
{
    return (pthread_detach(thr) == 0) ? thrd_success : thrd_error;
}

int thrd_equal(thrd_t thr0, thrd_t thr1)
{
    return pthread_equal(thr0, thr1);
}

void thrd_exit(int res)
{
    for (;;)
        pthread_exit((void *)(size_t)res);

}

int thrd_join(thrd_t thr, int *res)
{
    void *code;
    if (pthread_join(thr, &code) != 0)
        return thrd_error;
    if (res)
        *res = (int)(size_t)code;
    return thrd_success;
}

int thrd_sleep(const struct timespec *req, struct timespec *res)
{
    struct timespec tp = *req;
    nanosleep(&tp, res);
    return 0;
}

void thrd_yield(void)
{
    sched_yield();
}

utime_t cpu_clock(int clk)
{
    struct timespec xt;
    clock_gettime(clk, &xt);
    return TMSPEC_TO_USEC(xt);
}
