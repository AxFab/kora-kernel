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

/* Create a client socket from a listening one */
static socket_t *net_socket_from(socket_t *model, skb_t *skb)
{
    assert(model != NULL && model->stack != NULL && model->proto != NULL);
    assert(skb != NULL);

    if (model->proto->accept == NULL)
        return NULL;

    // Allocate the socket
    socket_t *sock = kalloc(sizeof(socket_t));
    sock->stack = model->stack;
    sock->proto = model->proto;
    sock->protocol = model->protocol;
    mtx_init(&sock->lock, mtx_plain);
    sem_init(&sock->rsem, 0);
    llist_init(&sock->lskb);

    // Check with protocol
    if (model->proto->accept(sock, model, skb) != 0) {
        kfree(sock);
        return NULL;
    }

    return sock;
}

 /* Create an endpoint for communication */
socket_t *net_socket(netstack_t *stack, int protocol, int method)
{
    assert(stack != NULL);
    nproto_t *proto = net_protocol(stack, protocol);
    if (proto == NULL || proto->send == NULL || proto->recv == NULL)
        return NULL; // No such protocol or is not socket capable

    // Allocate the socket
    socket_t *sock = kalloc(sizeof(socket_t));
    sock->stack = stack;
    sock->proto = proto;
    sock->protocol = protocol;
    mtx_init(&sock->lock, mtx_plain);
    sem_init(&sock->rsem, 0);
    llist_init(&sock->lskb);

    // Check protocol is accepting
    if (proto->socket && proto->socket(sock, method) != 0) {
        kfree(sock);
        return NULL;
    }

    return sock;
}

/* Close a socket endpoint */
int net_socket_close(socket_t *sock)
{
    int ret = 0;
    mtx_lock(&sock->lock);
    if (sock->proto->close)
        ret = sock->proto->close(sock);
    mtx_unlock(&sock->lock);
    if (sock->lskb.count_ > 0)
        return -1;
    if (ret == 0)
        kfree(sock);
    return ret;
}

/* Bind a name to a socket - set its local address */
int net_socket_bind(socket_t *sock, uint8_t *addr, size_t len)
{
    if (sock->flags & NET_ADDR_BINDED)
        return -1;
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

/* Connect a socket with a single address - set its remote address */
int net_socket_connect(socket_t *sock, uint8_t *addr, size_t len)
{
    if (sock->flags & NET_ADDR_CONNECTED)
        return -1;
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

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

/* Wrapper to preapre socket request and ask protocol for data exchange */
static int net_socket_xchg(socket_t *sock, const netmsg_t *msg, int flags, long (*xchg)(socket_t *, uint8_t *, char *, size_t, int))
{
    uint8_t addr[NET_MAX_HWADRLEN];
    if (msg == NULL || msg->iolven > IOVLEN_MAX)
        return -1;

    mtx_lock(&sock->lock);

    // Look for the address
    if (sock->flags & NET_ADDR_CONNECTED)
        memcpy(addr, sock->raddr, sock->proto->addrlen);
    else if (sock->proto->addrlen != msg->addrlen)
        memcpy(addr, msg->addr, sock->proto->addrlen);
    else if (xchg == (void *)sock->proto->send) {
        mtx_unlock(&sock->lock);
        return -1;
    }

    long bytes = 0;
    for (unsigned i = 0; i < msg->iolven; ++i) {
        const iovec_t *iov = &msg->iov[i];
        long ret = xchg(sock, addr, iov->buf, iov->len, flags);
        if (ret < 0) {
            mtx_unlock(&sock->lock);
            return -1;
        }
        bytes += ret; // iov->len;
    }

    if (xchg == sock->proto->recv && msg->addrlen == sock->proto->addrlen)
        memcpy((uint8_t *)msg->addr, addr, sock->proto->addrlen);

    mtx_unlock(&sock->lock);
    return bytes;
}

/* Sending a series of data to an address */
long net_socket_send(socket_t *sock, const netmsg_t *msg, int flags)
{
    assert(sock != NULL && sock->proto != NULL);
    assert(sock->proto->send != NULL);
    return net_socket_xchg(sock, msg, flags, (void *)sock->proto->send);
}

/* Receiving a series of data from an address */
long net_socket_recv(socket_t *sock, netmsg_t *msg, int flags)
{
    assert(sock != NULL && sock->proto != NULL);
    assert(sock->proto->recv != NULL);
    return net_socket_xchg(sock, msg, flags, sock->proto->recv);
}

/* Writing data to a socket */
long net_socket_write(socket_t *sock, const char *buf, size_t len, int flags)
{
    netmsg_t *msg = kalloc(sizeof(netmsg_t) + sizeof(iovec_t));
    msg->iolven = 1;
    msg->iov[0].buf = (char *)buf;
    msg->iov[0].len = len;
    long bytes = net_socket_send(sock, msg, flags);
    kfree(msg);
    return bytes;
}

/* Reading data from a socket */
long net_socket_read(socket_t *sock, char *buf, size_t len, int flags)
{
    netmsg_t *msg = kalloc(sizeof(netmsg_t) + sizeof(iovec_t));
    msg->iolven = 1;
    msg->iov[0].buf = buf;
    msg->iov[0].len = len;
    long bytes = net_socket_recv(sock, msg, flags);
    kfree(msg);
    return bytes;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

/* Turn the socket into listening mode and look for a client socket */
socket_t *net_socket_accept(socket_t *sock, xtime_t timeout)
{
    // TODO -- Check the socket is elligble
    if ((sock->flags & NET_ADDR_BINDED) == 0)
        return NULL;

    do {
        // Listen for incoming packet
        sock->flags |= NET_SOCK_LISTING;
        skb_t *skb = net_socket_pull(sock, NULL, 0, timeout);
        if (skb == NULL)
            continue;

        // Create client socket
        socket_t *client = net_socket_from(sock, skb);
        if (client != NULL) {
            net_socket_push(client, skb);
            return client;
        }

    } while (timeout < 0);
    return NULL;
}

/* Wait for an incoming packet on this socket */
skb_t *net_socket_pull(socket_t *sock, uint8_t *addr, int length, xtime_t timeout)
{
    struct timespec xt = { .tv_sec = timeout / 1000000LL, .tv_nsec = (timeout % 1000000LL) * 1000 };
    if (timeout < 0)
        sem_acquire(&sock->rsem);
    else if (timeout == 0 && sem_tryacquire(&sock->rsem) == thrd_busy)
        return NULL;
    else if (timeout > 0 && sem_timedacquire(&sock->rsem, &xt) == thrd_busy)
        return NULL;

    splock_lock(&sock->rlock);
    skb_t *skb = ll_dequeue(&sock->lskb, skb_t, node);
    splock_unlock(&sock->rlock);
    return skb;
}

/* Signal an incoming packet on this socket */
int net_socket_push(socket_t *sock, skb_t *skb)
{
    splock_lock(&sock->rlock);
    ll_enqueue(&sock->lskb, &skb->node);
    splock_unlock(&sock->rlock);
    sem_release(&sock->rsem);
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
EXPORT_SYMBOL(net_socket_accept, 0);
EXPORT_SYMBOL(net_socket_pull, 0);
EXPORT_SYMBOL(net_socket_push, 0);
