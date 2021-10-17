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
#ifndef __KERNEL_NET_ETH_H
#define __KERNEL_NET_ETH_H 1

#include "net.h"

#define ETH_ALEN  6

typedef struct eth_header eth_header_t;

struct eth_header {
    uint8_t sender[ETH_ALEN];
    uint8_t target[ETH_ALEN];
    uint16_t type;
};

int eth_receive(skb_t *skb);
skb_t *eth_packet(ifnet_t *ifnet, uint8_t *addr, uint16_t type);

#endif  /* __KERNEL_NET_ETH_H */
