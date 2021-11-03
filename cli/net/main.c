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
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <threads.h>
#include <kernel/net.h>
#include <math.h>

void ip4_checkup(ifnet_t* net);
void ip4_config(ifnet_t* net, const char* str);
void ip4_start(netstack_t* stack);
int ip4_readip(uint8_t* ip, const char* str);

typedef struct ip4_route ip4_route_t;
typedef struct icmp_ping icmp_ping_t;
icmp_ping_t *icmp_ping(ip4_route_t* route, const char* buf, int len);
ip4_route_t* ip4_route(netstack_t* stack, const uint8_t* ip);

netstack_t* net_setup();
void net_deamon(netstack_t* stack);
ifnet_t* net_interface(netstack_t* stack, int protocol, int idx);

// ======================================================

typedef struct subnet subnet_t;

struct subnet {
    ifnet_t* slots[16];
};

char* vfs_inokey(void* ino, char* tmp)
{
    tmp[0] = '\0';
    return tmp;
}

// ======================================================

static void fakeEth_link(ifnet_t* net)
{
    net->flags |= NET_CONNECTED;
    net_event(net, NET_EV_LINK, 0);
}

static int fakeEth_send(ifnet_t* net, skb_t *skb)
{
    if ((net->flags & NET_CONNECTED) == 0) {
        printf("Tx %s: %s : disconnected\n", net->stack->hostname, skb->log);
        return -1;
    }
    // I have a packet to send
    printf("Tx %s: %s\n", net->stack->hostname, skb->log);

    subnet_t *subnet = (subnet_t *)net->drv_data;
    if (subnet == NULL)
        return -1;

    for (int i = 0; i < 16; ++i) {
        if (subnet->slots[i] && subnet->slots[i] != net)
            net_skb_recv(subnet->slots[i], skb->buf, skb->length);
    }
    return 0;
}

struct net_ops fakeEth_ops = {
    .link = fakeEth_link,
    .send = fakeEth_send,
};

void fakeEth_setup(netstack_t* stack)
{
    char hwaddr[ETH_ALEN];
    hwaddr[0] = 0x80;
    for (int i = 1; i < 6; ++i)
        hwaddr[i] = rand();
    
    ifnet_t* net = net_alloc(stack, NET_AF_ETH, hwaddr, &fakeEth_ops, NULL);
    net->mtu = 1500;
}

// ======================================================

netstack_t* fakeLan_node(const char* name, int ports)
{
    // Create a NETWORK STACK instance
    netstack_t* stack = net_setup();
    stack->hostname = strdup(name);
    eth_setup(stack);
    ip4_start(stack);
      
    int i;
    for (i = 0; i < ports; ++i) {
        fakeEth_setup(stack);
    }

    thrd_t netThrd;
    thrd_create(&netThrd, net_deamon, stack);
    return stack;
}

void fakeLan_link(subnet_t* subnet, ifnet_t* net)
{
    for (int i = 0; i < 16; ++i) {
        if (subnet->slots[i] == NULL) {
            subnet->slots[i] = net;
            net->drv_data = subnet;
            net->ops->link(net);
            break;
        }
    }
}

// ======================================================

int test_ping()
{
    subnet_t subnet;
    memset(&subnet, 0, sizeof(subnet_t));
    // We create a small fake network
    netstack_t* ndGateway = fakeLan_node("Gateway", 1);
    netstack_t* ndAlice = fakeLan_node("Alice", 1);

    ifnet_t* gateway_eth0 = net_interface(ndGateway, NET_AF_ETH, 0);
    ip4_config(gateway_eth0, "ip=192.168.0.1 dhcp-server");
    Sleep(1000); // Give time to setup (1s)

    // We plug Alice to an interface of the Gateway
    fakeLan_link(&subnet, gateway_eth0);
    fakeLan_link(&subnet, net_interface(ndAlice, NET_AF_ETH, 0));

    Sleep(500);
    uint8_t ip[4];
    ip4_readip(ip, "192.168.0.1");
    icmp_ping(ip4_route(ndAlice, ip), "ABCD", 4);

    // We wait...
    Sleep(1500000);

    return 0;
}


int test_service()
{
    subnet_t subnet;
    memset(&subnet, 0, sizeof(subnet_t));

    // We create a small fake network
    netstack_t* ndAlice = fakeLan_node("Alice", 1);
    netstack_t* ndBob = fakeLan_node("Bob", 1);

    ifnet_t* alice_eth0 = net_interface(ndAlice, NET_AF_ETH, 0);
    ip4_config(alice_eth0, "ip=192.168.0.1");

    ifnet_t* bob_eth0 = net_interface(ndBob, NET_AF_ETH, 0);
    ip4_config(bob_eth0, "ip=192.168.0.2");

    Sleep(1000); // Give time to setup (1s)

    // We plug Alice to an interface of the Gateway
    fakeLan_link(&subnet, alice_eth0);
    fakeLan_link(&subnet, bob_eth0);

    Sleep(2000); // Give time to setup (2s)


    // Alice open a IP4/UDP socket
    socket_t *alice_srv = net_socket(ndAlice, NET_AF_UDP);
    if (alice_srv == NULL)
        return -1;

    // Alice bind the UDP socket to port 7003
    uint8_t ipaddr[6] = { 0 };
    ipaddr[4] = 7003 >> 8;
    ipaddr[5] = 7003 & 0xff;
    net_socket_bind(alice_srv, ipaddr, 6);

    // Alice accept connection (async)
    net_socket_accept(alice_srv, false);

    // Bob open a IP4/UDP socket
    socket_t* bob_cnx = net_socket(ndBob, NET_AF_UDP);

    // Bob connect to UDP port 7003 on 192.168.0.1
    ipaddr[0] = 192;
    ipaddr[1] = 168;
    ipaddr[2] = 0;
    ipaddr[3] = 1;
    ipaddr[4] = htonw(7003) >> 8;
    ipaddr[5] = htonw(7003) & 0xff;
    net_socket_connect(bob_cnx, ipaddr, 6);

    // Check Alice have a connection (TCP)

    // Bob say "Hello, I'm Bob"
    net_socket_write(bob_cnx, "Hello, I'm Bob\n", strlen("Hello, I'm Bob\n"), 0);

    // Alice read the message
    socket_t *alice_cnx = net_socket_accept(alice_srv, true);
    if (alice_cnx == NULL)
        return -1;
    char buf[128];
    net_socket_read(alice_cnx, buf, 128, 0);

    // Alice send "Hello Bob, I'm Alice"
    net_socket_write(bob_cnx, "Hello Bob, I'm Alice\n", strlen("Hello Bob, I'm Alice\n"), 0);

    // Bob read the message
    net_socket_read(bob_cnx, buf, 128, 0);
    // Bob close the socket
    net_socket_close(bob_cnx);

    // Alice loose connection (TCP)



    // We wait...
    Sleep(1500000);

    return 0;
}

int test_router()
{
    subnet_t subnet1;
    memset(&subnet1, 0, sizeof(subnet_t));

    subnet_t subnet2;
    memset(&subnet2, 0, sizeof(subnet_t));


    // We create a small fake network
    netstack_t* ndGateway = fakeLan_node("Gateway", 2);
    netstack_t* ndAlice = fakeLan_node("Alice", 1);
    netstack_t* ndBob = fakeLan_node("Bob", 1);
    netstack_t* ndCharlie = fakeLan_node("Charlie", 1);

    ifnet_t* gateway_eth0 = net_interface(ndGateway, NET_AF_ETH, 0);
    ip4_config(gateway_eth0, "ip=192.168.0.1 dhcp-server");

    ifnet_t* gateway_eth1 = net_interface(ndGateway, NET_AF_ETH, 1);
    ip4_config(gateway_eth1, "ip=192.168.128.1 dhcp-server");
    Sleep(1000); // Give time to setup (1s)

    // We plug Alice to an interface of the Gateway
    fakeLan_link(&subnet1, gateway_eth0);
    fakeLan_link(&subnet1, net_interface(ndAlice, NET_AF_ETH, 0));

    fakeLan_link(&subnet2, gateway_eth1);
    fakeLan_link(&subnet2, net_interface(ndBob, NET_AF_ETH, 0));
    fakeLan_link(&subnet2, net_interface(ndCharlie, NET_AF_ETH, 0));


    // We wait...
    Sleep(1500000);

    return 0;
}


// ======================================================

int main(int argc, char** argv)
{
    test_service();
}
