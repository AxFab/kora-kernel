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
#include <kernel/core.h>
#include <assert.h>

/* Dispatch an event to registered listener (see net_handler) */
static void net_handle_event(skb_t *skb)
{
    int len = 2 * sizeof(int);
    assert(skb && skb->ifnet && skb->ifnet->stack); // Handling of corrupted event packet
    netstack_t *stack = skb->ifnet->stack;
    int *ptr = net_skb_reserve(skb, len);
    assert(skb->err == 0); // Unable to read an event packet
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

/* Push an event on an interface device (connected/disconnected) */
int net_event(ifnet_t *net, int event, int param)
{
    // TODO -- If paranoid, should check network is registered
    if (net == NULL)
        return -1;
    assert(net->stack != NULL);
    int len = 2 * sizeof(int);
    skb_t *skb = kalloc(sizeof(skb_t) + len);
    skb->ifnet = net;
    skb->size = len;
    skb->protocol = NET_AF_EVT;
    int *ptr = net_skb_reserve(skb, len);
    if (ptr == NULL)
        return net_skb_trash(skb);
    skb->pen = 0; // Rewind for reading
    ptr[0] = event;
    ptr[1] = param;

    // Push on receiving list, don't count as `rx_packets`
    netstack_t *stack = net->stack;
    splock_lock(&stack->rx_lock);
    ll_enqueue(&stack->rx_list, &skb->node);
    splock_unlock(&stack->rx_lock);
    sem_release(&stack->rx_sem);
    return 0;
}

/* Register an handler to listen to network interface events */
void net_handler(netstack_t *stack, void(*handler)(ifnet_t *, int, int))
{
    assert(stack != NULL);
    if (handler == NULL)
        return;
    nhandler_t *hnode = kalloc(sizeof(nhandler_t));
    hnode->handler = handler;
    splock_lock(&stack->lock);
    ll_append(&stack->handlers, &hnode->node);
    splock_unlock(&stack->lock);
}

/* Search of a protocol registered on the network stack */
nproto_t *net_protocol(netstack_t *stack, int protocol)
{
    assert(stack != NULL);
    splock_lock(&stack->lock);
    nproto_t *proto = bbtree_search_eq(&stack->protocols, protocol, nproto_t, bnode);
    splock_unlock(&stack->lock);
    return proto;
}

/* Register a new protocol on the stack */
void net_set_protocol(netstack_t *stack, int protocol, nproto_t *proto)
{
    assert(stack != NULL);
    if (proto == NULL)
        return;
    splock_lock(&stack->lock);
    proto->bnode.value_ = protocol;
    bbtree_insert(&stack->protocols, &proto->bnode);
    splock_unlock(&stack->lock);
    kprintf(-1, "New protocol for %s: %d (%s)\n", stack->hostname, protocol, proto->name);
}

/* Search for an network interface */
ifnet_t *net_interface(netstack_t *stack, int protocol, int idx)
{
    ifnet_t *net;
    assert(stack != NULL);
    splock_lock(&stack->lock);
    for ll_each(&stack->list, net, ifnet_t, node)
    {
        if (net->protocol == protocol && net->idx == idx) {
            splock_unlock(&stack->lock);
            return net;
        }
    }
    splock_unlock(&stack->lock);
    return NULL;
}

/* Register a new network interface */
ifnet_t *net_device(netstack_t *stack, int protocol, uint8_t *hwaddr, net_ops_t *ops, void *driver)
{
    char buf[64];
    assert(stack != NULL);
    nproto_t *proto = net_protocol(stack, protocol);
    int hwlen = proto->addrlen;
    if (proto == NULL) {
        kprintf(-1, "\033[31mUnsupported protocol %d\033[0m\n", protocol);
        return NULL;
    } else if (proto->receive == NULL || proto->addrlen > NET_MAX_HWADRLEN) {
        kprintf(-1, "\033[31mProtocol is not capable to receive packet%d\033[0m\n", protocol);
        return NULL;
    } else if (ops == NULL || ops->send == NULL) {
        kprintf(-1, "\033[31mNetwork device doesn't provide `send` operation method%d\033[0m\n");
        return NULL;
    } else if (hwlen != 0 && hwaddr == NULL) {
        kprintf(-1, "Missing address to register device with this protocol %d\n", protocol);
        return NULL;
    }

    // Allocate the new interface
    int uid = atomic_xadd(&stack->id_max, 1);
    ifnet_t *net = kalloc(sizeof(ifnet_t));
    net->protocol = protocol;
    net->mtu = 1500;
    net->idx = uid;
    net->stack = stack;
    net->ops = ops;
    net->drv_data = driver;
    net->proto = proto;
    if (hwlen != 0)
        memcpy(net->hwaddr, hwaddr, hwlen);

    // Register the new interface
    splock_lock(&stack->lock);
    ll_append(&stack->list, &net->node);
    splock_unlock(&stack->lock);

    // Log the creation of the new interface
    // TODO -- Name and address display must be provided on `nproto_t`
    kprintf(-1, "New endpoint \033[35m%s:%s:%i (%s)\033[0m\n", stack->hostname, proto->name, uid, proto->paddr(hwaddr, buf, 64));
    return net;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

/* Create the network stack (non static only for testing) */
netstack_t *net_create_stack()
{
    netstack_t *stack = kalloc(sizeof(netstack_t));
    sem_init(&stack->rx_sem, 0);
    splock_init(&stack->rx_lock);
    splock_init(&stack->lock);
    bbtree_init(&stack->protocols);
    return stack;
}

/* Deamon thread to watch over incoming package and other
   device events (non static only for testing) */
void net_deamon(netstack_t *stack)
{
    for (;;) {
        // Wait for some new content
        sem_acquire(&stack->rx_sem);

        // Look for incoming packet
        splock_lock(&stack->rx_lock);
        skb_t *skb = ll_dequeue(&stack->rx_list, skb_t, node);
        splock_unlock(&stack->rx_lock);
        if (skb == NULL)
            continue;

        // Special handling of device events
        if (skb->protocol == NET_AF_EVT) {
            net_handle_event(skb);
            continue;
        }

        // Transfer to handler protocol
        assert(skb->ifnet != NULL);
        nproto_t *proto = skb->ifnet->proto;
        assert(proto != NULL && proto->receive != NULL);
        kprintf(-1, "Rx %s:%s%d\n", skb->ifnet->stack->hostname, proto->name, skb->ifnet->idx);
        if (proto->receive(skb) != 0) {
            skb->ifnet->rx_dropped++;
            kfree(skb);
        }
    }
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

netstack_t *__netstack;
extern net_ops_t loopback_ops;

/* Accessor for the kernel network stack */
netstack_t *net_stack()
{
    assert(__netstack != NULL);
    return __netstack;
}

/* Setup of the kernel network stack */
void net_setup()
{
    __netstack = net_create_stack();
    __netstack->hostname = strdup("vmdev");
    lo_setup(__netstack);
    task_start("Network stack deamon", net_deamon, __netstack);
}

EXPORT_SYMBOL(net_event, 0);
EXPORT_SYMBOL(net_handler, 0);
EXPORT_SYMBOL(net_protocol, 0);
EXPORT_SYMBOL(net_set_protocol, 0);
EXPORT_SYMBOL(net_interface, 0);
EXPORT_SYMBOL(net_device, 0);
EXPORT_SYMBOL(net_stack, 0);
