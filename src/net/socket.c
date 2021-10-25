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
#include <kernel/net.h>
#include <kernel/stdc.h>

socket_t *net_socket(netstack_t *net, int protocol)
{
    socket_t *socket = kalloc(sizeof(socket_t));
    socket->net = net;
    socket->protocol = NULL;
    if (socket->protocol == NULL) {
        kfree(socket);
        return NULL;
    }

    // TODO - if (protocol == NET_AF_TCP)
    //    ret = tcp_socket(socket);
    //else if (protocol == NET_AF_UDP)
    //    ret = udp_socket(socket);

    return socket;
}

int net_socket_bind(socket_t *sock, uint8_t *addr, size_t len)
{
    if (len > 32)
        return -1;

    memset(sock->laddr, 0, 32);
    if (sock->protocol->bind(sock, addr, len) != 0)
        return -1;

    memcpy(sock->laddr, addr, len);
    return 0;
}

int net_socket_connect(socket_t *sock, uint8_t *addr, size_t len)
{
    if (len > 32)
        return -1;

    memset(sock->raddr, 0, 32);
    if (sock->protocol->connect(sock, addr, len) != 0)
        return -1;

    memcpy(sock->raddr, addr, len);
    return 0;
}

int net_socket_close(socket_t *sock)
{
    return -1;
}

socket_t *net_socket_accept(socket_t *sock, bool block)
{
    return NULL;
}

int net_socket_write(socket_t *sock, const char *buf, size_t len)
{
    return -1;
}

int net_socket_read(socket_t *sock, const char *buf, size_t len)
{
    return -1;
}

int net_socket_recv(socket_t *socket, skb_t *skb, int length)
{
    return -1;
}


EXPORT_SYMBOL(net_socket_recv, 0);

