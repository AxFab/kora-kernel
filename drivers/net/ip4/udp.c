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
typedef struct udp_check_header udp_check_header_t;

PACK(struct udp_header {
    uint16_t src_port;
    uint16_t dest_port;
    uint16_t length;
    uint16_t checksum;
});

PACK(struct udp_check_header
{
    uint8_t source[IP4_ALEN];
    uint8_t target[IP4_ALEN];
    uint8_t mbz;
    uint8_t proto;
    uint16_t length;
};)

/* Look for an ephemeral port on UDP */
uint16_t udp_ephemeral_port(socket_t *sock)
{
    ip4_master_t *master = ip4_readmaster(sock->stack);
    return ip4_ephemeral_port(master, &master->udp_ports, sock);
}

/* Look for a socket binded to this port */
static socket_t *udp_lookfor_socket(ifnet_t *ifnet, uint16_t port)
{
    ip4_master_t *master = ip4_readmaster(ifnet->stack);
    return ip4_lookfor_socket(master, &master->udp_ports, ifnet, port);
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

/* Write an UDP header on a packet using the provided parameters */
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
    return 0;
}

uint16_t udp_do_checksum(udp_check_header_t *check, uint16_t *data, size_t len)
{
    int i, sum = 0;
    uint16_t *cptr = check;
    for (i = 0; i < sizeof(udp_check_header_t) / 2; i++)
        sum += ntohs(cptr[i]);
    for (i = 0; i < len / 2; i++)
        sum += ntohs(data[i]);
    if (sum > 0xFFFF)
        sum = (sum >> 16) + (sum & 0xFFFF);
    return htons(~(sum & 0xFFFF) & 0xFFFF);
}

int udp_checksum(skb_t *skb, ip4_route_t *route, unsigned length)
{
    udp_header_t *header = (void *)(&skb->buf[skb->pen - length - sizeof(udp_header_t)]);
    udp_check_header_t check;
    ip4_info_t *info = ip4_readinfo(skb->ifnet);
    memcpy(check.source, info->subnet.address, IP4_ALEN);
    memcpy(check.target, route->ip, IP4_ALEN);
    check.mbz = 0;
    check.length = htons(length + sizeof(udp_header_t));
    check.proto = IP4_UDP;
    // header->checksum = ip4_checksum((uint16_t *)ADDR_OFF(header, -8), sizeof(udp_header_t) + 8);
    // header->checksum = ip4_checksum((uint16_t *)ADDR_OFF(header, -12), sizeof(udp_header_t) + 12);
    header->checksum = udp_do_checksum(&check, (uint16_t *)header, sizeof(udp_header_t) + length);
}

/* Handle the reception of a UDP packet */
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

    memcpy(&skb->addr[skb->addrlen], &header->src_port, sizeof(uint16_t));
    skb->addrlen += sizeof(uint16_t);

    socket_t *sock = udp_lookfor_socket(skb->ifnet, port);
    if (sock == NULL)
        return -1;

    // TODO -- Add sock->data (length / offset / identifier / options)
    return net_socket_push(sock, skb);
}

int udp_socket_bind(socket_t *sock, const uint8_t *addr, size_t len)
{
    if (len < 6)
        return -1;
    ip4_master_t *master = ip4_readmaster(sock->stack);
    return ip4_socket_bind(master, &master->udp_ports, sock, addr);
}


int udp_socket_accept(socket_t *sock, socket_t *model, skb_t *skb)
{
    ip4_master_t *master = ip4_readmaster(sock->stack);
    return ip4_socket_accept(master, &master->udp_ports, sock, model, skb);
}

long udp_socket_send(socket_t* sock, const uint8_t* addr, const uint8_t* buf, size_t len, int flags)
{
    assert(sock && addr && buf);
    
    // Look for a route
    uint16_t rport = ntohs(*((uint16_t *)&addr[4]));
    if (rport == 0)
        return -1;
    ip4_route_t route;
    if (ip4_find_route(sock->stack, &route, addr) != 0)
        return -1;
    uint16_t lport = 0;
    if (sock->flags & NET_ADDR_BINDED)
        lport = htons(*((uint16_t *)&sock->laddr[4]));
    else
        lport = udp_ephemeral_port(sock);
    if (lport == 0)
        return -1;

    // Select an IP identifier
    uint16_t ip_id = 0;

    // Compute the optimal packet size
    unsigned packsize = 1500 - (14 + 20 + 8 + 8); // TOOD -- Better estimation
    unsigned count = len / packsize;
    unsigned rem = len % packsize;
    while (rem > 0 && rem < count && (packsize - rem) > count) {
        packsize--;
        count = len / packsize;
        rem = len % packsize;
    }

    uint16_t off = 0;
    while (len > 0) {
        unsigned pack = MIN(packsize, len);
        skb_t* skb = net_packet(route.net);
        if (udp_header(skb, &route, pack, rport, lport, ip_id, off) != 0)
            return net_skb_trash(skb);

        void* ptr = net_skb_reserve(skb, pack);
        if (ptr == NULL)
            return net_skb_trash(skb);

        memcpy(ptr, buf, pack);
        udp_checksum(skb, &route, pack);
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
    while (len > 0) {
        skb_t *skb = net_socket_pull(sock, addr, 6, -1);
        if (skb == NULL)
            continue;
    
        unsigned lg = skb->length - skb->pen;
        void *ptr = net_skb_reserve(skb, lg);
        if (len < lg) {
            kfree(skb);
            return -1;
        }
        memcpy(buf, ptr, lg);
        // Can we continue!?
        kfree(skb);
        return lg;
    }
    // Wait for recv packet (sleep on sock->incm - may be use a semaphore !?);
    // If data are available read it -- Build and store ip4 route !?
    return -1;
}

int udp_socket_close(socket_t *sock)
{
    ip4_master_t *master = ip4_readmaster(sock->stack);
    // Unbind the socket
    if (sock->flags & NET_ADDR_BINDED) {
        if (ip4_socket_close(master, &master->udp_ports, sock) != 0)
            return -1;
    }
    
    // Trash all unprocessed rx packet 
    while (sock->lskb.count_ > 0) {
        skb_t *skb = ll_dequeue(&sock->lskb, skb_t, node);
        kfree(skb);
    }

    return 0;
}



/* Fill-out the prototype structure for UDP */
void udp_proto(nproto_t* proto)
{
    proto->addrlen = 6;
    proto->name = "udp/ipv4";
    proto->bind = udp_socket_bind;
    // TODO -- Connect at least to check this is a valid address!
    proto->accept = udp_socket_accept;
    proto->send = udp_socket_send;
    proto->recv = udp_socket_recv;
    proto->close = udp_socket_close;
}

