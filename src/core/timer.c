#include <kernel/futex.h>
#include <kernel/task.h>
#include <kernel/files.h>
#include <kernel/input.h>
#include <kora/splock.h>
#include <string.h>
#include <time.h>

splock_t itimer_lock;
llhead_t itimer_list;


static void itimer_wake_advent(advent_t *advent)
{
    advent->until += advent->interval;
    evmsg_t msg = {0};
    msg.message = EV_TIMER;
    pipe_write(advent->pipe, (char *)&msg, sizeof(msg), IO_ATOMIC | IO_NO_BLOCK);
}

static void itimer_dtor_advent(advent_t *advent)
{
    kfree(advent);
}


static void sleep_wake_advent(advent_t *advent)
{
    /* wake up the thread */
    ll_remove(&itimer_list, &advent->tnode);
    scheduler_add(advent->task);
    ll_remove(&advent->task->alist, &advent->anode);
    // advent->task->advent = NULL;
}

static void sleep_dtor_advent(advent_t *advent)
{
    (void)advent;
}


void itimer_create(pipe_t *pipe, long delay, long interval)
{
    advent_t *advent = kalloc(sizeof(advent_t));
    advent->task = kCPU.running;
    advent->pipe = pipe;
    advent->dtor = itimer_dtor_advent;
    ll_append(&advent->task->alist, &advent->anode);

    advent->until = clock_read(CLOCK_MONOTONIC) + delay;
    advent->interval = interval;
    splock_lock(&itimer_lock);
    ll_append(&itimer_list, &advent->tnode);
    splock_unlock(&itimer_lock);
}

void sleep_timer(long timeout)
{
    advent_t advent;
    memset(&advent, 0, sizeof(advent));
    advent.task = kCPU.running;
    advent.dtor = sleep_dtor_advent;
    ll_append(&advent.task->alist, &advent.anode);
    // advent.task->advent = &advent;
    advent.until = clock_read(CLOCK_MONOTONIC) + MAX(1, timeout);
    splock_lock(&itimer_lock);
    ll_append(&itimer_list, &advent.tnode);
    splock_unlock(&itimer_lock);

    scheduler_switch(TS_BLOCKED, 0);
}


tick_t itimer_tick()
{
    tick_t now = clock_read(CLOCK_MONOTONIC);
    tick_t next = now + MIN_TO_USEC(30);
    splock_lock(&itimer_lock);

    advent_t *it = ll_first(&itimer_list, advent_t, tnode);
    while (it) {
        advent_t *advent = it;
        it = ll_next(&it->tnode, advent_t, tnode);
        if (advent->until == 0)
            continue;
        if (advent->until < now) {
            if (advent->pipe)
                itimer_wake_advent(advent);
            else
                sleep_wake_advent(advent);

        } else if (advent->until < next)
            next = advent->until;
    }
    splock_unlock(&itimer_lock);
    return next;
}

void itimer_init()
{
    splock_init(&itimer_lock);
    llist_init(&itimer_list);
}

