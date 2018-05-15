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
static int udp_checksum(skb_t *skb, UDP_header_t *header)
{
    return 0;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


int udp_header(skb_t *skb, const uint8_t *ip, int length, int port, int src)
{
    if (ip4_header(skb, ip, 0, 0, length + sizeof(UDP_header_t), IP4_UDP) != 0)
        return -1;
    strncat(skb->log, "udp:", NET_LOG_SIZE);
    UDP_header_t header;
    header.src_port = htonw(src > 0 ? src: net_rand_port());
    header.dest_port = htonw(port);
    header.length = htonw(length);
    header.checksum = udp_checksum(skb, &header);
    return net_write(skb, &header, sizeof(header));
}

int udp_receive(skb_t *skb, unsigned length)
{
    UDP_header_t header;
    strncat(skb->log, "udp:", NET_LOG_SIZE);
    if (net_read(skb, &header, sizeof(header)) != 0)
        return -1;
    // TODO check checksum, and length
    switch (header.dest_port) {
        case UDP_PORT_NTP:
            return ntp_receive(skb, header.length);
        case UDP_PORT_DHCP:
            return dhcp_receive(skb, header.length);
        case UDP_PORT_DNS:
            return dns_receive(skb, header.length);
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
    // TODO - does we know the MAC address!
    socket_t *socket = (socket_t*)kalloc(sizeof(socket_t));
    socket->ifnet = ifnet;
    socket->sport = net_ephemeral_port(socket);
    socket->dport = port;
    memcpy(socket->addr, ip, IP4_ALEN);
    socket->packet = udp_packet;
    return socket;
}
