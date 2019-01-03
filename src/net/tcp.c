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

typedef struct TCP_header TCP_header_t;

PACK(struct TCP_header {
    uint16_t src_port;
    uint16_t dest_port;
    uint32_t seq_no;
    uint32_t ack_no;
    uint16_t flags;
    uint16_t window;
    uint16_t checksum;
    uint16_t urgent;
});

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
static int tcp_checksum(skb_t *skb, TCP_header_t *header)
{
    return 0;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


int tcp_header(skb_t *skb, const uint8_t *ip, int length, int port, int src)
{
    if (ip4_header(skb, ip, 0, 0, length + sizeof(TCP_header_t), IP4_TCP) != 0)
        return -1;
    strncat(skb->log, "tcp:", NET_LOG_SIZE);
    TCP_header_t header;
    header.src_port = htonw(src > 0 ? src : net_rand_port());
    header.dest_port = htonw(port);
    header.checksum = tcp_checksum(skb, &header);
    return net_write(skb, &header, sizeof(header));
}

int tcp_receive(skb_t *skb, unsigned length)
{
    TCP_header_t header;
    strncat(skb->log, "tcp:", NET_LOG_SIZE);
    if (net_read(skb, &header, sizeof(header)) != 0)
        return -1;
    // TODO check checksum, and length
    switch (header.dest_port) {
    default:
        // socket_t *socket = net_listener("tcp", header.dest_port);
        return -1;
    }
}

int tcp_packet(socket_t *socket)
{
    return -1;
}

socket_t *tcp_socket(netdev_t *ifnet, uint8_t *ip, int port)
{
    // TODO - do we know the MAC address!
    socket_t *socket = (socket_t *)kalloc(sizeof(socket_t));
    socket->ifnet = ifnet;
    socket->sport = net_ephemeral_port(socket);
    socket->dport = port;
    memcpy(socket->addr, ip, IP4_ALEN);
    socket->packet = tcp_packet;
    return socket;
}
