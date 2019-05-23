#include <kernel/futex.h>
#include <kernel/task.h>
#include <kora/splock.h>
#include <kora/bbtree.h>
#include <string.h>
#include <time.h>

typedef struct ftx ftx_t;
typedef struct adv adv_t;

struct ftx {
    splock_t lock;
    llhead_t queue;
    bbnode_t bnode;
    int *pointer;
    int rcu;
    int flags;
};

struct adv {
    ftx_t *futex;
    task_t *task;
    llnode_t node;
    llnode_t tnode;
    clock_t until;
};

splock_t futex_lock;
bbtree_t futex_tree;
llhead_t futex_list;

static ftx_t *futex_open(int *addr, int flags)
{
    size_t phys = (size_t)addr; // TODO resolve physical address
    splock_lock(&futex_lock);
    ftx_t *futex = bbtree_search_eq(&futex_tree, phys, ftx_t, bnode);
    if (futex == NULL && (flags & FUTEX_CREATE)) {
        futex = (ftx_t *)kalloc(sizeof(ftx_t));
        futex->flags = flags;
        if (flags & FUTEX_SHARED) {
            // futex->pointer = ADDR_OFF(kmap(PAGE_SIZE, NULL, ALIGN_DOWN(phys), VMA_PHYSIQ), phys & (PAGE_SIZE-1));
        } else
            futex->pointer = addr;
        futex->bnode.value_ = phys;
        bbtree_insert(&futex_tree, & futex->bnode);
        splock_init(&futex->lock);
        llist_init(&futex->queue);
    }
    if (futex != NULL)
        futex->rcu++;
    splock_unlock(&futex_lock);
    return futex;
}

static void futex_close(ftx_t *futex)
{
    splock_lock(&futex_lock);
    if (--futex->rcu != 0) {
        splock_unlock(&futex->lock);
        splock_unlock(&futex_lock);
        return;
    }

    assert(futex->queue.count_ == 0);
    bbtree_remove(&futex_tree, futex->bnode.value_);
    if (futex->flags & FUTEX_SHARED) {
        // kunmap(PAGE_SIZE, ALIGN_DOWN(futex->pointer, PAGE_SIZE));
    }

    splock_unlock(&futex->lock);
    kfree(futex);
    splock_unlock(&futex_lock);
}

int futex_wait(int *addr, int val, long timeout, int flags)
{
    ftx_t *futex = futex_open(addr, flags | FUTEX_CREATE);
    adv_t advent;
    memset(&advent, 0, sizeof(advent));
    advent.futex = futex;
    advent.task = kCPU.running;
    advent.task->advent = &advent;
    if (timeout > 0) {
        advent.until = clock_read(CLOCK_MONOTONIC);
        splock_lock(&futex_lock);
        ll_append(&futex_list, &advent.tnode);
        splock_unlock(&futex_lock);
    }
    splock_lock(&futex->lock);
    if (*futex->pointer == val) {
        ll_enqueue(&futex->queue, &advent.node);
        splock_unlock(&futex->lock);
        scheduler_switch(TS_BLOCKED, 0);
        BARRIER;
        futex = advent.futex;
        splock_lock(&futex->lock);
    }

    assert(advent.task->advent == NULL);
    assert(advent.node.next_ == NULL && advent.node.prev_ == NULL);
    futex_close(futex);
    return 0;
}

int futex_requeue(int *addr, int val, int val2, int *addr2, int flags)
{
    ftx_t *origin = futex_open(addr, 0);
    if (origin == NULL)
        return 0;
    splock_lock(&origin->lock);
    adv_t *it = ll_first(&origin->queue, adv_t, node);
    while (it && val-- > 0) {
        adv_t *advent = it;
        it = ll_next(&it->node, adv_t, node);
        /* wake up the thread */
        if (advent->until != 0) {
            splock_lock(&futex_lock);
            ll_remove(&futex_list, &advent->tnode);
            splock_unlock(&futex_lock);
        }
        assert(advent->futex == origin);
        ll_remove(&origin->queue, &advent->node);
        scheduler_add(advent->task);
        advent->task->advent = NULL;
    }

    if (val2 == 0 || addr2 == NULL) {
        futex_close(origin);
        return 0;
    }

    ftx_t *target = futex_open(addr2, flags | FUTEX_CREATE);
    splock_lock(&target->lock);
    while (it && val2-- > 0) {
        adv_t *advent = it;
        it = ll_next(&it->node, adv_t, node);
        /* move to next futex */
        assert(advent->futex == origin);
        ll_remove(&origin->queue, &advent->node);
        ll_enqueue(&target->queue, &advent->node);
        origin->rcu--;
        target->rcu++;
        advent->futex = target;
    }

    futex_close(target);
    futex_close(origin);
    return 0;
}

int futex_wake(int *addr, int val)
{
    return futex_requeue(addr, val, 0, NULL, 0);
}

tick_t futex_tick()
{
    tick_t now = clock_read(CLOCK_MONOTONIC);
    tick_t next = now + MIN_TO_USEC(30);
    splock_lock(&futex_lock);

    adv_t *it = ll_first(&futex_list, adv_t, tnode);
    while (it) {
        adv_t *advent = it;
        it = ll_next(&it->tnode, adv_t, tnode);
        if (advent->until == 0)
            continue;
        if (advent->until < now) {
            /* wake up the thread */
            ftx_t *futex = advent->futex;
            splock_lock(&futex->lock);
            ll_remove(&futex->queue, &advent->node);
            ll_remove(&futex_list, &advent->tnode);
            scheduler_add(advent->task);
            advent->task->advent = NULL;
            splock_unlock(&futex->lock);
        } else if (advent->until < next)
            next = advent->until;
    }
    splock_unlock(&futex_lock);
    return next;
}

void futex_init()
{
    splock_init(&futex_lock);
    bbtree_init(&futex_tree);
    llist_init(&futex_list);
}

