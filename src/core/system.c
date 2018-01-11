#include <kernel/core.h>
#include <kernel/cpu.h>
#include <skc/llist.h>

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


void irq_enable()
{
}

void irq_disable()
{
}

void irq_register(int no, irq_handler_t func, void* data)
{
  if (no < 0 || no >= 16) {
    return;
  }
  irq_record_t *record = (irq_record_t*)kalloc(sizeof(irq_record_t));
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
  kprintf(-1, "Received IRQ%d\n", no);
  if (no < 0 || no >= 16) {
    return;
  }
  irq_record_t *record;
  for ll_each(&irqv[no].list, record, irq_record_t, node) {
    if (record->func(record->data) == 0) {
      return;
    }
  }
}



long sys_call(void *params)
{
  // CURRENT->stack = params;
  //
  // params[11] = ret;
  // params[9] = errno;
  return -1;
}

long sys_wait(void *params)
{
  // CURRENT->stack = params;
  return -1;
}


