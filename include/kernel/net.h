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
#ifndef _KERNEL_NET_H
#define _KERNEL_NET_H 1

#include <kernel/core.h>
#include <kora/llist.h>
#include <kora/splock.h>

typedef struct netdev netdev_t;
typedef struct skb skb_t;
typedef struct socket socket_t;

#define NET_LOG_SIZE 32

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
/* Ethernet */
#define ETH_ALEN 6
#define ETH_IP4 htonw(0x0800)
#define ETH_IP6 htonw(0x86DD)
#define ETH_ARP htonw(0x0806)

int eth_header(skb_t *skb, const uint8_t *target, uint16_t type);
int eth_receive(skb_t *skb);

extern const uint8_t eth_broadcast[ETH_ALEN];

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
/* ARP - Address Resolution Protocol */
int arp_query(netdev_t *ifnet, const uint8_t *ip);
int arp_receive(skb_t *skb);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
/* IPv4 */
#define IP4_ALEN 4
#define IP4_ICMP 0x01
#define IP4_TCP 0x06
#define IP4_UDP 0x11

int ip4_header(skb_t *skb, const uint8_t *ip_addr, int identifier, int offset, int length, int protocol);
int ip4_receive(skb_t *skb);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
/* ICMP */
#define ICMP_PONG 0
#define ICMP_PING 8

int icmp_ping(netdev_t *ifnet, const uint8_t *ip);
int icmp_receive(skb_t *skb, int len);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
/* IPv6 */
#define IP6_ALEN 16

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
struct netdev {
    int max_packet_size;
    uint8_t eth_addr[ETH_ALEN];
    uint8_t ip4_addr[IP4_ALEN];
    int(*send)(netdev_t*,skb_t*);

    llhead_t queue;
    splock_t lock;

    long rx_packets;
    long rx_bytes;
    long rx_broadcast;
    long rx_errors;
    long tx_packets;
    long tx_bytes;
    long tx_broadcast;
    long tx_errors;
    long tx_dropped;
};

struct skb {
    int err;
    int pen;
    int length;
    netdev_t *ifnet;
    uint8_t eth_addr[ETH_ALEN];
    uint8_t ip4_addr[IP4_ALEN];
    char log[NET_LOG_SIZE];
    llnode_t node;
    uint8_t buf[0];
};

#define NET_ERR_OVERFILL (1 << 0)
#define NET_ERR_HW_ADDR (1 << 1)
#define NET_ERR_SW_ADDR (1 << 2)

/* Create a new tx packet */
skb_t *net_packet(netdev_t *ifnet);
/* Create a new rx packet and push it into received queue */
void net_recv(netdev_t *ifnet, uint8_t *buf, int len);
/* Send a tx packet to the network card */
int net_send(skb_t *skb);
/* Trash a faulty tx packet */
int net_trash(skb_t *skb);
/* Read data from a packet */
int net_read(skb_t *skb, void *buf, int len);
/* Write data on a packet */
int net_write(skb_t *skb, const void *buf, int len);

char *net_ethstr(char *buf, uint8_t *mac);
char *net_ip4str(char *buf, uint8_t *ip);


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

#define __swap16(w) ((uint16_t)(((w & 0xFF00) >> 8) | ((w & 0xFF) << 8)))
#define __swap32(l) ((uint32_t)(((l & 0xFF000000) >> 24) | ((l & 0xFF0000) >> 8) | ((l & 0xFF00) << 8) | ((l & 0xFF) << 24))

#define htonw(w) __swap16(w)
#define ntohw(w) __swap16(w)

#define htonl(l) __swap32(l)
#define ntohl(w) __swap32(l)


#endif  /* _KERNEL_NET_H */
