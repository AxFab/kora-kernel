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
#include "ip4.h"
#include <assert.h>

struct ip4_port
{
    bbnode_t bnode;
    uint16_t port;
    socket_t *sock;
    uint8_t bind[IP4_ALEN];
    bool binded;
    bool listen;
    hmap_t clients;
};

#define IP4_EPHEMERAL_MIN 0xC000
#define IP4_EPHEMERAL_MAX 0xEE47

/* Bind a socket to a local port in order to redirect incoming packet */
int ip4_socket_bind(ip4_master_t *master, bbtree_t *tree, socket_t *sock, const uint8_t *addr)
{
    int ipclass = ip4_identify(addr);
    if (ipclass < 0)
        return -1;
    ip4_port_t *nport = kalloc(sizeof(ip4_port_t));
    uint16_t port = ntohs(*((uint16_t *)&addr[4]));

    // Looking for the port
    splock_lock(&master->plock);
    ip4_port_t *iport = bbtree_search_eq(tree, port, ip4_port_t, bnode);
    if (iport != NULL) {
        splock_unlock(&master->plock);
        kfree(nport);
        return -1;
    }

    nport->sock = sock;
    nport->bnode.value_ = nport->port = port;
    nport->listen = false;
    nport->binded = true;
    hmp_init(&nport->clients, 8);
    bbtree_insert(tree, &nport->bnode);

    splock_unlock(&master->plock);
    return 0;
}

/* Look for an ephemeral port on UDP or TCP */
uint16_t ip4_ephemeral_port(ip4_master_t *master, bbtree_t *tree, socket_t *sock)
{
    ip4_port_t *nport = kalloc(sizeof(ip4_port_t));

    // Looking for the next ephemeral port
    splock_lock(&master->plock);
    uint32_t port = IP4_EPHEMERAL_MAX;
    ip4_port_t *iport = bbtree_search_le(tree, port, ip4_port_t, bnode);
    if (iport == NULL)
        port = IP4_EPHEMERAL_MIN;
    else if (iport->port < port)
        port = MIN(iport->port + 1, IP4_EPHEMERAL_MIN);
    else {
        port = IP4_EPHEMERAL_MIN;
        iport = bbtree_search_ge(tree, port, ip4_port_t, bnode);
        ip4_port_t *next = bbtree_next(&iport->bnode, ip4_port_t, bnode);
        while (next && next->port == iport->port + 1) {
            iport = next;
            next = bbtree_next(&iport->bnode, ip4_port_t, bnode);
        }
        port = iport->port + 1;
    }

    // Bind the socket
    *((uint16_t *)&sock->laddr[4]) = htons(port);
    if (sock->flags & NET_ADDR_CONNECTED)
        memcpy(sock->laddr, sock->raddr, IP4_ALEN);
    else
        memset(sock->laddr, 0, IP4_ALEN);
    sock->flags |= NET_ADDR_BINDED;

    // Block the port
    nport->sock = sock;
    nport->bnode.value_ = nport->port = port;
    nport->listen = false;
    nport->binded = false;
    hmp_init(&nport->clients, 8);
    bbtree_insert(tree, &nport->bnode);


    splock_unlock(&master->plock);
    return port;
}

int ip4_socket_close(ip4_master_t *master, bbtree_t *tree, socket_t *sock)
{
    uint16_t port = ntohs(*((uint16_t *)&sock->laddr[4]));
    splock_lock(&master->plock);
    ip4_port_t *iport = bbtree_search_le(tree, port, ip4_port_t, bnode);
    if (iport == NULL) {
        splock_unlock(&master->plock);
        return 0;
    }
    if (iport->sock != sock) {
        socket_t *scpy = hmp_get(&iport->clients, (char *)sock->raddr, 6);
        if (scpy != sock) {
            splock_unlock(&master->plock);
            return -1;
        }
        hmp_remove(&iport->clients, (char *)sock->raddr, 6);

    } else {
        // TODO -- Race condition if recently given away by 'ip4_lookfor_socket'
        iport->listen = false;
        iport->sock = NULL;
    }

    if (iport->sock == NULL && iport->clients.count == 0) {
        bbtree_remove(tree, port);
        hmp_destroy(&iport->clients);
        kfree(iport);
    }
    splock_unlock(&master->plock);
    return 0;
}

/* Check if a socket is listen a port */
socket_t *ip4_lookfor_socket(ip4_master_t *master, bbtree_t *tree, ifnet_t *ifnet, uint16_t port)
{
    splock_lock(&master->plock);
    ip4_port_t *iport = bbtree_search_le(tree, port, ip4_port_t, bnode);
    if (iport == NULL) {
        splock_unlock(&master->plock);
        return NULL;
    }
    socket_t *sock = iport->sock;
    if (iport->listen) {
        // TODO -- Look is this a new packet or for a client socket ?
    }
    splock_unlock(&master->plock);
    // TODO -- Check Local address !
    return sock;
}

/* Accept a new client socket */
int ip4_socket_accept(ip4_master_t *master, bbtree_t *tree, socket_t *sock, socket_t *model, skb_t *skb)
{
    uint16_t port = ntohs(*((uint16_t *)&model->laddr[4]));

    // Connect socket
    memcpy(sock->raddr, &skb->addr[skb->addrlen - model->proto->addrlen], model->proto->addrlen);
    sock->flags |= NET_ADDR_CONNECTED;

    // Bind socket
    memcpy(sock->laddr, sock->raddr, IP4_ALEN);
    memcpy(&sock->laddr[IP4_ALEN], &model->laddr[4], 2);
    sock->flags |= NET_ADDR_BINDED;

    // Looking for the port
    splock_lock(&master->plock);
    ip4_port_t *iport = bbtree_search_eq(tree, port, ip4_port_t, bnode);
    if (iport == NULL) {
        kprintf(-1, "Error, unable to accept a socket assigned to wrong port\n");
        splock_unlock(&master->plock);
        return -1;
    }

    assert(iport->sock == model);
    iport->listen = true;
    
    hmp_put(&iport->clients, (char *)sock->raddr, 6, sock);
    splock_unlock(&master->plock);
    return 0;
}

