/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015  <Fabien Bavent>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   - - - - - - - - - - - - - - -
 */
#include <kernel/core.h>
#include <kernel/cpu.h>
#include <kernel/device.h>
#include <errno.h>


#define PORT_COM1 0x3f8   /* COM1 */
#define PORT_COM2 0x2f8   /* COM2 */
#define PORT_COM3 0x3e8   /* COM3 */
#define PORT_COM4 0x2e8   /* COM4 */

int serial_ports[] = { PORT_COM1, PORT_COM2, PORT_COM3, PORT_COM4 };
inode_t *serial_inos[] = { NULL, NULL, NULL, NULL };

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

static void com_init(int port)
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

static int com_read(int port, inode_t *ino)
{
    char byte;
    if ((inb(port + 5) & 1) != 0) {
        while ((inb(port + 5) & 1) != 0) {
            byte = inb(port);
            // chardev_push(ino, &byte, 1);
        }
        return 0;
    }
    return -1;
}

int com_output(int no, char *buf, int len)
{
    int port = serial_ports[no];
    for (; len-- > 0; buf++) {
        // Wait transmition is clear
        while ((inb(port + 5) & 0x20) == 0);
        outb(port, *buf);
    }
    errno = 0;
    return 0;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void com_early_init()
{
    int i;
    for (i = 0; i < 4; ++i)
        com_init(serial_ports[i]);
}


void com_irq(int o)
{
    com_read(serial_ports[o], serial_inos[o]);
    com_read(serial_ports[o + 2], serial_inos[o + 2]);
}

int com_write(inode_t *ino, char *buf, int len)
{
    return com_output(ino->no, buf, len);
}

int com_ioctl(inode_t *ino)
{
    errno = ENOSYS;
    return -1;
}


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

chardev_t com_devs[] = {
    {
        .dev = {
            .ioctl = (dev_ioctl)com_ioctl,
        },
        .dname = "COM1",
        .class = "Serial port",
        .write = (chr_write)com_write,
    },
    {
        .dev = {
            .ioctl = (dev_ioctl)com_ioctl,
        },
        .dname = "COM2",
        .class = "Serial port",
        .write = (chr_write)com_write,
    },
    {
        .dev = {
            .ioctl = (dev_ioctl)com_ioctl,
        },
        .dname = "COM3",
        .class = "Serial port",
        .write = (chr_write)com_write,
    },
    {
        .dev = {
            .ioctl = (dev_ioctl)com_ioctl,
        },
        .dname = "COM4",
        .class = "Serial port",
        .write = (chr_write)com_write,
    },
};


void com_setup()
{
    int i;
    char name[8];
    for (i = 0; i < 4; ++i) {
        snprintf(name, 8, "com%d", i + 1);
        inode_t *ino = vfs_inode(i, S_IFCHR | 0700, NULL, 0);
        serial_inos[i] = ino;
        vfs_mkdev(name, &com_devs[i].dev, ino);
    }
    irq_register(3, (irq_handler_t)com_irq, (void *)1);
    irq_register(4, (irq_handler_t)com_irq, (void *)0);
}

void com_teardown()
{
    int i;
    irq_unregister(3, (irq_handler_t)com_irq, (void *)1);
    irq_unregister(4, (irq_handler_t)com_irq, (void *)0);
    for (i = 0; i < 4; ++i)
        vfs_close(serial_inos[i]);
}

MODULE(serial, com_setup, com_teardown);

