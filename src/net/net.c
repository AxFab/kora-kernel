/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2019  <Fabien Bavent>
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
#include <kernel/task.h>
#include <string.h>

static int net_no = 0;

/* Create a new tx packet */
skb_t *net_packet(netdev_t *ifnet, unsigned size)
{
    if (size > ifnet->mtu)
        size = ifnet->mtu;
    skb_t *skb = (skb_t *)kalloc(sizeof(skb_t) + size);
    skb->ifnet = ifnet;
    skb->size = size;
    return skb;
}

/* Create a new rx packet and push it into received queue */
void net_recv(netdev_t *ifnet, uint8_t *buf, unsigned len)
{
    skb_t *skb = (skb_t *)kalloc(sizeof(skb_t) + len);
    skb->ifnet = ifnet;
    memcpy(skb->buf, buf, len);
    skb->length = len;
    ifnet->rx_packets++;
    ifnet->rx_bytes += len;

    /* Push on packets queue */
    splock_lock(&ifnet->lock);
    ll_enqueue(&ifnet->queue, &skb->node);
    splock_unlock(&ifnet->lock);
}

/* Send a tx packet to the network card */
int net_send(skb_t *skb)
{
    if (skb->err)
        return net_trash(skb);
    skb->ifnet->tx_packets++;
    skb->ifnet->tx_bytes += skb->length;
    // splock_lock(&skb->ifnet->lock);
    int ret = skb->ifnet->send(skb->ifnet, skb);
    // splock_unlock(&skb->ifnet->lock);
    if (ret != 0)
        skb->ifnet->tx_errors++;
    kfree(skb);
    return ret;
}

/* Trash a faulty tx packet */
int net_trash(skb_t *skb)
{
    kprintf(KLOG_NET, "Error on packet %s \n", skb->log);
    skb->ifnet->tx_packets++;
    skb->ifnet->tx_dropped++;
    kfree(skb);
    return -1;
}

/* Read data from a packet */
int net_read(skb_t *skb, void *buf, unsigned len)
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
int net_write(skb_t *skb, const void *buf, unsigned len)
{
    if (skb->err)
        return -1;
    if (skb->pen + len > skb->ifnet->mtu) {
        skb->err |= NET_ERR_OVERFILL;
        return -1;
    }

    memcpy(&skb->buf[skb->pen], buf, len);
    skb->pen += len;
    if (skb->pen > skb->length)
        skb->length = skb->pen;
    return 0;
}

/* Get pointer on data from a packet and move cursor */
void *net_pointer(skb_t *skb, unsigned len)
{
    if (skb->err)
        return NULL;
    if (skb->pen + len > skb->ifnet->mtu) {
        skb->err |= NET_ERR_OVERFILL;
        return NULL;
    }

    void *ptr = &skb->buf[skb->pen];
    skb->pen += len;
    if (skb->pen > skb->length)
        skb->length = skb->pen;
    return ptr;
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

uint16_t net_rand_port()
{
    return (rand() & 0xFFFF) | 0x400;
}

uint16_t net_ephemeral_port(socket_t *socket)
{
    uint16_t port = net_rand_port();
    // TODO Test port isn't taken
    return port;
}


void net_print(netdev_t *ifnet)
{
    char buf[20];
    kprintf(KLOG_DBG, "eth%d:  flags < %s%s>  mtu %d\n", ifnet->no,
            ifnet->flags & NET_CONNECTED ? "UP " : "",
            ifnet->flags & NET_QUIET ? "QUIET " : "",
            ifnet->mtu);
    kprintf(KLOG_DBG, "    MAC address %s\n", net_ethstr(buf, ifnet->eth_addr));
    kprintf(KLOG_DBG, "    IP address %s\n", net_ip4str(buf, ifnet->ip4_addr));
    kprintf(KLOG_DBG, "    RX packets %d   bytes %d (%s)\n", ifnet->rx_packets,
            ifnet->rx_bytes, sztoa(ifnet->rx_bytes));
    kprintf(KLOG_DBG, "    RX errors %d   dropped %d\n", ifnet->rx_errors,
            ifnet->rx_dropped);
    kprintf(KLOG_DBG, "    TX packets %d   bytes %d (%s)\n", ifnet->tx_packets,
            ifnet->tx_bytes, sztoa(ifnet->tx_bytes));
    kprintf(KLOG_DBG, "    TX errors %d   dropped %d\n", ifnet->tx_errors,
            ifnet->tx_dropped);
}



// ------------

#define NET_DELAY 5000000 // 1sec
#define NET_LONG_DELAY 10000000 // 10sec


// void net_tasklet(netdev_t *ifnet)
// {
//     int ret;
//     for (;;) {
//         splock_lock(&ifnet->lock);
//         skb_t *skb = ll_dequeue(&ifnet->queue, skb_t, node);
//         splock_unlock(&ifnet->lock);
//         if (skb != NULL) {
//             ret = eth_receive(skb);
//             if (ret != 0)
//                 ifnet->rx_errors++;
//             kfree(skb);
//         }
//     }
// }

void net_tasklet(netdev_t *ifnet)
{
    bool printed_ip4 = false;
    clock64_t timeout = kclock() + NET_DELAY;
    while ((ifnet->flags & NET_QUIT) == 0)  {

        /* Read available packets */
        for (;;) {
            splock_lock(&ifnet->lock);
            skb_t *skb = NULL;
            while (timeout - kclock() > 0) {
                skb = ll_dequeue(&ifnet->queue, skb_t, node);
                if (skb != NULL)
                    break;
                async_wait(&ifnet->lock, &ifnet->waiters, 1000); // timeout - kclock());
            }
            splock_unlock(&ifnet->lock);
            if (skb == NULL)
                break;

            int ret = eth_receive(skb);
            kprintf(KLOG_DBG, "Packet received %s (%d) : %d \n", skb->log, skb->length,
                    ret);
            if (ret != 0)
                ifnet->rx_errors++;
            kfree(skb);
        }

        timeout = kclock() + NET_DELAY;

        /* Initialize hardware */
        if (!(ifnet->flags & NET_CONNECTED)) {
            memset(ifnet->ip4_addr, 0, IP4_ALEN);
            kprintf(KLOG_NET, "Initialize eth%d\n", ifnet->no);
            ifnet->link(ifnet);
            net_print(ifnet);
            continue;
        }


        if (ip4_null(ifnet->ip4_addr)) {
            dhcp_discovery(ifnet);
            continue;
        }

        if (!printed_ip4) {
            net_print(ifnet);
            printed_ip4 = true;
        }

        kprintf(KLOG_NET, "Net ticks eth%d\n", ifnet->no);
        // /* check connection - TODO find generic method */
        // if (!(ifnet->flags & NET_CNX_IP4)) {
        //     dhcp_discovery(ifnet);
        //     continue;
        // }

        // if ((ifnet->lease_out - kclock()) * 4 < ifnet->lease_full && kclock() - ifnet->lease_last > NET_LEASE_25P) {
        //     dhcp_renew(ifnet);
        //     continue;
        // }

        // if ((ifnet->lease_out - kclock()) * 2 < ifnet->lease_full && kclock() - ifnet->lease_last > NET_LEASE_50P) {
        //     dhcp_renew(ifnet);
        //     continue;
        // }

        /* Longer delay when no imediate requirement */
        timeout = kclock() + NET_LONG_DELAY;
    }
}


int net_device(netdev_t *ifnet)
{
    ifnet->no = ++net_no;
    splock_init(&ifnet->lock);
    char tmp[20];
    kprintf(-1, "Network interface (eth%d) - MAC: \033[92m%s\033[0m\n", ifnet->no,
            net_ethstr(tmp, ifnet->eth_addr));
    snprintf(tmp, 20, "Ethernet.#%d", ifnet->no);
    task_create(&net_tasklet, ifnet, tmp);
    return 0;
}

// for (;;) {
//     if (rule == NULL) {
//         timeout = kclock() + NET_LONG_DELAY;
//         break;
//     }
//     if (rule->test(ifnet)) {
//         rule->action(ifnet);
//         break;
//     }
//     rule = rule->next;
// }

