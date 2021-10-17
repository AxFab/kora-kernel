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
#include "ip4.h"

typedef struct udp_header udp_header_t;

PACK(struct udp_header {
    uint16_t src_port;
    uint16_t dest_port;
    uint16_t length;
    uint16_t checksum;
});


int udp_header(skb_t *skb, ip4_route_t *route, int length, int port, int src, uint16_t identifier, uint16_t offset)
{
    if (ip4_header(skb, route, IP4_UDP, length + sizeof(udp_header_t), identifier, offset) != 0)
        return -1;

    net_skb_log(skb, ",udp");
    udp_header_t *header = net_skb_reserve(skb, sizeof(udp_header_t));
    if (header == NULL) {
        net_skb_log(skb, ":Unexpected end of data");
        return -1;
    }

    header->src_port = htonw(src);
    header->dest_port = htonw(port);
    header->length = htonw(length + sizeof(udp_header_t));
    header->checksum = 0;
    header->checksum = ip4_checksum(skb, sizeof(udp_header_t) + 8);
    // TODO - ip options
    return 0;
}


int udp_receive(skb_t *skb, int length, uint16_t identifier, uint16_t offset)
{
    net_skb_log(skb, ",udp");
    udp_header_t *header = net_skb_reserve(skb, sizeof(udp_header_t));
    if (header == NULL) {
        net_skb_log(skb, ":Unexpected end of data");
        return -1;
    }

    // TODO - ip options
    if (length != htonw(header->length))
        return -1;
    length -= sizeof(udp_header_t);

    uint16_t port = htonw(header->dest_port);
    ip4_info_t *info = ip4_readinfo(skb->ifnet);

    if ((port == IP4_PORT_DHCP || port == IP4_PORT_DHCP_SRV) && info->use_dhcp)
        return dhcp_receive(skb, length);
    //if (port == IP4_PORT_NTP && info->use_ntp)
    //    return ntp_receive(skb, length);

    socket_t *socket = ip4_lookfor_socket(skb->ifnet, port, false, NULL);
    if (socket == NULL)
        return -1;
    return net_socket_recv(socket, skb, length);
}

int udp_socket(socket_t *socket)
{
    //    socket->connect = udp_connect;
    //    socket->bind = udp_bind;
    //    socket->accept = udp_accept;
    //    socket->listen = udp_listen;
    //    socket->send = udp_send;
    return -1;
}
