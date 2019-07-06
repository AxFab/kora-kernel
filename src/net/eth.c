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
#include <string.h>

const uint8_t eth_broadcast[ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

int eth_header(skb_t *skb, const uint8_t *target, uint16_t type)
{
    strncat(skb->log, "eth:", NET_LOG_SIZE);
    net_write(skb, target, ETH_ALEN);
    net_write(skb, skb->ifnet->eth_addr, ETH_ALEN);
    memcpy(skb->eth_addr, target, ETH_ALEN);
    return net_write(skb, &type, 2);
}

int eth_receive(skb_t *skb)
{
    uint16_t type = 0;
    uint8_t *mac;

    strncat(skb->log, "eth:", NET_LOG_SIZE);
    mac = net_pointer(skb, ETH_ALEN);
    if (mac == NULL)
        return -1;
    if (memcmp(mac, skb->ifnet->eth_addr, ETH_ALEN) != 0 &&
        memcmp(mac, eth_broadcast, ETH_ALEN) != 0)
        return -1;
    net_read(skb, skb->eth_addr, ETH_ALEN);
    if (net_read(skb, &type, 2))
        return -1;
    switch (type) {
    case ETH_IP4:
        // return ip4_receive(skb);
    case ETH_ARP:
        // return arp_receive(skb);
    case ETH_IP6:
    default:
        return -1;
    }
}

