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
#include <kernel/net/eth.h>
#include <string.h>

const uint8_t eth_broadcast[ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };


skb_t *eth_packet(ifnet_t *ifnet, const uint8_t *addr, uint16_t type)
{
    skb_t *skb = net_packet(ifnet);
    eth_header_t *head = net_pointer(skb, sizeof(eth_header_t));
    memcpy(head->sender, ifnet->hwaddr, ETH_ALEN);
    memcpy(head->target, addr, ETH_ALEN);
    head->type = type;
    strncat(skb->log, "eth:", NET_LOG_SIZE);
    return skb;
}


int eth_receive(skb_t *skb)
{
    eth_header_t *head = net_pointer(skb, sizeof(eth_header_t));
    strncat(skb->log, "eth:", NET_LOG_SIZE);
    if (memcmp(head->target, skb->ifnet->hwaddr, ETH_ALEN) != 0 &&
        memcmp(head->target, eth_broadcast, ETH_ALEN) != 0)
        return -1;
    // TODO - Firewall
    inet_recv_t recv = NULL;
    if (recv == NULL)
        return -1;
    return recv(skb);
}

