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
#ifndef _SRC_IP4_H
#define _SRC_IP4_H 1

#include <stdint.h>
#include <kora/splock.h>
#include <kora/llist.h>
#include <bits/cdefs.h>
#include <kernel/net.h>
#include <kernel/stdc.h>
#include <kora/hmap.h>
#include <kora/mcrs.h>


 // Ethernet
#define ETH_ALEN 6
#define ETH_IP4 htons(0x0800)
#define ETH_IP6 htons(0x86DD)
#define ETH_ARP htons(0x0806)
char *eth_writemac(const uint8_t *mac, char *buf, int len);
int eth_handshake(netstack_t *stack, uint16_t protocol, net_recv_t recv);
int eth_header(skb_t *skb, const uint8_t *addr, uint16_t protocol);


// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#define IP4_ALEN 4

#define IP4_ICMP 0x01
#define IP4_TCP 0x06
#define IP4_UDP 0x11

#define IP4_PORT_DHCP 67
#define IP4_PORT_DHCP_SRV 68

typedef struct ip4_master ip4_master_t;
typedef struct ipv4_info ip4_info_t;
typedef struct dhcp_info dhcp_info_t;
typedef struct icmp_info icmp_info_t;

typedef struct ip4_route ip4_route_t;
typedef struct ip4_port ip4_port_t;
typedef struct icmp_ping icmp_ping_t;

// IPv4
int ip4_header(skb_t *skb, ip4_route_t *route, int protocol, unsigned length, uint16_t identifier, uint16_t offset);
int ip4_receive(skb_t *skb);
uint16_t ip4_checksum(uint16_t *ptr, unsigned len);

// TCP
int tcp_header(skb_t *skb, ip4_route_t *route, unsigned length, uint16_t rport, uint16_t lport, uint16_t identifier, uint16_t offset);
int tcp_receive(skb_t *skb, unsigned length, uint16_t identifier, uint16_t offset);
void tcp_proto(nproto_t* proto);

// UDP
int udp_header(skb_t *skb, ip4_route_t *route, unsigned length, uint16_t rport, uint16_t lport, uint16_t identifier, uint16_t offset);
int udp_receive(skb_t *skb, unsigned length, uint16_t identifier, uint16_t offset);
void udp_proto(nproto_t* proto);
// skb_t* udp_packet(ifnet_t* net);

// ICMP
int icmp_receive(skb_t *skb, unsigned length);
icmp_ping_t *icmp_ping(ip4_route_t *route, const char *buf, unsigned len);

// DHCP
#define DHCP_DELAY 15
#define DHCP_LEASE_DURATION 900
int dhcp_discovery(ifnet_t* net);
int dhcp_receive(skb_t *skb, int length);

// ARP
int arp_receive(skb_t *skb);
int arp_whois(ifnet_t *net, const uint8_t *ip);


int ip4_readip(uint8_t *ip, const char *str);
char *ip4_writeip(const uint8_t *ip, char *buf, int len);
void ip4_setip(ifnet_t *net, const uint8_t *ip, const uint8_t *submsk, const uint8_t *gateway);
void ip4_config(ifnet_t *net, const char *str);
int ip4_start(netstack_t *stack);

// Private -----

struct ip4_route {
    uint8_t ip[IP4_ALEN];
    uint8_t addr[8];
    ifnet_t *net;
    int ttl;
};

struct ipv4_info {
    int ttl;
    uint8_t ip[IP4_ALEN];
    uint8_t submsk[IP4_ALEN];
    uint8_t gateway[IP4_ALEN];
    bool use_dhcp;
    bool use_dhcp_server;
    bool use_routing;
    ip4_route_t broadcast;
    splock_t lock;

};

struct ip4_master {
    hmap_t tcp_ports;
    hmap_t udp_ports;
    hmap_t routes;
    splock_t lock;
};

struct ip4_port {
    uint16_t port;
    socket_t *socket;
    hmap_t clients;
    bool listen;
};

ip4_info_t *ip4_readinfo(ifnet_t *ifnet);
ip4_master_t* ip4_readmaster(netstack_t* stack);
ip4_route_t *ip4_route_broadcast(ifnet_t *net);
ip4_route_t *ip4_route(netstack_t *stack, const uint8_t *ip);
void ip4_route_add(ifnet_t *net, const uint8_t *ip, const uint8_t *mac);

socket_t* ip4_lookfor_socket(ifnet_t* net, uint16_t port, bool stream, const uint8_t* ip);

#endif /* _SRC_IP4_H */
