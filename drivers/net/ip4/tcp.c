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

typedef struct tcp_header tcp_header_t;

PACK(struct tcp_header {
    uint16_t src_port;
    uint16_t dest_port;
    uint32_t seq_no;
    uint32_t ack_no;
    uint16_t flags;
    uint16_t window;
    uint16_t checksum;
    uint16_t urgent;
});


/* Look for an ephemeral port on TCP */
uint16_t tcp_ephemeral_port(socket_t *sock)
{
    ip4_master_t *master = ip4_readmaster(sock->stack);
    return ip4_ephemeral_port(master, &master->tcp_ports, sock);
}

/* Look for a socket binded to this port */
static socket_t *tcp_lookfor_socket(ifnet_t *ifnet, uint16_t port)
{
    ip4_master_t *master = ip4_readmaster(ifnet->stack);
    return ip4_lookfor_socket(master, &master->tcp_ports, ifnet, port);
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

/* Write an TCP header on a packet using the provided parameters */
int tcp_header(skb_t *skb, ip4_route_t *route, unsigned length, uint16_t rport, uint16_t lport, uint16_t identifier, uint16_t offset)
{
    if (ip4_header(skb, route, IP4_TCP, length + sizeof(tcp_header_t), identifier, offset) != 0)
        return -1;

    net_log(skb, ",tcp");
    tcp_header_t *header = net_skb_reserve(skb, sizeof(tcp_header_t));
    if (header == NULL) {
        net_log(skb, ":Unexpected end of data");
        return -1;
    }

    memset(header, 0, sizeof(tcp_header_t));
    header->src_port = htons(lport);
    header->dest_port = htons(rport);
    //header->seq_no = seq;
    //header->ack_no = ack;
    //header->flags = flags;
    //header->window = window;
    //header->urgent = urgent;
    header->checksum = 0;
    header->checksum = ip4_checksum((uint16_t *)skb, sizeof(tcp_header_t) + 8);
    // TODO - ip options
    return -1;
}

/* Handle the reception of a TCP packet */
int tcp_receive(skb_t *skb, unsigned length, uint16_t identifier, uint16_t offset)
{
    net_log(skb, ",tcp");
    tcp_header_t *header = net_skb_reserve(skb, sizeof(tcp_header_t));
    if (header == NULL) {
        net_log(skb, ":Unexpected end of data");
        return -1;
    }

    // TODO - ip options
    length -= sizeof(tcp_header_t);

    uint16_t port = htons(header->dest_port);
    ip4_info_t *info = ip4_readinfo(skb->ifnet);

    memcpy(&skb->addr[skb->addrlen], &header->src_port, sizeof(uint16_t));
    skb->addrlen += sizeof(uint16_t);

    socket_t *sock = tcp_lookfor_socket(skb->ifnet, port);
    if (sock == NULL)
        return -1;

    // TODO -- Add sock->data (length / offset / identifier / options)
    net_socket_push(sock, skb);
    return -1;
}

int tcp_socket_bind(socket_t *sock, const uint8_t *addr, size_t len)
{
    if (len < 6)
        return -1;
    ip4_master_t *master = ip4_readmaster(sock->stack);
    return ip4_socket_bind(master, &master->udp_ports, sock, addr);
}

int tcp_socket_connect(socket_t *sock, const uint8_t *addr, size_t len)
{
    ip4_master_t *master = ip4_readmaster(sock->stack);
    // Start handshake
    return -1;
}


int tcp_socket_accept(socket_t *sock, socket_t *model, skb_t *skb)
{
    ip4_master_t *master = ip4_readmaster(sock->stack);
    if (ip4_socket_accept(master, &master->udp_ports, sock, model, skb) != 0)
        return -1;

    // Read skb / must be an handshake...

    return -1;
}


long tcp_socket_send(socket_t* sock, const uint8_t* addr, const uint8_t* buf, size_t len, int flags)
{
    return -1;
}

long tcp_socket_recv(socket_t* sock, uint8_t* addr, uint8_t* buf, size_t len, int flags)
{
    return -1;
}

/* Fill-out the prototype structure for TCP */
void tcp_proto(nproto_t* proto)
{
    proto->addrlen = 6;
    proto->name = "tcp/ipv4";
    proto->bind = tcp_socket_bind;
    // proto->connect = tcp_socket_connect;
    // proto->accept = tcp_socket_accept;
    proto->send = tcp_socket_send;
    proto->recv = tcp_socket_recv;
    // proto->close = tcp_close;
}

