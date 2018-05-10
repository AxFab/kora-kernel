#include <assert.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <kernel/core.h>

bool irq_enable();
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

bool irq_active = false;
unsigned irq_sem = 0;

void irq_reset(bool enable)
{
    irq_active = true;
    irq_sem = 1;
    asm("cli");
    if (enable) {
        irq_enable();
    }
}

bool irq_enable()
{
    if (irq_active) {
        assert(irq_sem > 0);
        if (--irq_sem == 0) {
            asm("sti");
            return true;
        }
    }
    return false;
}

void irq_disable()
{
    if (irq_active) {
        asm("cli");
        ++irq_sem;
    }
}

#define PIC1_CMD 0x20
#define PIC2_CMD 0xA0
#define PIC_EOI 0x20

void irq_ack(int no)
{
    if (no >= 8)
        outb(PIC2_CMD, PIC_EOI);
    outb(PIC1_CMD, PIC_EOI);
}


