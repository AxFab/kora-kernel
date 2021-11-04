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
#include <threads.h>
#include <kernel/core.h>

typedef struct icmp_header icmp_header_t;

struct icmp_info
{
    hmap_t pings;
    splock_t lock;
};

PACK(struct icmp_header {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint32_t data;
});

struct icmp_ping {
    mtx_t mtx;
    cnd_t cnd;
    uint32_t data;
    uint16_t id;
    xtime_t time;
    char *buf;
    int len;
    bool received;
};

#define ICMP_PONG 0
#define ICMP_PING 8

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

/* Gets ICMP information linked to a specific device */
static icmp_info_t *icmp_readinfo(ifnet_t *ifnet)
{
    ip4_info_t *ipinfo = ip4_readinfo(ifnet);
    assert(ipinfo != NULL);
    icmp_info_t *info = ipinfo->icmp;
    if (info == NULL) {
        info = kalloc(sizeof(icmp_info_t));
        hmp_init(&info->pings, 8);
        ipinfo->icmp = info;
    }
    return info;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

/* Build and send an ICMP packet from provided parameters */
static int icmp_packet(ifnet_t *ifnet, ip4_route_t *route, int type, int code, uint32_t data, const char *buf, int len)
{
    assert(ifnet != NULL && ifnet->stack != NULL && ifnet->proto != NULL);
    skb_t *skb = net_packet(ifnet);
    if (unlikely(skb == NULL))
        return -1;
    if (ip4_header(skb, route, IP4_ICMP, len + sizeof(icmp_header_t), 0, 0) != 0)
        return net_skb_trash(skb);

    net_log(skb, ",icmp");
    icmp_header_t *header = net_skb_reserve(skb, sizeof(icmp_header_t));
    if (unlikely(header == NULL)) {
        net_log(skb, ":Unexpected end of data");
        return -1;
    }

    header->type = type;
    header->code = code;
    header->data = data;
    header->checksum = 0;
    header->checksum = ip4_checksum((void*)header, sizeof(icmp_header_t));
    if (len > 0)
        net_skb_write(skb, buf, len);
    return net_skb_send(skb);
}

/* Handle an ICMP package and respond if required */
int icmp_receive(skb_t *skb, unsigned length)
{
    assert(skb && skb->ifnet);
    net_log(skb, ",icmp");
    icmp_header_t *header = net_skb_reserve(skb, sizeof(icmp_header_t));
    if (header == NULL) {
        net_log(skb, ":Unexpected end of data");
        return -1;
    }

    length -= sizeof(icmp_header_t);
    char *buf = NULL;
    if (length > 0)
        buf = net_skb_reserve(skb, length);

    if (header->type == ICMP_PING) {
        // TODO - Find who to send it to ?
        // icmp_packet(skb->ifnet, NULL, ICMP_PONG, 0, header->data, buf, length);
    } else if (header->type == ICMP_PONG) {
        icmp_info_t *info = icmp_readinfo(skb->ifnet);
        uint16_t id = header->data & 0xFFFF;
        icmp_ping_t *ping = hmp_get(&info->pings, (char *)&id, sizeof(uint16_t));
        if (ping != NULL) {
            // TODO - Check the payload!
            mtx_lock(&ping->mtx);
            splock_lock(&info->lock);
            hmp_remove(&info->pings, (char *)&id, sizeof(uint16_t));
            splock_unlock(&info->lock);
            ping->received = true;
            cnd_signal(&ping->cnd);
            mtx_unlock(&ping->mtx);
        }
    }

    kfree(skb);
    return 0;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

/* Send a PING request to the following IP route */
icmp_ping_t *icmp_ping(ip4_route_t *route, const char *buf, unsigned len)
{
    uint16_t id = rand16();
    short seq = 1;
    uint32_t data = id | (seq << 16);
    icmp_ping_t *ping = kalloc(sizeof(icmp_ping_t));
    if (len > 0) {
        ping->len = len;
        ping->buf = kalloc(len);
        memcpy(ping->buf, buf, len);
    }
    ping->id = id;
    ping->data = data;
    ping->time = xtime_read(XTIME_CLOCK);
    ping->received = false;
    mtx_init(&ping->mtx, mtx_plain);
    cnd_init(&ping->cnd);
    icmp_info_t *info = icmp_readinfo(route->net);
    splock_lock(&info->lock);
    hmp_put(&info->pings, (char *)&ping->id, sizeof(uint16_t), ping);
    splock_unlock(&info->lock);
    int ret = icmp_packet(route->net, route, ICMP_PING, 0, data, buf, len);
    if (ret == 0)
        return ping;

    splock_lock(&info->lock);
    hmp_remove(&info->pings, (char *)&id, sizeof(uint16_t));
    splock_unlock(&info->lock);
    kfree(ping->buf);
    kfree(ping);
    return NULL;
}
