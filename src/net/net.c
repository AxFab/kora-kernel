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

void net_event(ifnet_t* net, int event, int param)
{
    int len = 2 * sizeof(int);
    skb_t* skb = kalloc(sizeof(skb_t) + len);
    skb->ifnet = net;
    skb->size = len;
    skb->protocol = NET_AF_EVT;
    int* ptr = net_skb_reserve(skb, len);
    skb->pen = 0;
    ptr[0] = event;
    ptr[1] = param;

    netstack_t* stack = net->stack;
    splock_lock(&stack->rx_lock);
    ll_enqueue(&stack->rx_list, &skb->node);
    splock_unlock(&stack->rx_lock);
    sem_release(&stack->rx_sem);
}

void net_handle_event(skb_t* skb)
{
    int len = 2 * sizeof(int);
    netstack_t* stack = skb->ifnet->stack;
    int* ptr = net_skb_reserve(skb, len);
    splock_lock(&stack->lock);
    nhandler_t *hnode = ll_first(&stack->handlers, nhandler_t, node);
    splock_unlock(&stack->lock);
    while (hnode) {
        hnode->handler(skb->ifnet, ptr[0], ptr[1]);
        splock_lock(&stack->lock);
        hnode = ll_next(&hnode->node, nhandler_t, node);
        splock_unlock(&stack->lock);
    }
    kfree(skb);
}

void net_handler(netstack_t* stack, void(*handler)(ifnet_t *, int, int))
{
    nhandler_t* hnode = kalloc(sizeof(nhandler_t));
    hnode->handler = handler;
    splock_lock(&stack->lock);
    ll_append(&stack->handlers, &hnode->node);
    splock_unlock(&stack->lock);
}


nproto_t *net_protocol(netstack_t* stack, int protocol)
{
    splock_lock(&stack->lock);
    nproto_t* proto = bbtree_search_eq(&stack->protocols, protocol, nproto_t, bnode);
    splock_unlock(&stack->lock);
    return proto;
}

void net_set_protocol(netstack_t* stack, int protocol, nproto_t* proto)
{
    splock_lock(&stack->lock);
    proto->bnode.value_ = protocol;
    bbtree_insert(&stack->protocols, &proto->bnode);
    splock_unlock(&stack->lock);
}

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

        // Look for incoming packet
        splock_lock(&stack->rx_lock);
        skb = ll_dequeue(&stack->rx_list, skb_t, node);
        splock_unlock(&stack->rx_lock);
        if (skb == NULL)
            continue;

        if (skb->protocol == NET_AF_EVT) {
            net_handle_event(skb);
            continue;
        }

        skb->ifnet->rx_packets++;
        kprintf(-1, "Rx %s:%d\n", skb->ifnet->stack->hostname, skb->ifnet->idx);
        // kdump(skb->buf, skb->length);

        splock_lock(&stack->lock);
        nproto_t* proto = bbtree_search_eq(&stack->protocols, skb->protocol, nproto_t, bnode);
        splock_unlock(&stack->lock);

        if (proto == NULL || proto->receive == NULL || proto->receive(skb) != 0)  {
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
        kprintf(-1, "New endpoint \033[35m%s:lo:%i\033[0m\n", stack->hostname, uid);
    } else if (protocol == NET_AF_ETH) {
        hwlen = ETH_ALEN;
        eth_writemac(hwaddr, buf, 50);
        kprintf(-1, "New endpoint \033[35m%s:eth:%i (MAC %s)\033[0m\n", stack->hostname, uid, buf);
    } else {
        kprintf(-1, "\033[31mUnable to allocate network interface: unknown protocol %d\033[0m\n", protocol);
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
    bbtree_init(&stack->protocols);
    return stack;
}


#ifdef KORA_KRN
netstack_t *__netstack;
extern net_ops_t loopback_ops;

void net_init()
{
    __netstack = net_setup();
    __netstack->hostname = strdup("vmdev");
    net_alloc(__netstack, NET_AF_LBK, NULL, &loopback_ops, NULL);

    task_start("Network stack deamon", net_deamon, __netstack);
}

netstack_t* net_stack()
{
    return __netstack;
}

EXPORT_SYMBOL(net_stack, 0);
#endif

EXPORT_SYMBOL(net_alloc, 0);
EXPORT_SYMBOL(net_interface, 0);
