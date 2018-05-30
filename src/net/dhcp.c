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

#define BOOT_REQUEST 1
#define BOOT_REPLY 2

#define DHCP_DISCOVER 1
#define DHCP_OFFER 2
#define DHCP_REQUEST 3
#define DHCP_DECLINE 4
#define DHCP_PACK 5
#define DHCP_PNACK 6
#define DHCP_RELEASE 7
#define DHCP_INFORM 8

#define DHCP_OPT_SUBNETMASK 1
#define DHCP_OPT_ROOTER 3
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

typedef struct DHCP_header DHCP_header_t;

PACK(struct DHCP_header {
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
});

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

static skb_t *dhcp_packet(netdev_t *ifnet, const uint8_t *ip, uint32_t uid, int options_len)
{
    skb_t *skb = net_packet(ifnet, 576);
    if (skb == NULL)
        return NULL;
    if (udp_header(skb, ip, sizeof(DHCP_header_t) + options_len, UDP_PORT_DHCP, UDP_PORT_DHCP_S) != 0) {
        net_trash(skb);
        return NULL;
    }
    strncat(skb->log, "dhcp:", NET_LOG_SIZE);
    DHCP_header_t header;
    memset(&header, 0, sizeof(header));
    header.opcode = BOOT_REQUEST;
    header.htype = 1;
    header.hlen = 6;
    header.xid = uid;

    memcpy(header.ciaddr, ifnet->ip4_addr, IP4_ALEN);
    // futur
    memcpy(header.siaddr, ifnet->dhcp_addr, IP4_ALEN);
    // gateway

    memcpy(&header.chaddr, ifnet->eth_addr, ETH_ALEN);
    net_write(skb, &header, sizeof(header));
    return skb;
}

static int dhcp_handle(skb_t *skb, DHCP_header_t *header, int length)
{
    uint8_t options[32];
    while (length > 2) {
        net_read(skb, options, 2);
        length -= 2 + options[1];
        switch (options[0]) {
            case DHCP_OPT_SUBNETMASK:
            case DHCP_OPT_ROOTER:
            case DHCP_OPT_DNSIP:
            case DHCP_OPT_DOMAIN:
            case DHCP_OPT_LEASETIME:
            case DHCP_OPT_MSGTYPE:
            case DHCP_OPT_SERVERIP:
            case DHCP_OPT_VENDOR:
            default:
                net_read(skb, &options[2], options[1]); // TODO, HUGE DANGER HERE
                break;
        }
    }
    // Todo, validate and push proposal
    // after some delay, take one proposal and send dhcp request
    // wait ack to activate
    // yiaddr / siaddr / cnx
    // opt: gateway / dns / submask
    // on nack request other
    return 0;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
#define DHCP_MAGIC  htonl(0x63825363)

int dhcp_discovery(netdev_t *ifnet)
{
    uint8_t option[32];
    int opts = 3 + 6 + 6 + 8 + 4 + 2 + 8;
    skb_t *skb = dhcp_packet(ifnet, ip4_broadcast, rand32(), opts);
    if (skb == NULL)
        return -1;

    // Magic
    uint32_t magic = DHCP_MAGIC;
    net_write(skb, &magic, 4);

    // Options
    option[0] = DHCP_OPT_MSGTYPE;
    option[1] = 1;
    option[2] = DHCP_DISCOVER;
    net_write(skb, option, option[1] + 2);

    option[0] = DHCP_OPT_HOSTNAME;
    const char *hostname = "kora";
    option[1] = strlen(hostname);
    strncpy((char*)&option[2], hostname, 32-2);
    net_write(skb, option, option[1] + 2);

    option[0] = DHCP_OPT_VENDOR;
    const char *vendor = "KoraOS";
    option[1] = strlen(vendor);
    strncpy((char*)&option[2], vendor, 32-2);
    net_write(skb, option, option[1] + 2);

    option[0] = DHCP_OPT_QRYLIST;
    option[1] = 4;
    option[2] = DHCP_OPT_SUBNETMASK;
    option[3] = DHCP_OPT_DOMAIN;
    option[4] = DHCP_OPT_ROOTER;
    option[5] = DHCP_OPT_DNSIP;
    net_write(skb, option, option[1] + 2);

    option[0] = DHCP_OPT_CLIENTMAC;
    option[1] = ETH_ALEN + 1;
    option[2] = 1;
    memcpy(&option[3], ifnet->eth_addr, ETH_ALEN);
    net_write(skb, option, option[1] + 2);

    option[0] = 0xFF;
    net_write(skb, option, 1);
    return net_send(skb);
}


int dhcp_offer(netdev_t *ifnet)
{
    uint8_t option[32];
    int opts = 3 + 8 + 6 + 6 + 6 + 6 + 11 + 6 + 1 + 6 + 6;
    skb_t *skb = dhcp_packet(ifnet, ip4_broadcast, rand32(), opts);
    if (skb == NULL)
        return -1;

    // Magic
    uint32_t magic = DHCP_MAGIC;
    net_write(skb, &magic, 4);

    // Options
    option[0] = DHCP_OPT_MSGTYPE;
    option[1] = 1;
    option[2] = DHCP_OFFER;
    net_write(skb, option, option[1] + 2);

    option[0] = DHCP_OPT_VENDOR;
    const char *vendor = "KoraOS";
    option[1] = strlen(vendor);
    strncpy((char*)&option[2], vendor, 32-2);
    net_write(skb, option, option[1] + 2);

    option[0] = DHCP_OPT_SUBNETMASK;
    option[1] = 4;
    option[2] = 255;
    option[3] = 255;
    option[4] = 255;
    option[5] = 0;
    net_write(skb, option, option[1] + 2);

    option[0] = DHCP_OPT_SERVERIP;
    option[1] = 4;
    strncpy((char*)&option[2], (char*)ifnet->ip4_addr, 4);
    net_write(skb, option, option[1] + 2);

    if (ifnet->ip4_addr[0] != 0) {
        option[0] = DHCP_OPT_ROOTER;
        option[1] = 4;
        strncpy((char*)&option[2], (char*)ifnet->ip4_addr, 4);
        net_write(skb, option, option[1] + 2);
    }

    if (ifnet->dns_addr[0] != 0) {
        option[0] = DHCP_OPT_DNSIP;
        option[1] = 4;
        strncpy((char*)&option[2], (char*)ifnet->dns_addr, 4);
        net_write(skb, option, option[1] + 2);
    }

    // if (ifnet->dns_addr[0] != 0) {
        option[0] = DHCP_OPT_DOMAIN;
        const char *domain = "axfab.net";
        option[1] = strlen(domain);
        strncpy((char*)&option[2], domain, 32-2);
        net_write(skb, option, option[1] + 2);
    // }

    option[0] = DHCP_OPT_LEASETIME;
    int32_t lease = htonl(3600);
    option[1] = 4;
    strncpy((char*)&option[2], (char*)&lease, 4);
    net_write(skb, option, option[1] + 2);

    option[0] = DHCP_OPT_RENEWALTIME;
    int32_t renewal = htonl(3600 / 2);
    option[1] = 4;
    strncpy((char*)&option[2], (char*)&renewal, 4);
    net_write(skb, option, option[1] + 2);

    option[0] = DHCP_OPT_REBINDINGTIME;
    int32_t rebinding = htonl(3600 - 450);
    option[1] = 4;
    strncpy((char*)&option[2], (char*)&rebinding, 4);
    net_write(skb, option, option[1] + 2);

    option[0] = 0xFF;
    net_write(skb, option, 1);
    return net_send(skb);
}

int dhcp_receive(skb_t *skb, unsigned length)
{
    DHCP_header_t header;
    if (length < sizeof(header))
        return -1;
    if (net_read(skb, &header, sizeof(header)) != 0)
        return -1;
    strncat(skb->log, "dhcp:", NET_LOG_SIZE);
    if (header.htype != 1 || header.hlen != 6)
        return -1;

    uint32_t magic = DHCP_MAGIC;
    if (net_read(skb, &magic, 4) != 0 || magic != DHCP_MAGIC)
        return -1;

    switch (header.opcode) {
    case BOOT_REQUEST:
        return -1; // We're not a dhcp server
    case BOOT_REPLY:
        return dhcp_handle(skb, &header, length - sizeof(header));
    default:
        return -1;
    }
}

