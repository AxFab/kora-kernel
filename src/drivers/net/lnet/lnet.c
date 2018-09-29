/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2018  <Fabien Bavent>
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
#include <kernel/net.h>
#include <kora/socket.h>

void lnet_link(lnet_dev_t *ifnet)
{
    msg_t msg;
    msg.request = MR_REQUEST;
    msg.length = 0;
    send(ifnet->fd, &msg, sizeof(msg));
}

void lnet_send(lnet_dev_t *ifnet, skb_t *skb)
{
    msg_t msg;
    msg.request = MR_PACKET;
    msg.length = skb->length;
    send(ifnet->fd, &msg, sizeof(msg));
    send(ifnet->fd, skb->data, skb->length);
}

void lnet_idle(lnet_dev_t *ifnet)
{
    msg_t msg;
    char *frame = kalloc(ifnet->n.mtu);
    while (ifnet->n.is_connected) {
        recv(ifnet->fd, &msg, sizeof(msg));
        if (msg.length != 0)
            recv(ifnet->fd, frame, msg.length);
        switch (msg.request) {
        case MR_REGISTERED:
            ifnet->n.flags |= NET_UP;
            break;
        case MR_PACKET:
            net_recv(&ifnet.n, frame, msg.length);
            break;
        }
    }
    kfree(frame);
}

void lnet_setup()
{
    char addr[10];
    net_address_ipv4_port(addr, "192.168.0.1", 8080);
    int fd = open_socket(NPC_IPv4_TCP, NAD_IPv4, addr);
    if (fd == 0)
        return;

    lnet_dev_t *ifnet = net_device();
    ifnet->n.link = link;
    ifnet->n.send = send;
    ifnet->n.is_connected = true;
    thread_start(net_idle, fd);
}

void lnet_teardown()
{
    ifnet->n.is_connected = false;
    close(ifnet->fd);
}

