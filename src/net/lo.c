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
#include <kora/hmap.h>

 // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

void lo_link(ifnet_t *net)
{
    net->flags |= NET_CONNECTED;
}

int lo_send(ifnet_t *net, skb_t *skb)
{
    net_skb_recv(net, skb->buf, skb->length);
    return 0;
}

net_ops_t lo_ops = {
    .send = lo_send,
    .link = lo_link,
};

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

typedef struct lo_info lo_info_t;

struct lo_info
{
    splock_t lock;
    hmap_t recv_map;
};

/* Gets protocol driver data store */
static lo_info_t *lo_readinfo(netstack_t *stack)
{
    nproto_t *proto = net_protocol(stack, NET_AF_LO);
    assert(proto != NULL && proto->data);
    lo_info_t *info = proto->data;
    return info;
}

/* Gets the receiver method capable to handle next layer of a packet */
static net_recv_t lo_receiver(netstack_t *stack, uint16_t protocol)
{
    lo_info_t *info = lo_readinfo(stack);
    return hmp_get(&info->recv_map, (char *)&protocol, sizeof(uint16_t));
}

/* Registers a new protocol capable of using loopback */
int lo_handshake(netstack_t *stack, uint16_t protocol, net_recv_t recv)
{
    if (recv == NULL)
        return -1;
    lo_info_t *info = lo_readinfo(stack);
    splock_lock(&info->lock);
    hmp_put(&info->recv_map, (char *)&protocol, sizeof(uint16_t), recv);
    splock_unlock(&info->lock);
    return 0;
}

/* Unregisters a protocol capable of using loopback */
int lo_unregister(netstack_t *stack, uint16_t protocol, net_recv_t recv)
{
    if (recv == NULL)
        return -1;
    lo_info_t *info = lo_readinfo(stack);
    splock_lock(&info->lock);
    net_recv_t prev = hmp_get(&info->recv_map, (char *)&protocol, sizeof(uint16_t));
    if (prev != recv) {
        splock_unlock(&info->lock);
        return -1;
    }
    hmp_remove(&info->recv_map, (char *)&protocol, sizeof(uint16_t));
    splock_unlock(&info->lock);
    return 0;
}

/* Writes a loopback header on a tx packet */
int lo_header(skb_t *skb, uint16_t protocol, uint16_t port)
{
    net_log(skb, "lo");
    if (skb->ifnet->protocol != NET_AF_LO) {
        net_log(skb, ":Bad protocol");
        return -1;
    }

    if (protocol != 0)
        port = 0;
    net_skb_write(skb, &port, sizeof(uint16_t));
    net_skb_write(skb, &protocol, sizeof(uint16_t));
    return 0;
}

/* Read and transert to the upper layer an loopback rx packet */
int lo_receive(skb_t *skb)
{
    assert(skb && skb->ifnet && skb->ifnet->stack && skb->ifnet->proto);
    net_log(skb, "lo");
    if (skb->ifnet->protocol != NET_AF_LO) {
        net_log(skb, ":Bad protocol");
        return -1;
    }

    uint16_t protocol;
    uint16_t port;
    net_skb_read(skb, &port, sizeof(uint16_t));
    net_skb_read(skb, &protocol, sizeof(uint16_t));
    skb->addrlen = 0;

    netstack_t *stack = skb->ifnet->stack;
    net_recv_t recv = lo_receiver(stack, protocol);
    if (recv == NULL) {
        net_log(skb, ":Unknown protocol");
        return -1;
    }

    return recv(skb);
}

int lo_destroy(netstack_t *stack)
{
    nproto_t *proto = net_protocol(stack, NET_AF_LO);
    lo_info_t *info = proto->data;
    if (info->recv_map.count > 0)
        return -1;
    kfree(info);
    hmp_destroy(&info->recv_map);
    kfree(proto);
    return 0;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

int lo_teardown(netstack_t *stack)
{
    nproto_t *proto = net_protocol(stack, NET_AF_LO);
    if (proto == NULL)
        return -1;
    lo_info_t *info = proto->data;
    if (info->recv_map.count > 0)
        return -1;
    net_rm_protocol(stack, NET_AF_LO);
    hmp_destroy(&info->recv_map);
    kfree(info);
    kfree(proto);
    return 0;
}

/* Entry point of the loopback module */
int lo_setup(netstack_t *stack)
{
    nproto_t *proto = net_protocol(stack, NET_AF_LO);
    if (proto != NULL)
        return -1;

    proto = kalloc(sizeof(nproto_t));
    lo_info_t *info = kalloc(sizeof(lo_info_t));
    hmp_init(&info->recv_map, 8);
    proto->receive = lo_receive;
    proto->data = info;
    proto->name = "lo";
    proto->addrlen = 0;
    proto->teardown = lo_teardown;
    net_set_protocol(stack, NET_AF_LO, proto);

    net_device(stack, NET_AF_LO, NULL, &lo_ops, NULL);
    return 0;
}


EXPORT_SYMBOL(lo_handshake, 0);
EXPORT_SYMBOL(lo_unregister, 0);
EXPORT_SYMBOL(lo_header, 0);
