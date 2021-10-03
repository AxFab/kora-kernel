#include <threads.h>
#include <limits.h>
#include <kernel/stdc.h>
#include <kora/mcrs.h>
#include <bits/atomic.h>

/* creates a mutex */
int mtx_init(mtx_t *mutex, int type)
{
    atomic_store(&mutex->value, 0);
    // check error, deadlocks chain, recursive
    return 0;
}

/* blocks until locks a mutex */
int mtx_lock(mtx_t *mutex)
{
    struct timespec time_point;
    time_point.tv_sec = -1;
    return mtx_timedlock(mutex, &time_point);
}

/* blocks until locks a mutex or times out */
int mtx_timedlock(mtx_t *restrict mutex, const struct timespec *restrict time_point)
{
    int i, s;
    long timeout = time_point->tv_sec < 0 ? -1 : TMSPEC_TO_USEC(*time_point);

    /* spin and try to take lock */
    for (i = 0; i < 100; i++) {
        s = atomic_cmpxchg(&mutex->value, 0, 1);
        if (s)
            return 0;
        atomic_break();
    }

    /* the lock is now contended */
    if (s == 1)
        s = atomic_xchg(&mutex->value, 2);

    xtime_t start;
    if (time_point->tv_sec >= 0)
        start = xtime_read(XTIME_CLOCK);

    while (s) {
        /* wait in the kernel */
        futex_wait((int *)&mutex->value, 2, timeout, mutex->flags);
        s = atomic_xchg(&mutex->value, 2);

        if (time_point->tv_sec >= 0) {
            xtime_t now = xtime_read(XTIME_CLOCK);
            xtime_t elapsed = now - start;
            start = now;
            timeout -= (long)elapsed;
            if (timeout <= 0 || elapsed >= LONG_MAX)
                return thrd_timedout;
        }
    }
    return 0;
}

/* locks a mutex or returns without blocking if already locked */
int mtx_trylock(mtx_t *mutex)
{
    int i, s;
    /* spin and try to take lock */
    for (i = 0; i < 100; i++) {
        s = atomic_cmpxchg(&mutex->value, 0, 1);
        if (s)
            return 0;
        atomic_break();
    }
    return thrd_busy;
}

/* unlocks a mutex */
int mtx_unlock(mtx_t *mutex)
{
    int i, s;
    /* unlock, and exit if not contended */
    if (mutex->value == 2)
        mutex->value = 0;
    else if (atomic_xchg(&mutex->value, 0) == 1)
        return 0;

    /* spin and hope someone takes the lock */
    for (i = 0; i < 200; i++) {
        if (mutex->value) {
            /* need to set to contended state in case there is waiters */
            s = atomic_cmpxchg(&mutex->value, 1, 2);
            if (s)
                return 0;
        }
        atomic_break();
    }

    /* we need to wake someone up */
    futex_wake((int *)&mutex->value, 1, 0);
    return 0;
}

/* destroys a mutex */
void mtx_destroy(mtx_t *mutex)
{
    (void)mutex;
}

