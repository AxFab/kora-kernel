#include <assert.h>
#include <stdbool.h>
#include <bits/cdefs.h>
#include <kora/atomic.h>

bool irq_enable();

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

bool irq_active = false;
unsigned irq_sem = 0;

bool irq_last = false;


void irq_reset(bool enable)
{
    irq_active = true;
    irq_sem = enable ? 1 : 0;
    irq_last = false;
    if (enable) {
        irq_enable();
    }
}

bool irq_enable()
{
    if (irq_active) {
        assert(irq_sem > 0);
        if (--irq_sem == 0) {
            irq_last = true;
            return true;
        }
    }
    return false;
}

void irq_disable()
{
    if (irq_active) {
        irq_last = false;
        ++irq_sem;
    }
}

void irq_ack(int no)
{
}

