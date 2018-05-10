#include <kernel/core.h>
#include <kora/mcrs.h>

#define MICROSEC_IN_SEC (1000 * 1000)
#define PIT_CH0   0x40  /**< Channel 0 data port (read/write) */
#define PIT_CH1   0x41  /**< Channel 1 data port (read/write) */
#define PIT_CH2   0x42  /**< Channel 2 data port (read/write) */
#define PIT_CMD   0x43  /**< Mode/Command register (write only, a read is ignored) */


unsigned PIT_period = 0;
unsigned PIT_frequency = 0;

void PIT_set_interval(unsigned frequency)
{

    unsigned divisor = 1193180 / frequency; /* Calculate our divisor */
    divisor = MIN (65536, MAX(1, divisor));

    PIT_frequency = 1193180 / divisor;
    PIT_period = MICROSEC_IN_SEC / PIT_frequency;

    outb(PIT_CMD, 0x36);             /* Set our command byte 0x36 */
    outb(PIT_CH0, divisor & 0xff);   /* Set low byte of divisor */
    outb(PIT_CH0, (divisor >> 8) & 0xff);     /* Set high byte of divisor */

    kprintf (KLOG_MSG, "Set cpu ticks frequency: %d Hz\n", PIT_frequency);
}


