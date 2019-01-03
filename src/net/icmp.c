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

typedef struct ICMP_header ICMP_header_t;

PACK(struct ICMP_header {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint32_t data;
});

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

static uint16_t icmp_checksum(ICMP_header_t *header)
{
    return 0;
}

static int icmp_packet(netdev_t *ifnet, const uint8_t *ip, uint8_t type,
                       uint8_t code, uint32_t data, void *extra, int len)
{
    skb_t *skb = net_packet(ifnet, 128);
    if (skb == NULL)
        return -1;
    if (ip4_header(skb, ip, rand(), 0, sizeof(ICMP_header_t) + len, IP4_ICMP) != 0)
        return net_trash(skb);
    strncat(skb->log, "icmp:", NET_LOG_SIZE);
    ICMP_header_t header;
    header.type = type;
    header.code = code;
    header.data = data;
    header.checksum = icmp_checksum(&header);
    net_write(skb, &header, sizeof(header));
    if (len > 0)
        net_write(skb, extra, len);
    return net_send(skb);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int icmp_ping(netdev_t *ifnet, const uint8_t *ip)
{
    short id = rand();
    short seq = 1;
    return icmp_packet(ifnet, ip, ICMP_PING, 0, id | (seq << 16),
                       "abcdefghijklmnop", 16);
}

int icmp_receive(skb_t *skb, unsigned len)
{
    int ret;
    uint8_t *payload = NULL;
    strncat(skb->log, "icmp:", NET_LOG_SIZE);
    ICMP_header_t header;
    len -= sizeof(header);
    ret = net_read(skb, &header, sizeof(header));
    if (len > 0) {
        payload = kalloc(len);
        ret = net_read(skb, payload, len);
    }
    if (ret != 0)
        return -1;
    switch (header.type) {
    case ICMP_PONG:
        // get last request clock
        break;
    case ICMP_PING:
        ret = icmp_packet(skb->ifnet, skb->ip4_addr, ICMP_PONG, 0, header.data, payload,
                          len);
        break;
    // case ICMP_TIMESTAMP:
    default:
        ret = -1;
    }

    if (payload)
        kfree(payload);
    return ret;
}

