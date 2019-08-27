#ifndef __KERNEL_RESX__SYNC_H
#define __KERNEL_RESX__SYNC_H 1

#include <stddef.h>


utime_t futex_tick();
int futex_requeue(int *addr, int val, int val2, int *addr2, int flags);
int futex_wait(int *addr, int val, long timeout, int flags);
int futex_wake(int *addr, int val);
void futex_init();

void itimer_create(pipe_t *pipe, long delay, long interval);

int async_wait(splock_t *lock, emitter_t *emitter, long timeout_us);
int async_wait_rd(rwlock_t *lock, emitter_t *emitter, long timeout_us);
int async_wait_wr(rwlock_t *lock, emitter_t *emitter, long timeout_us);
void async_cancel(task_t *task);
void async_raise(emitter_t *emitter, int err);
void async_timesup();

void sleep_timer(long timeout);



#endif  /* __KERNEL_RESX__SYNC_H */
