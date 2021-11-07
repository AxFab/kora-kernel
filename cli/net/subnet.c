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
#include <stdio.h>
#include <kernel/net.h>
#include <kernel/core.h>


typedef struct subnet subnet_t;

struct subnet
{
    size_t len;
    ifnet_t *slots[0];
};

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#define ETH_ALEN 6

/* Fake ethernet controller, method called to check connection */
void fake_eth_link(ifnet_t *net)
{
    net->flags |= NET_CONNECTED;
    net_event(net, NET_EV_LINK, 0);
}

void fake_eth_unlink(ifnet_t *net)
{
    net->flags &= ~NET_CONNECTED;
    net_event(net, NET_EV_UNLINK, 0);
}

int fake_eth_close(ifnet_t *net)
{
    if (net->drv_data != NULL)
        lan_disconnect(net->drv_data, net);
    return 0;
}

/* Fake ethernet controller, method to send a packet */
static int fake_eth_send(ifnet_t *net, skb_t *skb)
{
    if ((net->flags & NET_CONNECTED) == 0) {
        printf("Tx %s: %s : disconnected\n", net->stack->hostname, skb->log);
        return -1;
    }

    printf("Tx %s: %s\n", net->stack->hostname, skb->log);
    subnet_t *subnet = (subnet_t *)net->drv_data;
    if (subnet == NULL)
        return -1;

    for (unsigned i = 0; i < subnet->len; ++i) {
        if (subnet->slots[i] && subnet->slots[i] != net)
            net_skb_recv(subnet->slots[i], skb->buf, skb->length);
    }
    return 0;
}

struct net_ops fake_eth_ops = {
    .link = fake_eth_link,
    .send = fake_eth_send,
    .close = fake_eth_close,
};

void fake_eth_setup(netstack_t *stack)
{
    char hwaddr[ETH_ALEN];
    hwaddr[0] = 0x80;
    for (int i = 1; i < 6; ++i)
        hwaddr[i] = rand();
    ifnet_t *net = net_device(stack, NET_AF_ETH, hwaddr, &fake_eth_ops, NULL);
    net->mtu = 1500;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

netstack_t *lan_init(const char *name, int cards)
{
    // Create the network stack
    netstack_t *stack = net_create_stack();
    stack->hostname = kstrdup(name);
    // Register known protocols
    lo_setup(stack);
    eth_setup(stack);
    ip4_start(stack);
    // Create feke ethernet devices
    for (int i = 0; i < cards; ++i)
        fake_eth_setup(stack);

    char buf[128];
    snprintf(buf, 128, "deamon-%s", stack->hostname);
    task_start(buf, net_deamon, stack);
    return stack;
}

int lan_connect(subnet_t *subnet, ifnet_t *ifnet)
{
    if (ifnet->drv_data != NULL) {
        fprintf(stderr, "Error, this device %s:%s:%d is already connected somewhere else\n",
            ifnet->stack->hostname, ifnet->proto->name, ifnet->idx);
        return -1;
    }
    for (unsigned i = 0; i < subnet->len; ++i) {
        if (subnet->slots[i] == NULL) {
            subnet->slots[i] = ifnet;
            ifnet->drv_data = subnet;
            ifnet->ops->link(ifnet);
            return 0;
        }
    }
    ifnet->drv_data = NULL;
    fprintf(stderr, "Error, no place for the device %s:%s:%d on this subnet\n",
        ifnet->stack->hostname, ifnet->proto->name, ifnet->idx);
    return -1;
}

int lan_disconnect(subnet_t *subnet, ifnet_t *ifnet)
{
    if (ifnet->drv_data != subnet) {
        fprintf(stderr, "Error, this device %s:%s:%d is not connected to this subnet\n",
            ifnet->stack->hostname, ifnet->proto->name, ifnet->idx);
        return -1;
    }
    int count = 0;
    for (unsigned i = 0; i < subnet->len; ++i) {
        if (subnet->slots[i] == ifnet)
            subnet->slots[i] = NULL;
        else if (subnet->slots[i] != ifnet)
            count++;
    }

    fake_eth_unlink(ifnet);
    ifnet->drv_data = NULL;
    return count;
}


// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=


