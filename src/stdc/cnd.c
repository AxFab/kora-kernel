#include <threads.h>
#include <limits.h>
#include <stdint.h>
#include <errno.h>
#include <kernel/stdc.h>
#include <kora/mcrs.h>
#include <bits/atomic.h>


/* creates a condition variable */
int cnd_init(cnd_t *cond)
{
    cond->mtx = NULL;
    atomic_store(&cond->seq, 0);
    return 0;
}

/* unblocks one thread blocked on a condition variable */
int cnd_signal(cnd_t *cond)
{
    /* we are waking someone up */
    cond->seq++;
    futex_wake((int *)&cond->seq, 1, 0);
    return 0;
}

/* unblocks all threads blocked on a condition variable */
int cnd_broadcast(cnd_t *cond)
{
    volatile mtx_t *mutex = cond->mtx;
    /* no mutex means no waiters */
    if (mutex == NULL)
        return 0;

    /* we are waking everyone up */
    cond->seq++;
    futex_requeue((int *)&cond->seq, 1, INT_MAX, (int *)&mutex->value, cond->flags);
    return 0;
}

/* blocks on a condition variable */
int cnd_wait(cnd_t *cond, mtx_t *mutex)
{
    struct timespec time_point;
    time_point.tv_sec = -1;
    return cnd_timedwait(cond, mutex, &time_point);
}

/* blocks on a condition variable, with a timeout */
int cnd_timedwait(cnd_t *restrict cond, mtx_t *restrict mutex, const struct timespec *restrict time_point)
{
    long timeout = time_point->tv_sec < 0 ? -1 : TMSPEC_TO_USEC(*time_point);
    int seq = cond->seq;
    if (cond->mtx != mutex) {
        if (cond->mtx)
            return EINVAL;
        /* atomically set mutex inside cond */
        atomic_ptr_cmpxchg(&cond->mtx, NULL, mutex);
        if (cond->mtx != mutex)
            return EINVAL;
    }

    xtime_t start;
    if (time_point->tv_sec >= 0)
        start = xtime_read(XTIME_CLOCK);

    mtx_unlock(mutex);
    futex_wait((int *)&cond->seq, seq, timeout, cond->flags);
    while (atomic_xchg(&mutex->value, 2)) {
        futex_wait((int *)&mutex->value, 2, timeout, mutex->flags);

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


/* destroys a condition variable */
void cnd_destroy(cnd_t *cond)
{
    (void)cond;
}

