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
#include <kora/mcrs.h>

/* Create a new tx packet */
skb_t *net_packet(ifnet_t *net)
{
    int len = MAX(1500, net->mtu);
    skb_t *skb = (skb_t *)kalloc(sizeof(skb_t) + len);
    skb->ifnet = net;
    skb->size = len;
    return skb;
}

/* Create a new rx packet and push it into received queue */
void net_skb_recv(ifnet_t *net, uint8_t *buf, unsigned len)
{
    skb_t *skb = (skb_t *)kalloc(sizeof(skb_t) + len);
    skb->size = len;
    skb->ifnet = net;
    memcpy(skb->buf, buf, len);
    skb->length = len;
    skb->protocol = net->protocol;
    net->rx_packets++;
    net->rx_bytes += len;

    /* Push on packets queue */
    netstack_t *stack = net->stack;
    splock_lock(&stack->rx_lock);
    ll_enqueue(&stack->rx_list, &skb->node);
    splock_unlock(&stack->rx_lock);
    sem_release(&stack->rx_sem);
}

/* Send a tx packet to the network card */
int net_skb_send(skb_t *skb)
{
    if (skb->err)
        return net_skb_trash(skb);
    skb->ifnet->tx_packets++;
    int ret = skb->ifnet->ops->send(skb->ifnet, skb);
    if (ret != 0)
        skb->ifnet->tx_errors++;
    else
        skb->ifnet->tx_bytes += skb->length;
    kfree(skb);
    return ret;
}

/* Trash a faulty tx packet */
int net_skb_trash(skb_t *skb)
{
    kprintf(-1, "Error on packet %s \n", skb->log);
    skb->ifnet->tx_packets++;
    skb->ifnet->tx_dropped++;
    kfree(skb);
    return -1;
}

/* Read data from a packet */
int net_skb_read(skb_t *skb, void *buf, unsigned len)
{
    if (skb->err)
        return -1;
    if (skb->length < skb->pen + len) {
        skb->err |= NET_OVERFILL;
        return -1;
    }

    memcpy(buf, &skb->buf[skb->pen], len);
    skb->pen += len;
    return 0;
}

/* Write data on a packet */
int net_skb_write(skb_t *skb, const void *buf, unsigned len)
{
    if (skb->err)
        return -1;
    if (skb->pen + len > skb->ifnet->mtu) {
        skb->err |= NET_OVERFILL;
        return -1;
    }

    memcpy(&skb->buf[skb->pen], buf, len);
    skb->pen += len;
    if (skb->pen > skb->length)
        skb->length = skb->pen;
    return 0;
}


/* Get pointer on data from a packet and move cursor */
void *net_skb_reserve(skb_t *skb, unsigned len)
{
    if (skb->err)
        return NULL;
    if (skb->pen + len > skb->size) {
        skb->err |= NET_OVERFILL;
        return NULL;
    }

    void *ptr = &skb->buf[skb->pen];
    skb->pen += len;
    if (skb->pen > skb->length)
        skb->length = skb->pen;
    return ptr;
}

EXPORT_SYMBOL(net_packet, 0);
EXPORT_SYMBOL(net_skb_recv, 0);
EXPORT_SYMBOL(net_skb_send, 0);
EXPORT_SYMBOL(net_skb_trash, 0);
EXPORT_SYMBOL(net_skb_read, 0);
EXPORT_SYMBOL(net_skb_write, 0);
EXPORT_SYMBOL(net_skb_reserve, 0);

