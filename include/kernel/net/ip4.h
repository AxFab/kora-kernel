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
#ifndef __KERNEL_NET_IP4_H
#define __KERNEL_NET_IP4_H 1

#include <kernel/net.h>

#define IP4_ALEN  4

typedef struct ip4_route ip4_route_t;
typedef struct ip4_frame ip4_frame_t;


void ip4_setup();

int ip4_receive(skb_t *skb);
skb_t *ip4_packet(ifnet_t *ifnet, ip4_route_t *route, ip4_frame_t *frame);
skb_t *ip4_sock_packet(socket_t *sock);
int ip4_connect(socket_t *socket, uint8_t *ip);
void ip4_config(ifnet_t *ifnet, uint8_t *addr, uint8_t *mask, uint8_t *gateway);




#endif  /* __KERNEL_NET_IP4_H */
