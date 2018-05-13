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

#define ARP_HW_ETH htonw(1)
#define ARP_PC_IP htonw(0x0800)

#define ARP_REQUEST htonw(1)
#define ARP_REPLY htonw(2)

#define ARP_TIMEOUT 10

typedef struct ARP_header ARP_header_t;

PACK(struct ARP_header {
    uint16_t hardware;
    uint16_t protocol;
    uint8_t hw_length;
    uint8_t pc_length;
    uint16_t opcode;
});

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

static int arp_packet_ip4(netdev_t *ifnet, const uint8_t *mac, const uint8_t *ip, int op)
{
    skb_t *skb = net_packet(ifnet);
    if (skb == NULL)
        return -1;
    if (eth_header(skb, mac, ETH_ARP) != 0)
        return net_trash(skb);
    strncat(skb->log, "ARP:", NET_LOG_SIZE);
    ARP_header_t header;
    header.hardware = ARP_HW_ETH;
    header.protocol = ARP_PC_IP;
    header.hw_length = ETH_ALEN;
    header.pc_length = IP4_ALEN;
    header.opcode = op;
    net_write(skb, &header, sizeof(header));
    net_write(skb, skb->ifnet->eth_addr, ETH_ALEN);
    net_write(skb, skb->ifnet->ip4_addr, IP4_ALEN);
    net_write(skb, mac, ETH_ALEN);
    net_write(skb, ip, IP4_ALEN);
    return net_send(skb);
}

static int arp_recv_ip4(skb_t *skb, int op)
{
    uint8_t mac[ETH_ALEN];
    uint8_t ip[IP4_ALEN];

    /* Source */
    net_read(skb, mac, ETH_ALEN);
    net_read(skb, ip, IP4_ALEN);
    if (memcmp(skb->eth_addr, mac, ETH_ALEN) != 0)
        return -1; // ETH and ARP contradict them-self!

    /* Target */
    net_read(skb, mac, ETH_ALEN);
    if (net_read(skb, ip, IP4_ALEN) != 0)
        return -1;

    if (memcmp(mac, skb->ifnet->eth_addr, ETH_ALEN) != 0 &&
            memcmp(mac, eth_broadcast, ETH_ALEN) != 0)
        return -1;

    if (op == ARP_REQUEST) {
        // net register skb-> ifnet, skb->addrs, ARP_NOT_REQUESTED
        if (memcmp(skb->ifnet->ip4_addr, ip, IP4_ALEN) != 0)
            return -1; /* that not our role to answer */
        return arp_packet_ip4(skb->ifnet, skb->eth_addr, skb->ip4_addr, ARP_REPLY);
    } else if (op == ARP_REPLY) {
        // time_t qry_expired = time(NULL) + ARP_TIMEOUT;
        // net register
        return 0;
    }
    return -1;
}

static int arp_recv_ip6(skb_t *skb, int op)
{
    return -1;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int arp_query(netdev_t *ifnet, const uint8_t *ip)
{
    // time_t now = time(NULL);
    // Store time of request
    return arp_packet_ip4(ifnet, eth_broadcast, ip, ARP_REQUEST);
}

int arp_receive(skb_t *skb)
{
    ARP_header_t header;
    strncat(skb->log, "ARP:", NET_LOG_SIZE);
    if (net_read(skb, &header, sizeof(header)) != 0)
        return -1;
    if (header.hardware != ARP_HW_ETH || header.protocol != ARP_PC_IP || header.hw_length != ETH_ALEN)
        return -1;

    if (header.pc_length == IP4_ALEN)
        return arp_recv_ip4(skb, header.opcode);
    else if (header.pc_length == IP6_ALEN)
        return arp_recv_ip6(skb, header.opcode);
    return -1;
}

