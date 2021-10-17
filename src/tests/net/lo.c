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
#include "net.h"


skb_t *lo_packet(ifnet_t *ifnet, uint16_t type)
{
    skb_t *skb = net_packet(ifnet);
    uint16_t *ptr = net_pointer(skb, sizeof(uint16_t));
    *ptr = type;
    strcat(skb->log, ".lo");
    return skb;
}

int lo_receive(skb_t *skb)
{
    uint16_t *ptr = net_pointer(skb, sizeof(uint16_t));
    // TODO - Firewall
    inet_recv_t recv = NULL;
    if (recv == NULL)
        return -1;
    return recv(skb);
}
