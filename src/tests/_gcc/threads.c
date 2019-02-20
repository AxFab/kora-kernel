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
#include <threads.h>
#include <time.h>
#include <pthread.h>
#include <stdbool.h>
#include <kora/hmap.h>
#include <kora/std.h>

struct _US_THREAD {
    pthread_t handler;
    thrd_start_t func;
    void *arg;
};

void free(void *);

typedef void(*tss_dtor_t)(void *);

bool tss_thread_init = false;
tss_t tss_key_thread;

static void thrd_init()
{
    tss_thread_init = true;
    tss_create(&tss_key_thread, (tss_dtor_t)free);
    thrd_t thread = malloc(sizeof(* thread));
    tss_set(tss_key_thread, thread);
}


void thrd_start(thrd_t thread)
{
    tss_set(tss_key_thread, thread);
    thread->func(thread->arg);
    tss_delete(tss_key_thread);
    free(thread);
}


/* Creates a thread */
int thrd_create(thrd_t *thr, thrd_start_t func, void *arg)
{
    if (!tss_thread_init)
        thrd_init();
    thrd_t thread = malloc(sizeof(* thread));
    thread->func = func;
    thread->arg = arg;
    pthread_create(&thread->handler, NULL, (thrd_start_t)thrd_start, thread);
    return 0;
}

/* Checks if two identifiers refer to the same thread */
int thrd_equal(thrd_t lhs, thrd_t rhs)
{
    return lhs->handler == rhs->handler;
}

/* Obtains the current thread identifier */
thrd_t thrd_current(void)
{
    if (!tss_thread_init)
        thrd_init();
    return tss_get(tss_key_thread);
}
/* Suspends execution of the calling thread for the given period of time */
int thrd_sleep(const struct timespec *duration, struct timespec *remaining)
{
    struct timespec ts = *duration;
    nanosleep(&ts, remaining);
    return 0;
}

/* Yields the current time slice */
void thrd_yield(void)
{
    // TODO
}

/* Terminates the calling thread */
_Noreturn void thrd_exit(int res)
{
    // thrd_t thread = thrd_current();
    tss_delete(tss_key_thread);
    pthread_exit(&res);
}

/* Detaches a thread */
int thrd_detach(thrd_t thr)
{
    return -1;
}

/* Blocks until a thread terminates */
int thrd_join(thrd_t thr, int *res)
{
    // TODO
    return 0;
}





/* Creates thread-specific storage pointer with a given destructor */
int tss_create(tss_t *tss_key, tss_dtor_t destructor)
{
    return pthread_key_create(tss_key, destructor);
}

/* Reads from thread-specific storage */
void *tss_get(tss_t tss_key)
{
    return pthread_getspecific((pthread_key_t)tss_key);
}

/* Write to thread-specific storage */
int tss_set(tss_t tss_key, void *val)
{
    return pthread_setspecific((pthread_key_t)tss_key, val);
}

/* Releases the resources held by a given thread-specific pointer */
void tss_delete(tss_t tss_key)
{
    pthread_key_delete((pthread_key_t)tss_key);
}

