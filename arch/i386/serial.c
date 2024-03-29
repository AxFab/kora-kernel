/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2021  <Fabien Bavent>
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
#include <kernel/arch.h>
#include <kernel/vfs.h>
#include <kernel/irq.h>
#include <errno.h>

#define PORT_COM1 0x3f8   /* COM1 */
#define PORT_COM2 0x2f8   /* COM2 */
#define PORT_COM3 0x3e8   /* COM3 */
#define PORT_COM4 0x2e8   /* COM4 */

int serial_ports[] = { PORT_COM1, PORT_COM2, PORT_COM3, PORT_COM4 };
inode_t *serial_inos[] = { NULL, NULL, NULL, NULL };
const char *devnames[] = { "COM1", "COM2", "COM3", "COM4", };


void serial_init(int no)
{
    int port = serial_ports[no];
    outb(port + 1, 0x00);  // Disable all interrupts
    outb(port + 3, 0x80);  // Enable DLAB (set baud rate divisor)
    outb(port + 0, 0x03);  // Set divisor to 3 (lo byte) 38400 baud
    outb(port + 1, 0x00);  //                  (hi byte)
    // outb(port + 3, 0x03);  // 8 bits, no parity, one stop bit
    outb(port + 3, 0x0A);  // 7 bits, odd parity
    outb(port + 2, 0xC7);  // Enable FIFO, clear them, with 14-byte threshold
    outb(port + 4, 0x0B);  // IRQs enabled, RTS/DSR set
}

void serial_early_init()
{
    int i;
    for (i = 0; i < 4; ++i)
        serial_init(i);
}

int serial_send(int no, const char *buf, size_t len)
{
    int port = serial_ports[no];
    for (; len-- > 0; buf++) {
        // Wait transmition is clear
        while ((inb(port + 5) & 0x20) == 0);
        outb(port, *buf);
    }
    return 0;
}

int serial_recv(int no)
{
    int port = serial_ports[no];
    char byte;
    if ((inb(port + 5) & 1) != 0) {
        while ((inb(port + 5) & 1) != 0) {
            byte = inb(port);
            (void)byte;
            // chardev_push(ino, &byte, 1);
        }
        return 0;
    }
    return -1;
}


void serial_irq(int o)
{
    serial_recv(o);
    serial_recv(o + 1);
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

int serial_write(inode_t *ino, const char *buf, size_t len, xoff_t off, int flags)
{
    return serial_send(ino->no - 1, buf, len);
}

int serial_ioctl(inode_t *ino, int cmd, void **params)
{
    errno = ENOSYS;
    return -1;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

ino_ops_t serial_fops = {
    .write = serial_write,
    .ioctl = serial_ioctl,
};


void serial_setup()
{
    int i;
    char name[8];
    for (i = 0; i < 4; ++i) {
        snprintf(name, 8, "com%d", i + 1);
        inode_t *ino = vfs_inode(i + 1, FL_CHR, NULL, &serial_fops);
        ino->dev->devclass = strdup("Serial port");
        ino->dev->devname = strdup((char *)devnames[i]);
        vfs_mkdev(ino, name);
        serial_inos[i] = ino;
    }
    irq_register(3, (irq_handler_t)serial_irq, (void *)1);
    irq_register(4, (irq_handler_t)serial_irq, (void *)0);
}

void serial_teardown()
{
    int i;
    irq_unregister(3, (irq_handler_t)serial_irq, (void *)1);
    irq_unregister(4, (irq_handler_t)serial_irq, (void *)0);
    for (i = 0; i < 4; ++i)
        vfs_close_inode(serial_inos[i]);
}
