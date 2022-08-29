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

static void icmp_forget_with(ip4_info_t *info, net_qry_t *qry)
{
    splock_lock(&info->qry_lock);
    bbtree_remove(&info->qry_pings, qry->bnode.value_);
    splock_unlock(&info->qry_lock);
}

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
        ip4_route_t route;
        memcpy(route.ip, &skb->addr[skb->addrlen - IP4_ALEN], IP4_ALEN);
        route.net = skb->ifnet;
        if (route.net->protocol == NET_AF_ETH)
            memcpy(route.addr, skb->addr, ETH_ALEN);
        else if (route.net->protocol == NET_AF_LO)
            return -1;
        icmp_packet(skb->ifnet, &route, ICMP_PONG, 0, header->data, buf, length);

    } else if (header->type == ICMP_PONG) {
        ip4_info_t *info = ip4_readinfo(skb->ifnet);
        // uint16_t id = header->data & 0xFFFF;
        // uint32_t data = header->data;
        splock_lock(&info->qry_lock);
        net_qry_t *qry = bbtree_search_eq(&info->qry_pings, header->data, net_qry_t, bnode);
        if (qry != NULL)
            bbtree_remove(&info->qry_pings, qry->bnode.value_);
        splock_unlock(&info->qry_lock);

        if (qry != NULL) {
            // Check the payload!
            bool match = true;
            if (qry->len > 0) {
                unsigned len = MIN3(qry->len, length, NET_MAX_RESPONSE);
                match = length == len && buf != NULL && memcmp(qry->res, buf, len) == 0;
                qry->len = len;
                if (buf != NULL)
                    memcpy(qry->res, buf, len);
                else
                    qry->len = 0;
            }
            mtx_lock(&qry->mtx);
            qry->received = true;
            qry->success = match;
            cnd_signal(&qry->cnd);
            mtx_unlock(&qry->mtx);
        }
    }

    kfree(skb);
    return 0;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

/* Send a PING request to the following IP route */
int icmp_ping(ip4_route_t *route, const char *buf, unsigned len, net_qry_t *qry)
{
    if (route == NULL)
        return -1;
    uint16_t id = rand16();
    short seq = 1;
    uint32_t data = id | (seq << 16);
    ip4_info_t *info = ip4_readinfo(route->net);

    if (qry != NULL) {
        memset(qry, 0, sizeof(net_qry_t));
        cnd_init(&qry->cnd);
        mtx_init(&qry->mtx, mtx_plain);
        qry->start = xtime_read(XTIME_CLOCK);
        if (len > 0) {
            len = MIN(len, NET_MAX_RESPONSE);
            qry->len = len;
            memcpy(qry->res, buf, len);
        }

        splock_lock(&info->qry_lock);
        qry->bnode.value_ = data;
        bbtree_insert(&info->qry_pings, &qry->bnode);
        splock_unlock(&info->qry_lock);
        mtx_lock(&qry->mtx);
    }

    int ret = icmp_packet(route->net, route, ICMP_PING, 0, data, buf, len);
    if (ret == 0)
        return 0;

    if (qry != NULL) {
        mtx_unlock(&qry->mtx);
        icmp_forget_with(info, qry);
    }

    return -1;
}

/* Unregistered a timedout ARP query */
void icmp_forget(ip4_route_t *route, net_qry_t *qry)
{
    ip4_info_t *info = ip4_readinfo(route->net);
    assert(info != NULL && qry != NULL);
    icmp_forget_with(info, qry);
}

