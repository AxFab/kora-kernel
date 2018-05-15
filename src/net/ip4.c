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


typedef struct IP4_header IP4_header_t;

PACK(struct IP4_header {
    uint8_t header_length:4;
    uint8_t version:4;
    uint8_t service_type;
    uint16_t length;
    uint16_t identifier;
    uint16_t offset;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint8_t source[4];
    uint8_t target[4];
});

const uint8_t ip4_broadcast[IP4_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF };

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

static uint16_t ip4_checksum(IP4_header_t *header)
{
    int i, sum = 0;
    uint16_t *ptr = (uint16_t *)header;
    for (i = 0; i < 10; ++i)
        sum += ntohw(ptr[i]);
    if (sum > 0xFFFF)
        sum = (sum >> 16) + (sum & 0xFFFF);
    return htonw(~(sum & 0xFFFF) & 0xFFFF);
}


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int ip4_header(skb_t *skb, const uint8_t *ip, int identifier, int offset, int length, int protocol)
{
    uint8_t mac [ETH_ALEN];
    if (ip == ip4_broadcast)
        memset(mac, 0xFF, ETH_ALEN);
    // todo, findac address using ip
    // memcpy(mac, !?, ETH_ALEN);
    if (eth_header(skb, mac, ETH_IP4) != 0)
        return -1;

    strncat(skb->log, "ipv4:", NET_LOG_SIZE);
    IP4_header_t header;
    header.version = 4;
    header.header_length = 5;
    header.service_type = 0;
    header.length = htonw(sizeof(header) + length);
    header.identifier = identifier;
    header.offset = offset;
    header.ttl = 128;
    header.protocol = protocol;

    memcpy(header.source, skb->ifnet->ip4_addr, IP4_ALEN);
    memcpy(header.target, ip, IP4_ALEN);
    memcpy(skb->ip4_addr, ip, IP4_ALEN);
    // TODO options
    header.checksum = ip4_checksum(&header);
    return net_write(skb, &header, sizeof(header));
}


int ip4_receive(skb_t *skb)
{
    IP4_header_t header;
    strncat(skb->log, "ipv4:", NET_LOG_SIZE);
    net_read(skb, &header, sizeof(header));
    memcpy(skb->ip4_addr, header.source, IP4_ALEN);
    // net_register(skb->ip4_addr,  skb->eth_addr, ANONYMOUS);
    // CHECK CHECKSUM
    // REGROUP SOCKET
    switch (header.protocol) {
    case IP4_TCP:
        return tcp_receive(skb, header.length - sizeof(header));
    case IP4_UDP:
        return udp_receive(skb, header.length - sizeof(header));
    case IP4_ICMP:
        if (header.offset != 0)
            return -1;
        return icmp_receive(skb, header.length - sizeof(header));
    default:
        return -1;
    }
}


