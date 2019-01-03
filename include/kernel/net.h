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
#ifndef _KERNEL_NET_H
#define _KERNEL_NET_H 1

#include <kernel/core.h>
#include <kora/llist.h>
#include <kora/splock.h>

#define NET_LOG_SIZE 32

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
/* Ethernet */
#define ETH_ALEN 6
#define ETH_IP4 htonw(0x0800)
#define ETH_IP6 htonw(0x86DD)
#define ETH_ARP htonw(0x0806)

int eth_header(skb_t *skb, const uint8_t *target, uint16_t type);
int eth_receive(skb_t *skb);

#define eth_null(n) (((uint16_t*)(n))[0]==0&&((uint16_t*)(n))[1]==0&&((uint16_t*)(n))[2])
#define eth_eq(a,b) (((uint16_t*)(a))[0]==((uint16_t*)(b))[0]&&((uint16_t*)(a))[1]==((uint16_t*)(b))[1]&&((uint16_t*)(a))[2]==((uint16_t*)(b))[2])
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

uint16_t ip4_checksum(skb_t *skb, size_t len);
int ip4_header(skb_t *skb, const uint8_t *ip_addr, int identifier, int offset,
               int length, int protocol);
int ip4_receive(skb_t *skb);

#define ip4_null(n) (*((uint32_t*)(n))==0)
#define ip4_eq(a,b) (*((uint32_t*)(a))==*((uint32_t*)(b)))
extern const uint8_t ip4_broadcast[IP4_ALEN];

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
/* ICMP */
#define ICMP_PONG 0
#define ICMP_PING 8

int icmp_ping(netdev_t *ifnet, const uint8_t *ip);
int icmp_receive(skb_t *skb, unsigned len);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
/* UDP */

#define UDP_PORT_DNS 53
#define UDP_PORT_NTP 123
#define UDP_PORT_DHCP 67
#define UDP_PORT_DHCP_S 68
#define UDP_PORT_DHCP6 546

int udp_header(skb_t *skb, const uint8_t *ip, int length, int port, int src);
int udp_receive(skb_t *skb, unsigned length);
int udp_packet(socket_t *socket);
socket_t *udp_socket(netdev_t *ifnet, uint8_t *ip, int port);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
/* TCP */

int tcp_header(skb_t *skb, const uint8_t *ip, int length, int port, int src);
int tcp_receive(skb_t *skb, unsigned length);
int tcp_packet(socket_t *socket);
socket_t *tcp_socket(netdev_t *ifnet, uint8_t *ip, int port);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
/* DHCP */
typedef struct dhcp_server dhcp_server_t;

int dhcp_discovery(netdev_t *ifnet);
int dhcp_receive(skb_t *skb, unsigned length);
dhcp_server_t *dhcp_create_server(uint8_t *ip_rooter, int netsz);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
/* DNS */

int dns_receive(skb_t *skb, int length);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
/* NTP */

int ntp_receive(skb_t *skb, int length);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
/* IPv6 */
#define IP6_ALEN 16

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
struct netdev {
    unsigned mtu;
    int no;
    int flags;
    uint8_t eth_addr[ETH_ALEN];
    uint8_t ip4_addr[IP4_ALEN];
    int(*send)(netdev_t *, skb_t *);
    int(*link)(netdev_t *);

    llhead_t queue;
    emitter_t waiters;
    splock_t lock;

    char *hostname;
    char *domain;

    // IP ------------
    uint8_t gateway_ip[IP4_ALEN];
    uint8_t gateway_mac[ETH_ALEN];
    uint8_t subnet_bits;

    // DHCP ------------
    dhcp_server_t *dhcp_srv;
    uint8_t dhcp_ip[IP4_ALEN];
    uint32_t dhcp_transaction;
    uint64_t dhcp_lastsend;
    uint8_t dhcp_mode;

    // DNS ------------
    uint8_t dns_ip[IP4_ALEN];


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
};

struct skb {
    int err;
    unsigned pen;
    unsigned length;
    unsigned size;
    netdev_t *ifnet;
    uint8_t eth_addr[ETH_ALEN];
    uint8_t ip4_addr[IP4_ALEN];
    char log[NET_LOG_SIZE];
    llnode_t node;
    uint8_t buf[1500];
};

struct socket {
    netdev_t *ifnet;
    uint16_t sport;
    uint16_t dport;
    uint8_t addr[16];
    int(*packet)(socket_t *socket);
};

#define NET_ERR_OVERFILL (1 << 0)
#define NET_ERR_HW_ADDR (1 << 1)
#define NET_ERR_SW_ADDR (1 << 2)

#define NET_CONNECTED  (1 << 0)
#define NET_QUIET  (1 << 1)
#define NET_NO_DHCP  (1 << 2)
#define NET_QUIT  (1 << 16)

#define HOST_TEMPORARY 1

/* Register a network device */
int net_device(netdev_t *ifnet);
/* Create a new tx packet */
skb_t *net_packet(netdev_t *ifnet, unsigned size);
/* Create a new rx packet and push it into received queue */
void net_recv(netdev_t *ifnet, uint8_t *buf, unsigned len);
/* Send a tx packet to the network card */
int net_send(skb_t *skb);
/* Trash a faulty tx packet */
int net_trash(skb_t *skb);
/* Read data from a packet */
int net_read(skb_t *skb, void *buf, unsigned len);
/* Write data on a packet */
int net_write(skb_t *skb, const void *buf, unsigned len);
/* Get pointer on data from a packet and move cursor */
void *net_pointer(skb_t *skb, unsigned len);


char *net_ethstr(char *buf, uint8_t *mac);
char *net_ip4str(char *buf, uint8_t *ip);

uint16_t net_rand_port();
uint16_t net_ephemeral_port(socket_t *socket);

void host_init();
void host_register(uint8_t *mac, uint8_t *ip, const char *hostname,
                   const char *domain, int trust);
int host_mac_for_ip(uint8_t *mac, const uint8_t *ip, int trust);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

#define __swap16(w) ((uint16_t)((((w) & 0xFF00) >> 8) | (((w) & 0xFF) << 8)))
#define __swap32(l) ((uint32_t)((((l) & 0xFF000000) >> 24) | (((l) & 0xFF0000) >> 8) | (((l) & 0xFF00) << 8) | (((l) & 0xFF) << 24)))

#define htonw(w) __swap16(w)
#define ntohw(w) __swap16(w)

#define htonl(l) __swap32(l)
#define ntohl(w) __swap32(l)


#endif  /* _KERNEL_NET_H */
