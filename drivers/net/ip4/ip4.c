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

uint16_t ip4_checksum(uint16_t *ptr, unsigned len)
{
    // TODO -- Do checksum the right way!
    int i, sum = 0;
    for (i = -(int)(len / 2); i < 0; ++i)
        sum += ntohs(ptr[i]);
    if (sum > 0xFFFF)
        sum = (sum >> 16) + (sum & 0xFFFF);
    return htons(~(sum & 0xFFFF) & 0xFFFF);
}

int ip4_header(skb_t *skb, ip4_route_t *route, int protocol, unsigned length, uint16_t identifier, uint16_t offset)
{
    switch (route->net->protocol) {
    case NET_AF_ETH:
        if (eth_header(skb, route->addr, ETH_IP4) != 0)
            return -1;
        break;
    default:
        net_log(skb, "ipv4:Unknown protocol");
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
    memcpy(header->source, info->ip, IP4_ALEN);
    memcpy(header->target, route->ip, IP4_ALEN);

    header->checksum = ip4_checksum((uint16_t *)header, sizeof(ip4_header_t));
    return 0;
}


int ip4_receive(skb_t *skb)
{
    net_log(skb, ",ipv4");
    ip4_info_t *info = ip4_readinfo(skb->ifnet);
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

    // TODO -- If that's not for me, should I re-route !?

    // TODO - Save remote address ?
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

