#include <kernel/core.h>
#include <kernel/cpu.h>
#include <kora/llist.h>
#include <assert.h>

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
typedef struct irq_record irq_record_t;
struct irq_vector {
    llhead_t list;
} irqv[16];

struct irq_record {
    llnode_t node;
    irq_handler_t func;
    void *data;
};

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void irq_register(int no, irq_handler_t func, void *data)
{
    if (no < 0 || no >= 16) {
        return;
    }
    kprintf(KLOG_IRQ, "Register IRQ%d <%08x(%08x)> \n", no, func, data);
    irq_record_t *record = (irq_record_t *)kalloc(sizeof(irq_record_t));
    record->func = func;
    record->data = data;
    ll_append(&irqv[no].list, &record->node);
}

void irq_unregister(int no, irq_handler_t func, void *data)
{
    if (no < 0 || no >= 16) {
        return;
    }
    irq_record_t *record;
    for ll_each(&irqv[no].list, record, irq_record_t, node) {
        if (record->func == func || record->data == data) {
            ll_remove(&irqv[no].list, &record->node);
            kfree(record);
            return;
        }
    }
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void sys_irq(int no)
{
    // irq_disable();
    assert(no >= 0 && no < 16);
    irq_record_t *record;
    if (irqv[no].list.count_ == 0) {
        irq_ack(no);
        kprintf(KLOG_IRQ, "Received IRQ%d, no handlers.\n", no);
        return;
    }
    for ll_each(&irqv[no].list, record, irq_record_t, node) {
        record->func(record->data);
    }
    irq_ack(no);
    // irq_enable();
}

// #define HZ 100
// #define TICKS_PER_SEC 10000 /* 100 µs */
// int timer_cpu = 0;
// splock_t xtime_lock;
// uint64_t jiffies = 0;
// uint64_t ticks = 0;
// uint64_t ticks_last = 0;
// uint64_t ticks_elapsed = 0;

// void scheduler_ticks();

// void ticks_init()
// {
//     splock_init(&xtime_lock);
//     cpu_elapsed(&ticks_last);
// }

// void sys_ticks()
// {
//     if (timer_cpu == cpu_no()) {
//         splock_lock(&xtime_lock);
//         ticks += TICKS_PER_SEC / HZ;
//         ticks_elapsed += cpu_elapsed(&ticks_last);
//         jiffies++;
//         // Update Wall time
//         // Compute global load
//         splock_unlock(&xtime_lock);
//     }

//     // seat_ticks();
//     scheduler_ticks();
// }

