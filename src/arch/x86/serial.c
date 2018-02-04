#include <kernel/core.h>
#include <kernel/cpu.h>

#define PORT_COM1 0x3f8   /* COM1 */
#define PORT_COM2 0x2f8   /* COM2 */
#define PORT_COM3 0x3e8   /* COM3 */
#define PORT_COM4 0x2e8   /* COM4 */

int serial_ports[] = { PORT_COM1, PORT_COM2, PORT_COM3, PORT_COM4 };

/*
  On PORT+3
  bits: 0-1 give data length form 5 to 8
        2  number of stop bits (1 or more)
        3-4 Parity bits NONE / ODD / EVENT
        5   -- Parity wierd

  On PORT+5
  bits: 0 -- Data available
        1 -- Transmitter empty
        2 -- Error
        3 -- Status change
*/

// You use IRQ #4 for COM ports 1 or 3, and IRQ #3 for COM ports 2 or 4

static int serial_read_all(int port, void *fifo)
{
    char byte;
    if ((inb(port + 5) & 1) != 0) {
        while ((inb(port + 5) & 1) != 0) {
            byte = inb(port);
            // fifo_write(NULL, &byte, 1); NON-BLOCKING !!!
            ((void)fifo);
            ((void)byte);
        }
        return 0;
    }
    return -1;
}

static void serial_init(int port)
{
    outb(port + 1, 0x00);  // Disable all interrupts
    outb(port + 3, 0x80);  // Enable DLAB (set baud rate divisor)
    outb(port + 0, 0x03);  // Set divisor to 3 (lo byte) 38400 baud
    outb(port + 1, 0x00);  //                  (hi byte)
    // outb(port + 3, 0x03);  // 8 bits, no parity, one stop bit
    outb(port + 3, 0x0A);  // 7 bits, odd parity
    outb(port + 2, 0xC7);  // Enable FIFO, clear them, with 14-byte threshold
    outb(port + 4, 0x0B);  // IRQs enabled, RTS/DSR set
}


char serial_read(int port)
{
    // Wait for a signal to be received
    while ((inb(port + 5) & 1) == 0);
    return inb(port);
}

void serial_write(int port, char a)
{
    // Wait transmition is clear
    while ((inb(port + 5) & 0x20) == 0);
    outb(port, a);
}


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int SRL_irq(int o)
{
    if (serial_read_all(serial_ports[o], NULL) == 0) {
        return 0;
    } else if (serial_read_all(serial_ports[o + 2], NULL) == 0) {
        return 0;
    }
    return -1;
}

int SRL_setup()
{
    int i;
    for (i = 0; i < 4; ++i) {
        serial_init(serial_ports[i]);
    }

    // irq_register(3, (irq_handler_t)SRL_irq, (void*)1);
    // irq_register(4, (irq_handler_t)SRL_irq, (void*)0);
    return 0;
}

int SRL_teardown()
{
    // irq_unregister(3, (irq_handler_t)SRL_irq, (void*)1);
    // irq_unregister(4, (irq_handler_t)SRL_irq, (void*)0);
    return 0;
}


int TXT_ansi_escape(const char *msg);

void SRL_write(const char *msg, size_t lg)
{
    const char *st = msg;
    for (; msg - st < (int)lg; ++msg) {
        if (*msg == '\r' || *msg == '\n' || *msg == '\t') {
            serial_write(PORT_COM1, *msg);
        } else if (*msg == '\e' && msg[1] == '[') {
            msg += TXT_ansi_escape(msg);
        } else if (*msg < 0x20) {
            // Ignore
        } else if (*msg < 0x79) {
            serial_write(PORT_COM1, *msg);
        }
    }
}

