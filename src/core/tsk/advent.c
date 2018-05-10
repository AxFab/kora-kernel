#include <kernel/core.h>
#include <kernel/task.h>
#include <kora/llist.h>

typedef struct advent advent_t;

struct advent {
    void *listener;
    time64_t timeout;
    llnode_t t_node;
    llnode_t l_node;
    task_t *task;
};

llhead_t wlist;
splock_t wlock;
time64_t wtime = INT64_MAX;

void task_wait(void *listener, long timeout_us)
{
    assert(kCPU.running != NULL);
    advent_t *advent = (advent_t*)kalloc(sizeof(advent_t));
    advent->listener = listener;
    advent->task = kCPU.running;
    if (timeout_us > 0)
        advent->timeout = time64() + timeout_us;
    else
        advent->timeout = INT64_MAX;

    irq_disable();
    splock_lock(&wlock);
    ll_append(&wlist, &advent->t_node);
    if (wtime > advent->timeout)
        wtime = advent->timeout;
    splock_unlock(&wlock);
    asm("int $0x41"); // Will put task into sleep mode and return here after!
}


void task_wakeup_timeout()
{
    time64_t now = time64();
    if (time64() < wtime)
        return;
    if (!splock_trylock(&wlock))
        return;

    advent_t *advent;
    time64_t next = INT64_MAX;
    for ll_each (&wlist, advent, advent_t, t_node) {
        if (advent->timeout < now) {
            /* Wakup task and mark as timeout */
            ll_remove(&wlist, &advent->t_node);
            task_resume(advent->task);
            kfree(advent);
        } else if (advent->timeout < next) {
            next = advent->timeout;
        }
    }

    wtime = next;
    splock_unlock(&wlock);
}
