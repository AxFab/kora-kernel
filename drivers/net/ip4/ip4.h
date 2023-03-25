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
#include <kora/time.h>

 // Ethernet
#define ETH_ALEN 6
#define ETH_IP4 htons(0x0800)
#define ETH_IP6 htons(0x86DD)
#define ETH_ARP htons(0x0806)
/* Print as a readable string a MAC address */
char *eth_writemac(const uint8_t *mac, char *buf, int len);
/* Registers a new protocol capable of using Ethernet */
int eth_handshake(netstack_t *stack, uint16_t protocol, net_recv_t recv);
/* Unregisters a protocol capable of using Ethernet */
int eth_unregister(netstack_t *stack, uint16_t protocol, net_recv_t recv);
/* Writes a ethernet header on a tx packet */
int eth_header(skb_t *skb, const uint8_t *addr, uint16_t protocol);



// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#define IP4_ALEN 4

#define IP4_ICMP 0x01
#define IP4_TCP 0x06
#define IP4_UDP 0x11

#define IP4_PORT_DHCP 67
#define IP4_PORT_DHCP_SRV 68

extern const uint8_t mac_broadcast[6];

typedef struct ip4_master ip4_master_t;
typedef struct ip4_info ip4_info_t;
typedef struct dhcp_info dhcp_info_t;
typedef struct icmp_info icmp_info_t;
typedef struct ip4_route ip4_route_t;
typedef struct ip4_port ip4_port_t;
typedef struct ip4_subnet ip4_subnet_t;
typedef struct ip4_class ip4_class_t;

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// IPv4
/* Compute the checksum of a IP4 header */
uint16_t ip4_checksum(uint16_t *ptr, unsigned len);
/* Write a IP header on the socket buffer */
int ip4_header(skb_t *skb, ip4_route_t *route, int protocol, unsigned length, uint16_t identifier, uint16_t offset);
/* Handle the reception of an IP4 packet */
int ip4_receive(skb_t *skb);
/* Fill-out the prototype structure for IP4 */
void ip4_proto(nproto_t *proto);

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// TCP
/* Look for an ephemeral port on TCP */
uint16_t tcp_ephemeral_port(socket_t *sock);
/* Handle the reception of a TCP packet */
int tcp_receive(skb_t *skb, unsigned length, uint16_t identifier, uint16_t offset);
/* Fill-out the prototype structure for TCP */
void tcp_proto(nproto_t* proto);

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// UDP
/* Look for an ephemeral port on UDP */
uint16_t udp_ephemeral_port(socket_t *sock);
/* Write an UDP header on a packet using the provided parameters */
int udp_header(skb_t *skb, ip4_route_t *route, unsigned length, uint16_t rport, uint16_t lport, uint16_t identifier, uint16_t offset);
/* Compute UDP checksum */
void udp_checksum(skb_t *skb, ip4_route_t *route, unsigned length);
/* Handle the reception of a UDP packet */
int udp_receive(skb_t *skb, unsigned length, uint16_t identifier, uint16_t offset);
/* Fill-out the prototype structure for UDP */
void udp_proto(nproto_t* proto);

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// ICMP
/* Handle an ICMP package and respond if required */
int icmp_receive(skb_t *skb, unsigned length);
/* Write an TCP header on a packet using the provided parameters */
int tcp_header(skb_t *skb, ip4_route_t *route, unsigned length, uint16_t rport, uint16_t lport, uint16_t identifier, uint16_t offset);
/* Send a PING request to the following IP route */
int icmp_ping(ip4_route_t *route, const char *buf, unsigned len, net_qry_t *qry);
/* Unregistered a timedout ARP query */
void icmp_forget(ip4_route_t *route, net_qry_t *qry);

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// DHCP
#define DHCP_DELAY 15
#define DHCP_LEASE_DURATION 900
/* Handle the reception of a DHCP packet */
int dhcp_receive(skb_t *skb, int length);
/* Start the discovery process of a DHCP client in order to set IP address */
int dhcp_discovery(ifnet_t* net);
/* - */
int dhcmp_clear(ifnet_t *ifnet, ip4_info_t *info);

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// ARP
/* Handle an ARP package, respond to request and save result of response */
int arp_receive(skb_t *skb);
/* Send an ARP request to look for harware address of the following ip */
int arp_whois(ifnet_t *net, const uint8_t *ip, net_qry_t *qry);
/* Unregistered a timedout ARP query */
void arp_forget(ifnet_t *net, net_qry_t *qry);

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

struct ip4_route {
    uint8_t ip[IP4_ALEN];
    uint8_t addr[8];
    ifnet_t *net;
    int ttl;
};

struct ip4_subnet
{
    llnode_t node;
    ifnet_t *ifnet;
    uint8_t address[IP4_ALEN];
    uint8_t submask[IP4_ALEN];
    uint8_t gateway[IP4_ALEN];
    uint8_t broadcast[IP4_ALEN];
    uint8_t gateway_addr[NET_MAX_HWADRLEN];
};

struct ip4_info {
    int ttl;
    bool use_dhcp;
    bool use_dhcp_server;
    bool use_routing;
    splock_t lock;

    dhcp_info_t *dhcp;
    ip4_subnet_t subnet;

    splock_t qry_lock;
    bbtree_t qry_arps;
    bbtree_t qry_pings;
};

struct ip4_master {
    splock_t plock;
    bbtree_t udp_ports;
    bbtree_t tcp_ports;

    splock_t lock;
    hmap_t ifinfos;
    
    splock_t rlock;
    llhead_t rtable;
    llhead_t subnets;
};

struct ip4_class
{
    const char *label;
    uint8_t mask[IP4_ALEN];
};

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Routing
/* Identify the IP class of an IP address */
int ip4_identify(const uint8_t *ip);
/* Find a route for broadcast */
int ip4_broadcast_route(netstack_t *stack, ip4_route_t *route, ifnet_t *ifnet);
/* Find a route for an IP */
int ip4_find_route(netstack_t *stack, ip4_route_t *route, const uint8_t *ip);

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Generic socket handling
/* Bind a socket to a local port in order to redirect incoming packet */
int ip4_socket_bind(ip4_master_t *master, bbtree_t *tree, socket_t *sock, const uint8_t *addr);
/* Look for an ephemeral port on UDP or TCP */
uint16_t ip4_ephemeral_port(ip4_master_t *master, bbtree_t *tree, socket_t *sock);
/* Check if a socket is listen a port */
socket_t *ip4_lookfor_socket(ip4_master_t *master, bbtree_t *tree, ifnet_t *ifnet, uint16_t port);

int ip4_socket_accept(ip4_master_t *master, bbtree_t *tree, socket_t *sock, socket_t *model, skb_t *skb);

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=



int ip4_socket_close(ip4_master_t *master, bbtree_t *tree, socket_t *sock);


int ip4_readip(uint8_t *ip, const char *str);
char *ip4_writeip(const uint8_t *ip, char *buf, int len);
void ip4_setip(ifnet_t *net, const uint8_t *ip, const uint8_t *submsk, const uint8_t *gateway);
void ip4_config(ifnet_t *net, const char *str);
int ip4_start(netstack_t *stack);


ip4_info_t *ip4_readinfo(ifnet_t *ifnet);
ip4_master_t* ip4_readmaster(netstack_t* stack);


#endif /* _SRC_IP4_H */
