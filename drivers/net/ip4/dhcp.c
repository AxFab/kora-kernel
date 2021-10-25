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
#include <time.h>
#include <kernel/core.h>

#define BOOT_REQUEST 1
#define BOOT_REPLY 2

#define DHCP_MAGIC  htonl(0x63825363)

#define DHCP_DISCOVER 1
#define DHCP_OFFER 2
#define DHCP_REQUEST 3
#define DHCP_DECLINE 4
#define DHCP_PACK 5
#define DHCP_PNACK 6
#define DHCP_RELEASE 7
#define DHCP_INFORM 8

#define DHCP_OPT_SUBNETMASK 1
#define DHCP_OPT_ROUTER 3
#define DHCP_OPT_DNSIP 6
#define DHCP_OPT_HOSTNAME 12
#define DHCP_OPT_DOMAIN 15
#define DHCP_OPT_BROADCAST 28
#define DHCP_OPT_QRYIP 50
#define DHCP_OPT_LEASETIME 51
#define DHCP_OPT_MSGTYPE 53
#define DHCP_OPT_SERVERIP 54
#define DHCP_OPT_QRYLIST 55
#define DHCP_OPT_RENEWALTIME 58
#define DHCP_OPT_REBINDINGTIME 59
#define DHCP_OPT_VENDOR 60
#define DHCP_OPT_CLIENTMAC 61

typedef struct dhcp_header dhcp_header_t;
typedef struct dhcp_msg dhcp_msg_t;
typedef struct dhcp_lease dhcp_lease_t;

struct dhcp_info {
    int mode;
    uint32_t transaction;
    xtime_t last_request;
    xtime_t expire;
    xtime_t renewal;
    splock_t lock;

    dhcp_lease_t *lease_range[16];
};


PACK(struct dhcp_header {
    uint8_t opcode;
    uint8_t htype;
    uint8_t hlen;
    uint8_t hops;
    uint32_t xid;
    uint16_t secs;
    uint16_t flags;
    uint8_t ciaddr[4];
    uint8_t yiaddr[4];
    uint8_t siaddr[4];
    uint8_t giaddr[4];
    uint8_t chaddr[16];
    char sname[64];
    char file[128];
    uint32_t magic;
});


struct dhcp_msg {
    int mode;
    uint32_t uid;
    uint64_t qry;
    uint8_t chaddr[16];
    uint8_t yiaddr[4];

    uint8_t siaddr[4];
    uint8_t submsk[4];
    uint8_t gateway[4];
    uint32_t lease_time;
};

struct dhcp_lease {
    uint8_t ip[4];
    uint8_t sr[4];
    xtime_t expired;
};

static char dhcp2boot[] = {
    0,
    BOOT_REQUEST, // DHCP_DISCOVER  1
    BOOT_REPLY, // DHCP_OFFER  2
    BOOT_REQUEST, // DHCP_REQUEST  3
    BOOT_REQUEST, // DHCP_DECLINE  4
    BOOT_REPLY, // DHCP_PACK  5
    BOOT_REPLY, // DHCP_PNACK  6
    BOOT_REQUEST, // DHCP_RELEASE  7
    BOOT_REQUEST, // DHCP_INFORM  8
};

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

static dhcp_info_t *dhcp_readinfo(ifnet_t *net)
{
    dhcp_info_t *info = net->dhcp;
    if (info == NULL) {
        info = kalloc(sizeof(dhcp_info_t));
        info->mode = 0;
        info->last_request = 0;
        splock_init(&info->lock);
        net->dhcp = info;
    }
    return info;
}

static void dhcp_parse_msg(dhcp_msg_t *msg, skb_t *skb, dhcp_header_t *header)
{
    msg->uid = header->xid;
    memcpy(msg->chaddr, header->chaddr, ETH_ALEN);
    memcpy(msg->yiaddr, header->yiaddr, IP4_ALEN);
    memcpy(msg->siaddr, header->siaddr, IP4_ALEN);
    memcpy(msg->gateway, header->giaddr, IP4_ALEN);

    int i;
    uint8_t type;
    uint8_t len;
    uint8_t value[32];
    for (;;) {
        if (net_skb_read(skb, &type, 1) != 0 || type == 0xff)
            return;
        net_skb_read(skb, &len, 1);
        net_skb_read(skb, value, len);

        switch (type) {
        case DHCP_OPT_MSGTYPE:
            msg->mode = value[0];
            break;
        case DHCP_OPT_CLIENTMAC:
            memcpy(msg->chaddr, &value[1], ETH_ALEN);
            break;
        case DHCP_OPT_QRYLIST:
            msg->qry = 0;
            for (i = 0; i < len; ++i) {
                if (value[0] < 64)
                    msg->qry |= (1ULL << value[0]);
            }
            break;

        case DHCP_OPT_QRYIP:
            memcpy(msg->yiaddr, value, IP4_ALEN);
            break;
        case DHCP_OPT_SERVERIP:
            memcpy(msg->siaddr, value, IP4_ALEN);
            break;
        case DHCP_OPT_SUBNETMASK:
            memcpy(msg->submsk, value, IP4_ALEN);
            break;
        case DHCP_OPT_ROUTER:
            memcpy(msg->gateway, value, IP4_ALEN);
            break;
        case DHCP_OPT_LEASETIME:
            msg->lease_time = htonl(*((uint32_t *)(value)));
        }
    }
}

static void dhcp_clean_msg(dhcp_msg_t *msg)
{

}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

static void dhcp_option_value(skb_t *skb, uint8_t typ, uint8_t len, uint8_t val)
{
    uint8_t option[3];
    option[0] = typ;
    option[1] = len;
    option[2] = val;
    net_skb_write(skb, option, 3);
}

static void dhcp_option_buf(skb_t *skb, uint8_t typ, uint8_t len, const void *buf)
{
    uint8_t option[2];
    option[0] = typ;
    option[1] = len;
    net_skb_write(skb, option, 2);
    net_skb_write(skb, buf, len);
}

static int dhcp_opts_length_req(ifnet_t *net, dhcp_lease_t *lease)
{
    int len = 0;
    len += 5 + 2;
    if (lease)
        len += 6 + 6;
    return len;
}

static int dhcp_opts_write_req(ifnet_t *net, skb_t *skb, dhcp_lease_t *lease)
{
    uint8_t option[12];

    if (lease) {
        dhcp_option_buf(skb, DHCP_OPT_QRYIP, IP4_ALEN, lease->ip);
        dhcp_option_buf(skb, DHCP_OPT_SERVERIP, IP4_ALEN, lease->sr);
    }

    // HOSTNAME

    option[0] = DHCP_OPT_QRYLIST;
    option[1] = 5;
    option[2] = DHCP_OPT_SUBNETMASK;
    option[3] = DHCP_OPT_DOMAIN;
    option[4] = DHCP_OPT_ROUTER;
    option[5] = DHCP_OPT_DNSIP;
    option[6] = DHCP_OPT_LEASETIME;
    net_skb_write(skb, option, option[1] + 2);
    return 0;
}

static int dhcp_opts_length_res(ifnet_t *net, dhcp_lease_t *lease)
{
    int len = 0;
    len += 6 + 6 + 6;
    if (lease)
        len += 6;
    return len;
}

static int dhcp_opts_write_res(ifnet_t *net, skb_t *skb, dhcp_lease_t *lease)
{
    dhcp_info_t *info = dhcp_readinfo(net);
    ip4_info_t *ip4 = ip4_readinfo(net);

    dhcp_option_buf(skb, DHCP_OPT_SERVERIP, IP4_ALEN, ip4->ip);
    dhcp_option_buf(skb, DHCP_OPT_SUBNETMASK, IP4_ALEN, ip4->submsk);
    dhcp_option_buf(skb, DHCP_OPT_ROUTER, IP4_ALEN, ip4->gateway);

    // TODO - Add the domain if we have one.

    if (lease) {
        uint32_t leasetime = htonl(DHCP_LEASE_DURATION);
        dhcp_option_buf(skb, DHCP_OPT_LEASETIME, sizeof(uint32_t), &leasetime);
    }

    return 0;
}

int dhcp_packet(ifnet_t *net, ip4_route_t *route, uint32_t uid, int mode, dhcp_lease_t *lease)
{
    int opcode = dhcp2boot[mode];

    int packet_length = sizeof(dhcp_header_t) + 3 + 9 + 1;
    if (opcode == BOOT_REQUEST)
        packet_length += dhcp_opts_length_req(net, lease);
    else
        packet_length += dhcp_opts_length_res(net, lease);

    skb_t *skb = net_packet(net);
    if (udp_header(skb, route, packet_length, IP4_PORT_DHCP_SRV, IP4_PORT_DHCP, 0, 0) != 0)
        return net_skb_trash(skb);

    net_skb_log(skb, ",dhcp");
    dhcp_header_t *header = net_skb_reserve(skb, sizeof(dhcp_header_t));
    if (header == NULL) {
        net_skb_log(skb, ":Unexpected end of data");
        return -1;
    }

    memset(header, 0, sizeof(dhcp_header_t));
    header->opcode = opcode;
    header->htype = 1;
    header->hlen = 6;
    header->xid = uid;

    ip4_info_t *info = ip4_readinfo(skb->ifnet);
    memcpy(header->ciaddr, info->ip, IP4_ALEN);
    if (opcode == BOOT_REPLY)
        memcpy(header->yiaddr, lease->ip, IP4_ALEN);
    memcpy(header->siaddr, opcode == BOOT_REQUEST ? route->ip : info->ip, IP4_ALEN);
    memcpy(&header->chaddr, opcode == BOOT_REQUEST ? net->hwaddr : route->addr, ETH_ALEN);
    header->magic = DHCP_MAGIC;

    // Write options
    dhcp_option_value(skb, DHCP_OPT_MSGTYPE, 1, mode);

    dhcp_option_value(skb, DHCP_OPT_CLIENTMAC, ETH_ALEN + 1, 1);
    net_skb_write(skb, opcode == BOOT_REQUEST ? net->hwaddr : route->addr, ETH_ALEN);

    const char *vendor = "KoraOS";
    dhcp_option_buf(skb, DHCP_OPT_VENDOR, (uint8_t)strlen(vendor), vendor);

    if (opcode == BOOT_REQUEST)
        dhcp_opts_write_req(net, skb, lease);
    else
        dhcp_opts_write_res(net, skb, lease);

    uint8_t end_of_options = 0xFF;
    net_skb_write(skb, &end_of_options, 1);

    return net_skb_send(skb);
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

void dhcp_on_discovery(ifnet_t *net, dhcp_msg_t *msg)
{
    dhcp_info_t *info = dhcp_readinfo(net);
    ip4_info_t *ip4 = ip4_readinfo(net);

    dhcp_lease_t *lease = NULL;
    // TODO
    // Does this guy have choosed an IP ?
    // If yes check availability (get lease)
    // Is this configured (by MAC, by hostname...)

    splock_lock(&info->lock);
    int i, idx = -1;
    for (i = 0; i < 16; ++i) {
        if (info->lease_range[i] == NULL) {
            idx = i;
            break;
        }
    }

    if (idx < 0) {
        kprintf(-1, "No more IP available on thus sub-network\n");
        splock_unlock(&info->lock);
        return;
    }


    // If not create a lease on new IP
    lease = kalloc(sizeof(dhcp_lease_t));
    memcpy(lease->ip, ip4->ip, IP4_ALEN);
    lease->ip[3] = idx + 4;
    lease->expired = xtime_read(XTIME_CLOCK) + SEC_TO_USEC(DHCP_LEASE_DURATION);
    info->lease_range[idx] = lease;

    splock_unlock(&info->lock);

    ip4_route_t route;
    memcpy(route.ip, lease->ip, IP4_ALEN);
    memcpy(route.addr, msg->chaddr, ETH_ALEN);
    route.ttl = ip4->ttl;
    route.net = net;
    dhcp_packet(net, &route, msg->uid, DHCP_OFFER, lease);
}

void dhcp_on_offer(ifnet_t *net, dhcp_msg_t *msg)
{
    dhcp_info_t *info = dhcp_readinfo(net);

    /* Ignore unsolicited packets */
    if (msg->uid != info->transaction || info->mode != DHCP_DISCOVER) // || REQUEST
        return;

    /* Check proposal validity */
    // TODO -- If not interested, decline ASAP

    splock_lock(&info->lock);
    info->mode = DHCP_REQUEST;
    info->last_request = xtime_read(XTIME_CLOCK);
    splock_unlock(&info->lock);

    dhcp_lease_t lease;
    memcpy(lease.ip, msg->yiaddr, IP4_ALEN);
    memcpy(lease.sr, msg->siaddr, IP4_ALEN);
    dhcp_packet(net, ip4_route_broadcast(net), msg->uid, DHCP_REQUEST, &lease);
}

void dhcp_on_request(ifnet_t *net, dhcp_msg_t *msg)
{
    dhcp_info_t *info = dhcp_readinfo(net);
    ip4_info_t *ip4 = ip4_readinfo(net);

    dhcp_lease_t *lease = info->lease_range[msg->yiaddr[3] - 4];
    if (lease == NULL) {
        dhcp_packet(net, ip4_route_broadcast(net), msg->uid, DHCP_PNACK, NULL);
        return;
    }

    // Get lease, check it's been proposed, check time is OK, check the state is at proposal...

    ip4_route_t route;
    memcpy(route.ip, lease->ip, IP4_ALEN);
    memcpy(route.addr, msg->chaddr, ETH_ALEN);
    route.ttl = ip4->ttl;
    route.net = net;
    dhcp_packet(net, &route, msg->uid, DHCP_PACK, lease);
}

void dhcp_on_ack(ifnet_t *net, dhcp_msg_t *msg)
{
    dhcp_info_t *info = dhcp_readinfo(net);
    ip4_info_t *ip4 = ip4_readinfo(net);

    /* Ignore unsolicited packets */
    if (msg->uid != info->transaction || info->mode != DHCP_REQUEST)
        return;

    ip4_setip(net, msg->yiaddr, msg->submsk, msg->gateway);
    info->mode = DHCP_PACK;
    info->last_request = xtime_read(XTIME_CLOCK);
    info->expire = info->last_request + SEC_TO_USEC(msg->lease_time);
    info->renewal = info->last_request + SEC_TO_USEC(msg->lease_time / 2);
}

void dhcp_on_nack(ifnet_t *net, dhcp_msg_t *msg)
{
    dhcp_info_t *info = dhcp_readinfo(net);

    /* Ignore unsolicited packets */
    if (msg->uid != info->transaction || info->mode != DHCP_REQUEST)
        return;

    info->mode = DHCP_DISCOVER;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

int dhcp_discovery(ifnet_t *net)
{
    /* Check if this is relevant */
    dhcp_info_t *info = dhcp_readinfo(net);
    splock_lock(&info->lock);
    if (info->last_request > xtime_read(XTIME_CLOCK) - SEC_TO_USEC(DHCP_DELAY) || info->mode > DHCP_DISCOVER) {
        splock_unlock(&info->lock);
        return -1;
    }

    // Send the packet DHCP_DISCOVER
    info->mode = DHCP_DISCOVER;
    info->last_request = xtime_read(XTIME_CLOCK);
    info->transaction = rand32();
    splock_unlock(&info->lock);
    return dhcp_packet(net, ip4_route_broadcast(net), info->transaction, DHCP_DISCOVER, NULL);
}

int dhcp_receive(skb_t *skb, int length)
{
    net_skb_log(skb, ",dhcp");
    dhcp_header_t *header = net_skb_reserve(skb, sizeof(dhcp_header_t));
    if (header == NULL) {
        net_skb_log(skb, ":Unexpected end of data");
        return -1;
    }

    if (header->htype != 1 || header->hlen != 6 || header->magic != DHCP_MAGIC)
        return -1;

    ip4_info_t *ip4 = ip4_readinfo(skb->ifnet);
    if (header->opcode == BOOT_REQUEST && !ip4->use_dhcp_server) {
        net_skb_log(skb, ":No server");
        return -1;
    }

    dhcp_msg_t msg;
    dhcp_parse_msg(&msg, skb, header);
    switch (msg.mode) {
    case DHCP_DISCOVER:
        dhcp_on_discovery(skb->ifnet, &msg);
        break;
    case DHCP_OFFER:
        dhcp_on_offer(skb->ifnet, &msg);
        break;
    case DHCP_REQUEST:
        dhcp_on_request(skb->ifnet, &msg);
        break;
    case DHCP_DECLINE:
        // TODO
        break;
    case DHCP_PACK:
        dhcp_on_ack(skb->ifnet, &msg);
        break;
    case DHCP_PNACK:
        dhcp_on_nack(skb->ifnet, &msg);
        break;
    case DHCP_RELEASE:
        // TODO
        break;
    case DHCP_INFORM:
        break;
    }

    dhcp_clean_msg(&msg);
    kfree(skb);
    return 0;
}
