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
#include <kora/hmap.h>
#include <kernel/stdc.h>

 // Ethernet
#define ETH_ALEN 6
#define ETH_IP4 htons(0x0800)
#define ETH_IP6 htons(0x86DD)
#define ETH_ARP htons(0x0806)

static_assert(ETH_ALEN <= NET_MAX_HWADRLEN, "Ethernet addresses are bigger than allowed");

/* Print as a readable string a MAC address */
char *eth_writemac(const uint8_t *mac, char *buf, int len);
/* Registers a new protocol capable of using Ethernet */
int eth_handshake(netstack_t *stack, uint16_t protocol, net_recv_t recv);
/* Writes a ethernet header on a tx packet */
int eth_header(skb_t *skb, const uint8_t *addr, uint16_t protocol);

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

typedef struct eth_info eth_info_t;
typedef struct eth_header eth_header_t;

struct eth_info {
    hmap_t recv_map;
};

PACK(struct eth_header {
    uint8_t target[ETH_ALEN];
    uint8_t source[ETH_ALEN];
    uint16_t protocol;
});

const uint8_t eth_broadcast[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

/* Gets protocol driver data store */
static eth_info_t *eth_readinfo(netstack_t *stack)
{
    nproto_t *proto = net_protocol(stack, NET_AF_ETH);
    assert(proto != NULL && proto->data);
    eth_info_t* info = proto->data;
    return info;
}

/* Gets the receiver method capable to handle next layer of a packet */
static net_recv_t eth_receiver(netstack_t *stack, uint16_t protocol)
{
    eth_info_t *info = eth_readinfo(stack);
    return hmp_get(&info->recv_map, (char *)&protocol, sizeof(uint16_t));
}

/* Print as a readable string a MAC address */
char *eth_writemac(const uint8_t *mac, char *buf, int len)
{
    assert(mac && buf && len);
    snprintf(buf, len, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return buf;
}

/* Registers a new protocol capable of using Ethernet */
int eth_handshake(netstack_t *stack, uint16_t protocol, net_recv_t recv)
{
    if (recv == NULL)
        return -1;
    eth_info_t *info = eth_readinfo(stack);
    hmp_put(&info->recv_map, (char *)&protocol, sizeof(uint16_t), recv);
    return 0;
}

/* Writes a ethernet header on a tx packet */
int eth_header(skb_t *skb, const uint8_t *addr, uint16_t protocol)
{
    net_log(skb, "eth");
    if (skb->ifnet->protocol != NET_AF_ETH) {
        net_log(skb, ":Bad protocol");
        return -1;
    }

    eth_header_t *header = net_skb_reserve(skb, sizeof(eth_header_t));
    if (header == NULL) {
        net_log(skb, ":Unexpected end of data");
        return -1;
    }
    memcpy(header->target, addr, ETH_ALEN);
    memcpy(header->source, skb->ifnet->hwaddr, ETH_ALEN);
    memcpy(header->target, addr, ETH_ALEN);
    header->protocol = protocol;
    return 0;
}

/* Read and transert to the upper layer an ethernet rx packet */
int eth_receive(skb_t *skb)
{
    assert(skb && skb->ifnet && skb->ifnet->stack && skb->ifnet->proto);
    net_log(skb, "eth");
    if (skb->ifnet->protocol != NET_AF_ETH) {
        net_log(skb, ":Bad protocol");
        return -1;
    }

    eth_header_t *header = net_skb_reserve(skb, sizeof(eth_header_t));
    if (header == NULL) {
        net_log(skb, ":Unexpected end of data");
        return -1;
    }
    if (memcmp(header->target, skb->ifnet->hwaddr, ETH_ALEN) != 0 &&
        memcmp(header->target, eth_broadcast, ETH_ALEN) != 0) {
        net_log(skb, ":Wrong recipient");
        return -1;
    }

    // TODO - Firewall -- Black listed MAC
    memcpy(skb->addr, header->source, ETH_ALEN);
    skb->addrlen = ETH_ALEN;

    netstack_t *stack = skb->ifnet->stack;
    net_recv_t recv = eth_receiver(stack, header->protocol);
    if (recv == NULL) {
        net_log(skb, ":Unknown protocol");
        return -1;
    }

    return recv(skb);
}

/* Entry point of the ethernet module */
int eth_setup(netstack_t* stack)
{
    nproto_t* proto = net_protocol(stack, NET_AF_ETH);
    if (proto != NULL)
        return -1;

    proto = kalloc(sizeof(nproto_t));
    eth_info_t *info = kalloc(sizeof(eth_info_t));
    hmp_init(&info->recv_map, 8);
    proto->receive = eth_receive;
    proto->data = info;
    strcpy(proto->name, "eth");
    proto->addrlen = ETH_ALEN;
    proto->paddr = eth_writemac;
    net_set_protocol(stack, NET_AF_ETH, proto);
    return 0;
}

/* Exit point of the ethernet module */
int eth_teardown(netstack_t* stack)
{
    splock_lock(&stack->lock);
    nproto_t* proto = bbtree_search_eq(&stack->protocols, NET_AF_ETH, nproto_t, bnode);
    eth_info_t *info = proto->data;
    if (info->recv_map.count > 0) {
        splock_unlock(&stack->lock);
        return -1;
    }

    bbtree_remove(&stack->protocols, NET_AF_ETH);
    // Warning, no RCU ?
    kfree(proto->data);
    kfree(proto);
    splock_unlock(&stack->lock);
    return 0;
}

EXPORT_SYMBOL(eth_writemac, 0);
EXPORT_SYMBOL(eth_handshake, 0);
EXPORT_SYMBOL(eth_header, 0);

// EXPORT_MODULE(eth, eth_setup, eth_teardown);
