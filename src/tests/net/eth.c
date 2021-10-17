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
#include "eth.h"


skb_t *eth_packet(ifnet_t *ifnet, uint8_t *addr, uint16_t type)
{
    skb_t *skb = net_packet(ifnet);
    eth_header_t *head = net_pointer(skb, sizeof(eth_header_t));
    memcpy(head->sender, ifnet->hwaddr, ETH_ALEN);
    memcpy(head->target, addr, ETH_ALEN);
    head->type = type;
    strcat(skb->log, ".eth");
    return skb;
}


int eth_receive(skb_t *skb)
{
    eth_header_t *head = net_pointer(skb, sizeof(eth_header_t));
    // TODO - AIO
    inet_recv_t recv = NULL;
    if (recv == NULL)
        return -1;
    return recv(skb);
}
