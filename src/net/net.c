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
#ifdef _WIN32
#include <threads.h>
#endif
#include <kernel/net.h>
#include <kernel/core.h>

void net_deamon(netstack_t *stack)
{
#ifdef _WIN32
    char buf[128];
    wchar_t wbuf[128];
    snprintf(buf, 128, "Deamon-%s", stack->hostname);
    size_t len;
    mbstowcs_s(&len, wbuf, 128, buf, 128);
    SetThreadDescription(GetCurrentThread(), wbuf);
#endif

    for (;;) {
        skb_t *skb;

        sem_acquire(&stack->rx_sem);

        splock_lock(&stack->rx_lock);
        skb = ll_dequeue(&stack->rx_list, skb_t, node);
        splock_unlock(&stack->rx_lock);
        if (skb == NULL)
            continue;

        kprintf(-1, "Rx %s\n", skb->ifnet->stack->hostname);
        // kdump(skb->buf, skb->length);

        net_recv_t recv = NULL;
        if (skb->ifnet->protocol == NET_AF_ETH)
            recv = eth_receive;

        if (recv == NULL || recv(skb) != 0) {
            skb->ifnet->rx_dropped++;
            kfree(skb);
        }
    }
}

ifnet_t *net_alloc(netstack_t *stack, int protocol, uint8_t *hwaddr, net_ops_t *ops, void *driver)
{
    int hwlen = 0;
    char buf[50];
    int uid = atomic_xadd(&stack->idMax, 1);
    if (protocol == NET_AF_LBK) {
        kprintf(-1, "New endpoint \e[35m%s:lo:%i\e[0m\n", stack->hostname, uid);
    } else if (protocol == NET_AF_ETH) {
        hwlen = ETH_ALEN;
        eth_writemac(hwaddr, buf, 50);
        kprintf(-1, "New endpoint \e[35m%s:eth:%i (MAC %s)\e[0m\n", stack->hostname, uid, buf);
    } else {
        kprintf(-1, "\e[31mUnable to allocate network interface: unknown protocol %d\e[0m\n", protocol);
        return NULL;
    }

    ifnet_t *net = kalloc(sizeof(ifnet_t));
    net->protocol = protocol;
    net->mtu = 1500;
    if (hwlen != 0)
        memcpy(net->hwaddr, hwaddr, ETH_ALEN);

    splock_lock(&stack->lock);
    net->idx = uid;
    net->stack = stack;
    net->ops = ops;
    net->drv_data = driver;
    ll_append(&stack->list, &net->node);
    splock_unlock(&stack->lock);

    return net;
}

ifnet_t *net_interface(netstack_t *stack, int protocol, int idx)
{
    ifnet_t *net;
    splock_lock(&stack->lock);
    for ll_each(&stack->list, net, ifnet_t, node) {
        if (net->protocol == protocol && net->idx == idx) {
            splock_unlock(&stack->lock);
            return net;
        }
    }
    splock_unlock(&stack->lock);
    return NULL;
}

netstack_t *net_setup()
{
    netstack_t *stack = kalloc(sizeof(netstack_t));
    sem_init(&stack->rx_sem, 0);
    splock_init(&stack->rx_lock);
    splock_init(&stack->lock);
    return stack;
}


netstack_t *__netstack;
extern net_ops_t loopback_ops;

void net_init()
{
    __netstack = net_setup();
    __netstack->hostname = strdup("vmdev");
    // kthread(net_deamon, stack);
    net_alloc(__netstack, NET_AF_LBK, NULL, &loopback_ops, NULL);

    task_start("Network stack deamon", net_deamon, __netstack);
}

netstack_t* net_stack()
{
    return __netstack;
}


EXPORT_SYMBOL(net_alloc, 0);
EXPORT_SYMBOL(net_stack, 0);
