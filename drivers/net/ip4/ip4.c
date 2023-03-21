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
#include <kernel/core.h>
#include "ip4.h"

typedef struct ip4_header ip4_header_t;

PACK(struct ip4_header {
    uint8_t header_length : 4;
    uint8_t version : 4;
    uint8_t service_type;
    uint16_t length;
    uint16_t identifier;
    uint16_t offset;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint8_t source[IP4_ALEN];
    uint8_t target[IP4_ALEN];
});

const uint8_t eth_broadcast[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

/* Compute the checksum of a IP4 header */
uint16_t ip4_checksum(uint16_t *ptr, unsigned len)
{
    unsigned i, sum = 0;
    for (i = 0; i < len / 2; i++)
        sum += ntohs(ptr[i]);
    if (sum > 0xFFFF)
        sum = (sum >> 16) + (sum & 0xFFFF);
    return htons(~(sum & 0xFFFF) & 0xFFFF);
}

/* Write a IP header on the socket buffer */
int ip4_header(skb_t *skb, ip4_route_t *route, int protocol, unsigned length, uint16_t identifier, uint16_t offset)
{
    assert(route && route->net && route->net->proto);
    switch (route->net->protocol) {
    case NET_AF_ETH:
        if (eth_header(skb, route->addr, ETH_IP4) != 0)
            return -1;
        break;
    case NET_AF_LO:
        if (lo_header(skb, ETH_IP4, 0) != 0)
            return -1;
        break;
    default:
        net_log(skb, ":ipv4:Unknown protocol");
        return -1;
    }

    net_log(skb, ",ipv4");
    ip4_info_t *info = ip4_readinfo(skb->ifnet);
    ip4_header_t *header = net_skb_reserve(skb, sizeof(ip4_header_t));
    if (header == NULL) {
        net_log(skb, ":Unexpected end of data");
        return -1;
    }

    memset(header, 0, sizeof(ip4_header_t));
    header->version = 4; // Ipv4
    header->header_length = 5; // No option
    header->service_type = 0; // No features
    header->length = htons(sizeof(ip4_header_t) + length);
    header->identifier = identifier;
    header->offset = offset;
    header->ttl = route->ttl;
    header->protocol = protocol;
    // TODO -- Check subnet is configured
    memcpy(header->source, info->subnet.address, IP4_ALEN);
    memcpy(header->target, route->ip, IP4_ALEN);

    header->checksum = ip4_checksum((uint16_t *)header, sizeof(ip4_header_t));
    return 0;
}

/* Handle the reception of an IP4 packet */
int ip4_receive(skb_t *skb)
{
    net_log(skb, ",ipv4");
    // ip4_info_t *info = ip4_readinfo(skb->ifnet);
    ip4_header_t *header = net_skb_reserve(skb, sizeof(ip4_header_t));
    if (header == NULL) {
        net_log(skb, ":Unexpected end of data");
        return -1;
    }

    uint16_t checksum = header->checksum;
    uint16_t length = htons(header->length) - sizeof(ip4_header_t);
    header->checksum = 0;

    header->checksum = ip4_checksum((uint16_t *)header, sizeof(ip4_header_t));
    if (checksum != header->checksum) {
        net_log(skb, ":Bad checksum");
        return -1;
    }

    // TODO -- Check I'm the target 
    // TODO -- If router is configure, reroute the package (TTL-1)
    // TODO -- Save remote address ?
    memcpy(&skb->addr[skb->addrlen], header->source, IP4_ALEN);
    skb->addrlen += IP4_ALEN;

    // TODO -- Check if ithat's a raw socket
    //socket_t *sock = ip4_lookfor_socket(skb->ifnet, 0, header->protocol, NULL);
    //if (sock != NULL)
    //    return net_socket_push(sock, skb);

    // Transfer to upper layer protocol
    switch (header->protocol) {
    case IP4_TCP:
        return tcp_receive(skb, length, header->identifier, header->offset);
    case IP4_UDP:
        return udp_receive(skb, length, header->identifier, header->offset);
    case IP4_ICMP:
        return icmp_receive(skb, length);
    default:
        net_log(skb, ":Unknown protocol");
        return -1;
    }
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

int ip4_socket(socket_t *sock, int method)
{
    return -1;
}

long ip4_socket_send(socket_t *sock, const uint8_t *addr, const char *buf, size_t len, int flags)
{
    return -1;
}

long ip4_socket_recv(socket_t *sock, uint8_t *addr, char *buf, size_t len, int flags)
{
    return -1;
}


int ip4_clear(netstack_t * stack, ifnet_t * ifnet)
{
    ip4_master_t *master = ip4_readmaster(stack);
    ip4_info_t *info = ip4_readinfo(ifnet);
    if (info->qry_arps.count_ > 0)
        return -1; // TODO -- Forget them all...
    if (info->qry_pings.count_ > 0)
        return -1; // TODO -- Forget them all...
    if (info->dhcp != NULL)
        dhcmp_clear(ifnet, info);

    char key[8];
    int lg = snprintf(key, 8, "%s.%d", ifnet->proto->name, ifnet->idx);
    splock_lock(&master->lock);
    if (ll_contains(&master->subnets, &info->subnet.node))
        ll_remove(&master->subnets, &info->subnet.node);
    hmp_remove(&master->ifinfos, key, lg);
    splock_unlock(&master->lock);
    kfree(info);
    return 0;
}

/* Fill-out the prototype structure for IP4 */
void ip4_proto(nproto_t *proto)
{
    proto->addrlen = 6;
    proto->name = "ipv4";
    proto->socket = ip4_socket;
    // proto->bind = ip4_tcp_bind;
    // proto->connect = ip4_tcp_connect;
    proto->send = ip4_socket_send;
    proto->recv = ip4_socket_recv;
    proto->clear = ip4_clear;
    // proto->close = ip4_tcp_close;
}


