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


static eth_info_t *eth_readinfo(netstack_t *stack)
{
    eth_info_t *info = stack->eth;
    if (info == NULL) {
        splock_lock(&stack->lock);
        info = kalloc(sizeof(eth_info_t));
        stack->eth = info;
        hmp_init(&info->recv_map, 8);
        splock_unlock(&stack->lock);
    }
    return info;
}

static net_recv_t eth_receiver(netstack_t *stack, uint16_t protocol)
{
    eth_info_t *info = eth_readinfo(stack);
    return hmp_get(&info->recv_map, (char *)&protocol, sizeof(uint16_t));
}


char *eth_writemac(const uint8_t *mac, char *buf, int len)
{
    snprintf(buf, len, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return buf;
}

int eth_handshake(netstack_t *stack, uint16_t protocol, net_recv_t recv)
{
    eth_info_t *info = eth_readinfo(stack);
    hmp_put(&info->recv_map, (char *)&protocol, sizeof(uint16_t), recv);
    return 0;
}

int eth_header(skb_t *skb, const uint8_t *addr, uint16_t protocol)
{
    net_skb_log(skb, "eth");
    if (skb->ifnet->protocol != NET_AF_ETH) {
        net_skb_log(skb, ":Bad protocol");
        return -1;
    }

    eth_header_t *header = net_skb_reserve(skb, sizeof(eth_header_t));
    if (header == NULL) {
        net_skb_log(skb, ":Unexpected end of data");
        return -1;
    }
    memcpy(header->target, addr, ETH_ALEN);
    memcpy(header->source, skb->ifnet->hwaddr, ETH_ALEN);
    memcpy(header->target, addr, ETH_ALEN);
    header->protocol = protocol;
    return 0;
}

int eth_receive(skb_t *skb)
{
    net_skb_log(skb, "eth");
    if (skb->ifnet->protocol != NET_AF_ETH) {
        net_skb_log(skb, ":Bad protocol");
        return -1;
    }

    eth_header_t *header = net_skb_reserve(skb, sizeof(eth_header_t));
    if (header == NULL) {
        net_skb_log(skb, ":Unexpected end of data");
        return -1;
    }
    if (memcmp(header->target, skb->ifnet->hwaddr, ETH_ALEN) != 0 &&
        memcmp(header->target, eth_broadcast, ETH_ALEN) != 0) {
        net_skb_log(skb, ":Wrong recipient");
        return -1;
    }

    netstack_t *stack = skb->ifnet->stack;
    // TODO - Firewall -- Black listed MAC
    // TODO - Save remote address ?

    net_recv_t recv = eth_receiver(stack, header->protocol);
    if (recv == NULL) {
        net_skb_log(skb, ":Unknown protocol");
        return -1;
    }

    return recv(skb);
}


EXPORT_SYMBOL(eth_writemac, 0);
EXPORT_SYMBOL(eth_handshake, 0);
EXPORT_SYMBOL(eth_header, 0);

