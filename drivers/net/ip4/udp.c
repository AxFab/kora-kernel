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


int udp_header(skb_t *skb, ip4_route_t *route, unsigned length, uint16_t rport, uint16_t lport, uint16_t identifier, uint16_t offset)
{
    if (ip4_header(skb, route, IP4_UDP, length + sizeof(udp_header_t), identifier, offset) != 0)
        return -1;

    net_log(skb, ",udp");
    udp_header_t *header = net_skb_reserve(skb, sizeof(udp_header_t));
    if (header == NULL) {
        net_log(skb, ":Unexpected end of data");
        return -1;
    }

    header->src_port = htons(lport);
    header->dest_port = htons(rport);
    header->length = htons(length + sizeof(udp_header_t));
    header->checksum = 0;
    header->checksum = ip4_checksum((uint16_t *)skb->buf, sizeof(udp_header_t) + 8);
    // TODO - ip options
    return 0;
}


int udp_receive(skb_t *skb, unsigned length, uint16_t identifier, uint16_t offset)
{
    net_log(skb, ",udp");
    udp_header_t *header = net_skb_reserve(skb, sizeof(udp_header_t));
    if (header == NULL) {
        net_log(skb, ":Unexpected end of data");
        return -1;
    }

    // TODO - ip options
    if (length != htons(header->length))
        return -1;
    length -= sizeof(udp_header_t);

    uint16_t port = htons(header->dest_port);
    ip4_info_t *info = ip4_readinfo(skb->ifnet);

    if ((port == IP4_PORT_DHCP || port == IP4_PORT_DHCP_SRV) && info->use_dhcp)
        return dhcp_receive(skb, length);
    //if (port == IP4_PORT_NTP && info->use_ntp)
    //    return ntp_receive(skb, length);

    socket_t *socket = ip4_lookfor_socket(skb->ifnet, port, false, NULL);
    if (socket == NULL)
        return -1;
    return -1;
    // net_socket_psuh(socket, skb, identifier, offset, length);
}


long udp_socket_send(socket_t* sock, const uint8_t* addr, const uint8_t* buf, size_t len, int flags)
{
    // First find a ip4 route !
    // Check if we already have a port -- if not take ephemeral UDP port !
    // Select an IP identifier
    ip4_route_t* route = ip4_route(sock->stack, addr);
    uint16_t ip_id = 0;
    uint16_t lport = 0;
    uint16_t rport = ntohs((addr[4] << 8) | (addr[5]));

    unsigned packsize = 1500 - (14 + 20 + 8 + 8); // route->head_size;
    int count = len / packsize;
    int rem = len % packsize;
    while (rem > 0 && rem < count) {
        packsize--;
        count = len / packsize;
        rem = len % packsize;
    }

    if (route == NULL || lport == 0 || rport == 0)
        return -1;

    uint16_t off = 0;
    while (len > 0) {
        unsigned pack = MIN(packsize, len);
        skb_t* skb = net_packet(route->net);
        if (udp_header(skb, route, pack, rport, lport, ip_id, off) != 0)
            return net_skb_trash(skb);

        void* ptr = net_skb_reserve(skb, pack);
        if (ptr == NULL)
            return net_skb_trash(skb);

        memcpy(ptr, buf, pack);
        if (net_skb_send(skb) != 0)
            return -1;
        len -= pack;
        buf += pack;
        off++;
    }
    return 0;
}

long udp_socket_recv(socket_t* sock, uint8_t* addr, uint8_t* buf, size_t len, int flags)
{
    // Wait for recv packet (sleep on sock->incm - may be use a semaphore !?);
    // If data are available read it -- Build and store ip4 route !?
    return -1;
}


void udp_proto(nproto_t* proto)
{
    proto->addrlen = 6;
    proto->send = udp_socket_send;
    proto->recv = udp_socket_recv;
    // proto->close = udp_close;
}

