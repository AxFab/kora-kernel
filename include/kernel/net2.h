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
#ifndef _KORA_NET2_H
#define _KORA_NET2_H 1

#include <kora/llist.h>
#include <kora/splock.h>
#include <kora/rwlock.h>
#include <stdint.h>
#include <string.h>
#include <sys/sem.h>


#define NET_AF_ETH 1
#define NET_AF_IP4 2
#define NET_AF_IP6 3
#define NET_AF_TCP 4
#define NET_AF_UDP 5


typedef struct netstack netstack_t;
typedef struct ifnet ifnet_t;
typedef struct skb skb_t;
typedef struct socket socket_t;

typedef int (*net_recv_t)(skb_t *);
typedef int (*net_sock_t)(socket_t *);

typedef struct eth_info eth_info_t;
typedef struct ip4_master ip4_master_t;
typedef struct ipv4_info ip4_info_t;
typedef struct dhcp_info dhcp_info_t;
typedef struct icmp_info icmp_info_t;

// Network stack
struct netstack {
    char *hostname;
    char *domain;
    llhead_t list;
    splock_t lock;

    sem_t rx_sem;
    llhead_t rx_list;
    splock_t rx_lock;

    int idMax;

    // protocol info
    eth_info_t *eth;
    ip4_master_t *ipv4;
};

// Network interface
struct ifnet {
    int idx;
    int mtu;
    splock_t lock;
    netstack_t *stack;
    uint8_t hwaddr[16];
    llnode_t node;
    int protocol;
    int flags;

    // Drivers operations
    void *driver_data;
    // void (*link)(ifnet_t*);
    int (*send)(ifnet_t *, skb_t *);

    // Statistics
    long rx_packets;
    long rx_bytes;
    long rx_broadcast;
    long rx_errors;
    long rx_dropped;
    long tx_packets;
    long tx_bytes;
    long tx_broadcast;
    long tx_errors;
    long tx_dropped;


    // protocol info
    ip4_info_t *ipv4;
    dhcp_info_t *dhcp;
    icmp_info_t *icmp;
};

// Socket kernel buffer
struct skb {
    int err;
    unsigned pen;
    unsigned length;
    unsigned size;
    ifnet_t *ifnet;
    llnode_t node;
    char log[64];
    uint8_t buf[1];
};

struct socket {
    int protocol;
};



enum {
    NET_CONNECTED = (1 << 0),

    NET_OVERFILL = (1 << 2),
};



// Socket kernel buffer

/* Create a new tx packet */
skb_t *net_packet(ifnet_t *net);
/* Create a new tx packet */
skb_t *net_packet(ifnet_t *net);
/* Create a new rx packet and push it into received queue */
void net_skb_recv(ifnet_t *net, uint8_t *buf, unsigned len);
/* Send a tx packet to the network card */
int net_skb_send(skb_t *skb);
/* Trash a faulty tx packet */
int net_skb_trash(skb_t *skb);
/* Read data from a packet */
int net_skb_read(skb_t *skb, void *buf, unsigned len);
/* Write data on a packet */
int net_skb_write(skb_t *skb, const void *buf, unsigned len);
/* Get pointer on data from a packet and move cursor */
void *net_skb_reserve(skb_t *skb, unsigned len);


#define net_skb_log(s,m)  strncat((s)->log,(m),64)


// Socket
socket_t *net_socket(netstack_t *stack, int protocol);


// Ethernet
#define ETH_ALEN 6
#define ETH_IP4 htonw(0x0800)
#define ETH_IP6 htonw(0x86DD)
#define ETH_ARP htonw(0x0806)

char *eth_writemac(const uint8_t *mac, char *buf, int len);
int eth_handshake(netstack_t *stack, uint16_t protocol, net_recv_t recv);
int eth_header(skb_t *skb, const uint8_t *addr, uint16_t protocol);
int eth_receive(skb_t *skb);



#define __swap16(w) ((uint16_t)((((w) & 0xFF00) >> 8) | (((w) & 0xFF) << 8)))
#define __swap32(l) ((uint32_t)((((l) & 0xFF000000) >> 24) | (((l) & 0xFF0000) >> 8) | (((l) & 0xFF00) << 8) | (((l) & 0xFF) << 24)))

#define htonw(w) __swap16(w)
#define ntohw(w) __swap16(w)

#define htonl(l) __swap32(l)
#define ntohl(w) __swap32(l)


#endif /* _KORA_NET2_H */
