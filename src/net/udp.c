/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015  <Fabien Bavent>
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

typedef struct UDP_header UDP_header_t;

PACK(struct UDP_header {
    uint16_t src_port;
    uint16_t dest_port;
    uint16_t length;
    uint16_t checksum;
});

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int udp_header(skb_t *skb, const uint8_t *ip, int length, int port, int src)
{
    if (ip4_header(skb, ip, 0, 0, length + sizeof(UDP_header_t), IP4_UDP) != 0)
        return -1;
    strncat(skb->log, "udp:", NET_LOG_SIZE);
    UDP_header_t *header = net_pointer(skb, sizeof(UDP_header_t));
    if (header == NULL)
        return -1;
    header->src_port = htonw(src > 0 ? src: net_rand_port());
    header->dest_port = htonw(port);
    header->length = htonw(length);
    header->checksum = 0;
    header->checksum = ip4_checksum(skb, sizeof(UDP_header_t) + 8); /* TODO - ip options */
    return 0;
}

int udp_receive(skb_t *skb, unsigned length)
{
    strncat(skb->log, "udp:", NET_LOG_SIZE);
    UDP_header_t *header = net_pointer(skb, sizeof(UDP_header_t));
    if (header == NULL)
        return -1;
    uint16_t checksum = header->checksum;
    header->checksum = 0;
    if (checksum != ip4_checksum(skb, sizeof(UDP_header_t) + 8)) /* TODO - ip options */
        return -1;
    if (length != htonw(header->length))
        return -1;
    length -= sizeof(UDP_header_t);
    switch (htonw(header->dest_port)) {
        case UDP_PORT_NTP:
            return ntp_receive(skb, length);
        case UDP_PORT_DHCP:
            return dhcp_receive(skb, length);
        case UDP_PORT_DNS:
            return dns_receive(skb, length);
        default:
            // socket_t *socket = net_listener("udp", header.dest_port);
            return -1;
    }
}

int udp_packet(socket_t *socket)
{
    return -1;
}

socket_t *udp_socket(netdev_t *ifnet, uint8_t *ip, int port)
{
    // TODO - do we know the MAC address!
    socket_t *socket = (socket_t*)kalloc(sizeof(socket_t));
    socket->ifnet = ifnet;
    socket->sport = net_ephemeral_port(socket);
    socket->dport = port;
    memcpy(socket->addr, ip, IP4_ALEN);
    socket->packet = udp_packet;
    return socket;
}
