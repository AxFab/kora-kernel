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
#include <kernel/net.h>


void net_tasklet(netdev_t *ifnet)
{
    int ret;
    for (;;) {
        skb_t *skb = ifnet.poll();...
        if (skb != NULL) {
            ret = eth_receive(skb);
            if (ret != 0)
                ifnet->rx_errors++;
            kfree(skb);
        }
    }
}

int net_device(netdev_t *ifnet)
{
    ...
}

/* Create a new tx packet */
skb_t *net_packet(netdev_t *ifnet)
{
    skb_t *skb = (skb_t*)kalloc(sizeof(skb_t));
    skb->ifnet = ifnet;
    return skb;
}

/* Create a new rx packet and push it into received queue */
void net_recv(netdev_t *ifnet, uint8_t *buf, int len)
{
    skb_t *skb = (skb_t*)kalloc(sizeof(skb_t));
    skb->ifnet = ifnet;
    memcpy(skb->buf, buf, len);
    skb->length = len;
    ifnet->rx_packets++;
    ifnet->rx_bytes += len;

    /* Push on packets queue */
    ...
}

/* Send a tx packet to the network card */
int net_send(skb_t *skb)
{
    if (skb->err)
        return net_trash(skb);
    skb->ifnet->tx_packets++;
    skb->ifnet->tx_bytes += skb->length;
    int ret = skb->ifnet->send(skb->ifnet, skb->buf, skb->length);
    if (ret != 0)
        skb->ifnet->tx_errors++;
    kfree(skb);
    return ret;
}

/* Trash a faulty tx packet */
int net_trash(skb_t *skb)
{
    skb->ifnet->tx_packets++;
    skb->ifnet->tx_dropped++;
    kfree(skb);
    return -1;
}

/* Read data from a packet */
int net_read(skb_t *skb, void *buf, int len)
{
    if (skb->err)
        return -1;
    if (skb->length < skb->pen + len) {
        skb->err |= NET_ERR_OVERFILL;
        return -1;
    }

    memcpy(buf, &skb->buf[skb->pen], len);
    skb->pen += len;
    return 0;
}

/* Write data on a packet */
int net_write(skb_t *skb, const void *buf, int len)
{
    if (skb->err)
        return -1;
    if (skb->pen + len > skb->ifnet->max_packet_size) {
        skb->err |= NET_ERR_OVERFILL;
        return -1;
    }

    memcpy(&skb->buf[skb->pen], buf, len);
    skb->pen += len;
    if (skb->pen > skb->length)
        skb->length = skb->pen;
    return 0;
}

char *net_ethstr(char *buf, uint8_t *mac)
{
    snprintf(buf, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return buf;
}

char *net_ip4str(char *buf, uint8_t *ip)
{
    snprintf(buf, 16, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
    return buf;
}
