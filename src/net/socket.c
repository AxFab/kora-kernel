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

//
//struct socket {
//    int protocol; // Ip4_Tcp / Ip4_Udp / Ip4 / Lo_Seq / Lo_Dram
//    int addrlen;
//    uint8_t local_addr[16];
//    uint8_t remote_addr[16];
//}

// Create an endpoint for communication
socket_t* net_socket(netstack_t* stack, int protocol)
{
    nproto_t* proto = bbtree_search_eq(&stack->protocols, protocol, nproto_t, bnode);
    if (proto == NULL || proto->send == NULL || proto->recv == NULL)
        return NULL; // No such protocol or is not socket capable

    socket_t *sock = kalloc(sizeof(socket_t));
    sock->stack = stack;
    sock->proto = proto;

    mtx_init(&sock->lock, mtx_plain);
    cnd_init(&sock->incm);
    llist_init(&sock->lskb);
    return sock;
}

int net_socket_close(socket_t* sock)
{
    int ret = 0;
    mtx_lock(&sock->lock);
    if (sock->proto->close)
        ret = sock->proto->close(sock);

    mtx_unlock(&sock->lock);
    if (ret == 0) {
        kfree(sock);
        // free all
    }

    return ret;
}

// Bind a name to a socket
int net_socket_bind(socket_t* sock, uint8_t* addr, size_t len)
{
    if (sock->proto->addrlen != len)
        return -1;
    mtx_lock(&sock->lock);
    if (sock->proto->bind && sock->proto->bind(sock, addr, len) != 0) {
        mtx_unlock(&sock->lock);
        return -1;
    }

    memcpy(sock->laddr, addr, len);
    sock->flags |= NET_ADDR_BINDED;
    mtx_unlock(&sock->lock);
    return 0;
}

// Connect a socket with a single address.
int net_socket_connect(socket_t* sock, uint8_t* addr, size_t len)
{
    if (sock->proto->addrlen != len)
        return -1;
    mtx_lock(&sock->lock);
    if (sock->proto->connect && sock->proto->connect(sock, addr, len) != 0) {
        mtx_unlock(&sock->lock);
        return -1;
    }

    memcpy(sock->raddr, addr, len);
    sock->flags |= NET_ADDR_CONNECTED;
    mtx_unlock(&sock->lock);
    return 0;
}


long net_socket_send(socket_t* sock, const netmsg_t* msg, int flags)
{
    uint8_t addr[32];

    if (msg->iolven > NET_IOVLEN_MAX)
        return -1;

    mtx_lock(&sock->lock);

    if (sock->flags & NET_ADDR_CONNECTED)
        memcpy(addr, sock->raddr, sock->proto->addrlen);
    else if (msg->addrlen == sock->proto->addrlen)
        memcpy(addr, msg->addr, sock->proto->addrlen);
    else {
        mtx_unlock(&sock->lock);
        return -1;
    }

    assert(sock->proto->send != NULL);
    long bytes = 0;
    for (unsigned i = 0; i < msg->iolven; ++i) {
        const iovec_t* iov = &msg->iov[i];
        long ret = sock->proto->send(sock, addr, iov->buf, iov->len, flags);
        if (ret < 0) {
            mtx_unlock(&sock->lock);
            return -1;
        }
        bytes += ret;
    }

    mtx_unlock(&sock->lock);
    return bytes;
}

long net_socket_recv(socket_t* sock, const netmsg_t* msg, int flags)
{
    unsigned addrlen = msg->addrlen;
    uint8_t addr[16];

    if (msg->iolven > NET_IOVLEN_MAX)
        return -1;

    mtx_lock(&sock->lock);

    if (sock->flags & NET_ADDR_BINDED) {
        memcpy(addr, sock->laddr, sock->proto->addrlen);
        addrlen = sock->proto->addrlen;
    }
    else
        memcpy(addr, msg->addr, sock->proto->addrlen);

    if (sock->proto->addrlen != addrlen) {
        mtx_unlock(&sock->lock);
        return -1;
    }

    assert(sock->proto->recv != NULL);
    long bytes = 0;
    for (unsigned i = 0; i < msg->iolven; ++i) {
        const iovec_t* iov = &msg->iov[i];
        long ret = sock->proto->recv(sock, addr, iov->buf, iov->len, flags);
        if (ret < 0) {
            mtx_unlock(&sock->lock);
            return -1;
        }
        bytes += ret;
    }

    mtx_unlock(&sock->lock);
    return bytes;
}

long net_socket_write(socket_t* sock, const char *buf, size_t len, int flags)
{
    netmsg_t* msg = kalloc(sizeof(netmsg_t) + sizeof(iovec_t));
    msg->iolven = 1;
    msg->iov[0].buf = buf;
    msg->iov[0].len = len;
    return net_socket_send(sock, msg, flags);
}

long net_socket_read(socket_t* sock, char *buf, size_t len, int flags)
{
    netmsg_t* msg = kalloc(sizeof(netmsg_t) + sizeof(iovec_t));
    msg->iolven = 1;
    msg->iov[0].buf = buf;
    msg->iov[0].len = len;
    return net_socket_recv(sock, msg, flags);
}




socket_t *net_socket_accept(socket_t *sock, bool block)
{
    mtx_lock(&sock->lock);

    mtx_unlock(&sock->lock);
    return NULL;
}


int net_socket_push(socket_t* sock, skb_t *skb, int length)
{
    mtx_lock(&sock->lock);
    ll_append(&sock->lskb, &skb->node);
    cnd_broadcast(&sock->incm);
    mtx_unlock(&sock->lock);
    return 0;
}

EXPORT_SYMBOL(net_socket, 0);
EXPORT_SYMBOL(net_socket_close, 0);
EXPORT_SYMBOL(net_socket_bind, 0);
EXPORT_SYMBOL(net_socket_connect, 0);
EXPORT_SYMBOL(net_socket_send, 0);
EXPORT_SYMBOL(net_socket_recv, 0);
EXPORT_SYMBOL(net_socket_write, 0);
EXPORT_SYMBOL(net_socket_read, 0);


