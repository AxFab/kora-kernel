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
#include "../cli.h"
#include <stdlib.h>
#include <stdint.h>
#include <threads.h>
#include <kernel/net.h>
#include <kora/mcrs.h>
#include <kora/time.h>
#include <math.h>

int alloc_check();

#define ETH_ALEN 6
#define IP4_ALEN 4

void ip4_config(ifnet_t* net, const char* str);
void ip4_start(netstack_t* stack);
int ip4_readip(uint8_t* ip, const char* str);

typedef struct ip4_route ip4_route_t;
typedef struct icmp_ping icmp_ping_t;
int icmp_ping(ip4_route_t *route, const char *buf, unsigned len, net_qry_t *qry);
int ip4_find_route(netstack_t *stack, ip4_route_t *route, const uint8_t *ip);

int ip4_readaddr(uint8_t *addr, const char *str);
void icmp_forget(ip4_route_t *route, net_qry_t *qry);


struct ip4_route
{
    uint8_t ip[IP4_ALEN];
    uint8_t addr[8];
    ifnet_t *net;
    int ttl;
};


// ======================================================

typedef struct
{
    void *ptr;
    size_t len;
    int type;
} buffer_t;

typedef struct subnet subnet_t;

netstack_t *lan_init(const char *name, int cards);
int lan_connect(subnet_t *subnet, ifnet_t *ifnet);
int lan_disconnect(subnet_t *subnet, ifnet_t *ifnet);

struct subnet 
{
    size_t len;
    ifnet_t *slots[0];
};

// ======================================================

char* vfs_inokey(void* ino, char* tmp)
{
    tmp[0] = '\0';
    return tmp;
}

typedef int64_t xoff_t;
//size_t block_fetch(void *ino, xoff_t off) { return 0; }
//int block_release(void *ino, xoff_t off, size_t page, bool dirty) { return -1; }

size_t vfs_fetch_page(void *ino, xoff_t off) { return 0; }
int vfs_release_page(void *ino, xoff_t off, size_t pg, bool dirty) { return -1; }

// ======================================================

int test_ping()
{
    subnet_t subnet;
    memset(&subnet, 0, sizeof(subnet_t));
    // We create a small fake network
    netstack_t* ndGateway = lan_init("Gateway", 1);
    netstack_t* ndAlice = lan_init("Alice", 1);

    ifnet_t* gateway_eth0 = net_interface(ndGateway, NET_AF_ETH, 0);
    ip4_config(gateway_eth0, "ip=192.168.0.1 dhcp-server");
    struct timespec st = { .tv_sec = 1, .tv_nsec = 0 };
    thrd_sleep(&st, NULL); // Give time to setup (1s)

    // We plug Alice to an interface of the Gateway
    lan_connect(&subnet, gateway_eth0);
    lan_connect(&subnet, net_interface(ndAlice, NET_AF_ETH, 0));

    thrd_sleep(&st, NULL);
    uint8_t ip[4];
    ip4_readip(ip, "192.168.0.1");
    ip4_route_t route;
    ip4_find_route(ndAlice, &route, ip);
    net_qry_t qry;
    icmp_ping(&route, "ABCD", 4, &qry);

    // We wait...
    st.tv_sec = 1500;
    thrd_sleep(&st, NULL);
    return 0;
}

int test_service()
{
    int len = sizeof(subnet_t) + 4 * sizeof(void *);
    subnet_t *subnet = malloc(len);
    memset(subnet, 0, len);
    subnet->len = 4;

    // We create a small fake network
    netstack_t* ndAlice = lan_init("Alice", 1);
    netstack_t* ndBob = lan_init("Bob", 1);

    ifnet_t* alice_eth0 = net_interface(ndAlice, NET_AF_ETH, 1);
    ip4_config(alice_eth0, "ip=192.168.0.1");

    ifnet_t* bob_eth0 = net_interface(ndBob, NET_AF_ETH, 1);
    ip4_config(bob_eth0, "ip=192.168.0.2");

    struct timespec st = { .tv_sec = 1, .tv_nsec = 0 };
    thrd_sleep(&st, NULL); // Give time to setup (1s)

    // We plug Alice to an interface of the Gateway
    lan_connect(subnet, alice_eth0);
    lan_connect(subnet, bob_eth0);

    st.tv_sec = 2;
    thrd_sleep(&st, NULL); // Give time to setup (2s)


    // Alice open a IP4/UDP socket
    socket_t *alice_srv = net_socket(ndAlice, NET_AF_UDP, 0);
    if (alice_srv == NULL)
        return -1;

    // Alice bind the UDP socket to port 7003
    uint8_t ipaddr[6] = { 0 };
    ipaddr[4] = htons(7003) & 0xff;
    ipaddr[5] = htons(7003) >> 8;
    net_socket_bind(alice_srv, ipaddr, 6);

    // Alice accept connection (async)
    net_socket_accept(alice_srv, false);

    // Bob open a IP4/UDP socket
    socket_t* bob_cnx = net_socket(ndBob, NET_AF_UDP, 0);

    // Bob connect to UDP port 7003 on 192.168.0.1
    ipaddr[0] = 192;
    ipaddr[1] = 168;
    ipaddr[2] = 0;
    ipaddr[3] = 1;
    ipaddr[4] = htons(7003) & 0xff;
    ipaddr[5] = htons(7003) >> 8;
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
    // Check it's equal to it!

    // Alice send "Hello Bob, I'm Alice"
    net_socket_write(alice_cnx, "Hello Bob, I'm Alice\n", strlen("Hello Bob, I'm Alice\n"), 0);

    // Bob read the message
    net_socket_read(bob_cnx, buf, 128, 0);
    // Bob close the socket
    net_socket_close(bob_cnx);

    // Alice loose connection (TCP)



    // We wait...
    // Sleep(1500000);

    return 0;
}

int test_router()
{
    subnet_t subnet1;
    memset(&subnet1, 0, sizeof(subnet_t));

    subnet_t subnet2;
    memset(&subnet2, 0, sizeof(subnet_t));


    // We create a small fake network
    netstack_t* ndGateway = lan_init("Gateway", 2);
    netstack_t* ndAlice = lan_init("Alice", 1);
    netstack_t* ndBob = lan_init("Bob", 1);
    netstack_t* ndCharlie = lan_init("Charlie", 1);

    ifnet_t* gateway_eth0 = net_interface(ndGateway, NET_AF_ETH, 0);
    ip4_config(gateway_eth0, "ip=192.168.0.1 dhcp-server");

    ifnet_t* gateway_eth1 = net_interface(ndGateway, NET_AF_ETH, 1);
    ip4_config(gateway_eth1, "ip=192.168.128.1 dhcp-server");
    struct timespec st = { .tv_sec = 1, .tv_nsec = 0 };
    thrd_sleep(&st, NULL); // Give time to setup (1s)

    // We plug Alice to an interface of the Gateway
    lan_connect(&subnet1, gateway_eth0);
    lan_connect(&subnet1, net_interface(ndAlice, NET_AF_ETH, 0));

    lan_connect(&subnet2, gateway_eth1);
    lan_connect(&subnet2, net_interface(ndBob, NET_AF_ETH, 0));
    lan_connect(&subnet2, net_interface(ndCharlie, NET_AF_ETH, 0));


    // We wait...
    st.tv_sec = 1500;
    thrd_sleep(&st, NULL);

    return 0;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#define OBJ_BUFFER 1
#define OBJ_SUBNET 2
#define OBJ_NSTACK 3
#define OBJ_SOCKET 4

static ifnet_t *fetch_ifnet(char *name)
{
    char *host = strtok(name, ":");
    char *prot = strtok(NULL, ":");
    int idx = strtol(strtok(NULL, ":"), NULL, 10);
    netstack_t *stack = cli_fetch(name, OBJ_NSTACK);
    if (stack == NULL) {
        fprintf(stderr, "The stack '%s' doesn't exist\n", host);
        return NULL;
    }
    ifnet_t *ifnet = net_interface(stack, NET_AF_ETH, idx);
    if (ifnet == NULL) {
        fprintf(stderr, "The device '%s:%d' doesn't exist on stack %s\n", prot, idx, host);
        return NULL;
    }
    return ifnet;
}

static subnet_t *fetch_subnet(char *name)
{
    subnet_t *subnet = cli_fetch(name, OBJ_SUBNET);
    if (subnet == NULL) {
        subnet = calloc(sizeof(subnet_t) + 16 * sizeof(void *), 1);
        subnet->len = 16;
        cli_store(name, subnet, OBJ_SUBNET);
    }
    return subnet;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

int do_node(void *cfg, size_t *params)
{
    char *name = (char *)params[0];
    int cards = (int)params[1];
    netstack_t *stack = lan_init(name, cards);
    if (stack == NULL)
        return cli_error("Unable to initizalise stack %s\n", name);
    cli_store(name, stack, OBJ_NSTACK);
    return 0;
}

int do_kill(void *cfg, size_t *params)
{
    char *host = (char *)params[0];
    netstack_t *stack = cli_fetch(host, OBJ_NSTACK);
    if (stack == NULL)
        return cli_error("Unable to find the stack named '%s'\n", host);
    cli_remove(host, OBJ_NSTACK);
    int ret = net_destroy_stack(stack);
    return ret;
}

int do_link(void *cfg, size_t * params)
{
    char *lan = (char *)params[0];
    char *name = (char *)params[1];
    subnet_t *subnet = fetch_subnet(lan);
    ifnet_t *ifnet = fetch_ifnet(name);
    if (subnet == NULL || ifnet == NULL)
        return -1;
    lan_connect(subnet, ifnet);
    return 0;
}

int do_unlink(void *cfg, size_t *params)
{
    char *lan = (char *)params[0];
    char *name = (char *)params[1];
    subnet_t *subnet = fetch_subnet(lan);
    ifnet_t *ifnet = fetch_ifnet(name);
    if (subnet == NULL || ifnet == NULL)
        return -1;
    int stay = lan_disconnect(subnet, ifnet);
    if (stay == 0) {
        cli_remove(lan, OBJ_SUBNET);
        free(subnet);
    }
    return 0;
}

int do_tempo(void *cfg, size_t *params)
{
    struct timespec sl;
    sl.tv_sec = 0;
    sl.tv_nsec = 500 * 1000 * 1000;
    thrd_sleep(&sl, NULL);
    return 0;
}

int do_text(void *cfg, size_t *params)
{
    char *name = (char *)params[0];
    char *value = (char *)params[1];
    buffer_t *buffer = cli_fetch(name, OBJ_BUFFER);
    if (buffer != NULL) {
        free(buffer->ptr);
        free(buffer);
    }
    buffer = malloc(sizeof(buffer_t));
    buffer->ptr = strdup(value);
    buffer->len = strlen(value);
    buffer->type = 1;
    cli_store(name, buffer, OBJ_BUFFER);
    return 0;
}


int do_rmbuf(void *cfg, size_t *params)
{
    char *name = (char *)params[0];
    buffer_t *buffer = cli_fetch(name, OBJ_BUFFER);
    if (buffer == NULL)
        return cli_error("Unknwon buffer");
    free(buffer->ptr);
    free(buffer);
    cli_remove(name, OBJ_BUFFER);
    return 0;
}

int do_expect(void *cfg, size_t * params)
{
    char *cmd = (char *)params[0];
    char *oprd0 = (char *)params[1];
    char *oprd1 = (char *)params[2];

    if (strcmp(cmd, "EQ") == 0) {
        buffer_t *buf0 = cli_fetch(oprd0, OBJ_BUFFER);
        buffer_t *buf1 = cli_fetch(oprd1, OBJ_BUFFER);
        if (buf0 == NULL || buf1 == NULL)
            return cli_error("Unavailable operands");
        if (buf0->len != buf1->len || memcmp(buf0->ptr, buf1->ptr, buf0->len) != 0)
            return -1;
        return 0;

    } else if (strcmp(cmd, "NONULL") == 0) {
        socket_t *sock = cli_fetch(oprd0, OBJ_SOCKET);
        if (sock == NULL)
            return -1;
        return 0;
    }

    return cli_error("Unknown operator %s", cmd);
}

void kdump(void *, size_t);

int do_print(void *cfg, size_t *params)
{
    char *name = (char *)params[0];
    buffer_t *buffer = cli_fetch(name, OBJ_BUFFER);
    printf("Value %s:\n", name);
    kdump(buffer->ptr, buffer->len);
    return 0;
}

int do_socket(void *cfg, size_t *params)
{
    char *host = (char *)params[0];
    char *name = (char *)params[1];
    char *protol = (char *)params[2];
    // char *method = (char *)params[3];
    netstack_t *stack = cli_fetch(host, OBJ_NSTACK);
    int protocol = 0;
    if (strcmp(protol, "udp") == 0 || strcmp(protol, "udp/ip4") == 0)
        protocol = NET_AF_UDP;
    else if (strcmp(protol, "tcp") == 0 || strcmp(protol, "tcp/ip4") == 0)
        protocol = NET_AF_TCP;
    socket_t *sock = net_socket(stack, protocol, 0);
    if (sock == NULL) {
        fprintf(stderr, "Unable to create the socket\n");
        return -1;
    }
    cli_store(name, sock, OBJ_SOCKET);
    return 0;
}

int do_close(void *cfg, size_t *params)
{
    char *name = (char *)params[0];
    socket_t *sock = cli_fetch(name, OBJ_SOCKET);
    if (sock == NULL)
        return cli_error("Unknwon socket\n");
    int ret = net_socket_close(sock);
    if (ret == 0)
        cli_remove(name, OBJ_SOCKET);
    return ret;
}

int do_bind(void *cfg, size_t *params)
{
    char *name = (char *)params[0];
    char *addr = (char *)params[1];
    uint8_t haddr[NET_MAX_HWADRLEN];
    socket_t *sock = cli_fetch(name, OBJ_SOCKET);
    if (sock == NULL) {
        fprintf(stderr, "Unknwon socket\n");
        return -1;
    }
    ip4_readaddr(haddr, addr);
    net_socket_bind(sock, haddr, 6);
    return 0;
}

int do_connect(void *cfg, size_t *params)
{
    char *name = (char *)params[0];
    char *addr = (char *)params[1];
    uint8_t haddr[NET_MAX_HWADRLEN];
    socket_t *sock = cli_fetch(name, OBJ_SOCKET);
    if (sock == NULL) {
        fprintf(stderr, "Unknwon socket\n");
        return -1;
    }
    ip4_readaddr(haddr, addr);
    net_socket_connect(sock, haddr, 6);
    return 0;
}

int do_send(void *cfg, size_t *params)
{
    char *name = (char *)params[0];
    char *buff = (char *)params[1];
    socket_t *sock = cli_fetch(name, OBJ_SOCKET);
    buffer_t *buffer = cli_fetch(buff, OBJ_BUFFER);
    if (sock == NULL || buffer == NULL)
        return cli_error("Unknwon element");
    long bytes = net_socket_write(sock, (const char *)buffer->ptr, buffer->len, 0);
    return bytes == (long)buffer->len ? 0 : -1;
}

int do_recv(void *cfg, size_t *params)
{
    char *name = (char *)params[0];
    char *buff = (char *)params[1];
    socket_t *sock = cli_fetch(name, OBJ_SOCKET);
    buffer_t *buffer = cli_fetch(buff, OBJ_BUFFER);
    if (buffer == NULL)
        buffer = malloc(sizeof(buffer_t));
    else
        free(buffer->ptr);
    buffer->ptr = malloc(1500);
    buffer->len = 1500;
    buffer->type = 1;
    cli_store(buff, buffer, OBJ_BUFFER);
    if (sock == NULL || buffer == NULL)
        return cli_error("Unknwon element");
    long ret = net_socket_read(sock, buffer->ptr, buffer->len, 0);
    if (ret == 0)
        cli_warn("Did not received any data\n");
    buffer->len = ret;
    return 0;
}

int do_accept(void *cfg, size_t *params)
{
    char *name = (char *)params[0];
    char *name2 = (char *)params[1];
    int timeout = (int)params[2];
    socket_t *sock = cli_fetch(name, OBJ_SOCKET);
    if (sock == NULL)
        return cli_error("Unknwon socket\n");
    socket_t *client = net_socket_accept(sock, (xtime_t)timeout * 1000);
    if (client != NULL)
        cli_store(name2, client, OBJ_SOCKET);
    else
        cli_warn("No incoming socket\n");
    return 0;
}

int do_ip4_config(void *cfg, size_t *params)
{
    char *name = (char *)params[0];
    char *arg = (char *)params[1];
    ifnet_t *ifnet = fetch_ifnet(name);
    if (ifnet == NULL)
        return -1;
    ip4_config(ifnet, arg);
    return 0;
}

int do_ping(void *cfg, size_t *params)
{
    char *host = (char *)params[0];
    char *target = (char *)params[1];
    int len = MAX(4, MIN(64, (int )params[2]));
    netstack_t *stack = cli_fetch(host, OBJ_NSTACK);
    if (stack == NULL)
        return cli_error("The stack '%s' doesn't exist\n", host);

    uint8_t ip[4];
    uint8_t payload[64];
    uint8_t salt = rand();
    for (int i = 0; i < len; ++i)
        payload[i] = i ^ salt;
    ip4_readip(ip, target);
    ip4_route_t route;
    if (ip4_find_route(stack, &route, ip) != 0)
        return cli_error("Unable to find a route to '%s' from '%s'\n", target, host);
    net_qry_t qry;
    int ret = icmp_ping(&route, (char *)payload, len, &qry);
    if (ret != 0)
        return cli_error("Unable to complete ping request on %s\n", host);

#if defined WIN32 || defined KORA_KRN
    struct timespec wt = { .tv_sec = 1, .tv_nsec = 0 };
#else
    xtime_t rl = xtime_read(XTIME_CLOCK);
    struct timespec wt = { .tv_sec = rl / 1000000LL + 1, .tv_nsec = 0 };
#endif
    cnd_timedwait(&qry.cnd, &qry.mtx, &wt);
    if (qry.success)
        fprintf(stderr, "Ping request completed with success\n");
    else if (qry.received)
        fprintf(stderr, "Ping request completed but wrong payload\n");
    else {
        fprintf(stderr, "Ping request timeout\n");
        icmp_forget(&route, &qry);
    }
    return 0;
}

int do_test(void *cfg, size_t *params)
{
    test_service();
    return 0;
}

int do_restart(void *cfg, size_t *params)
{
    cli_warn("Checking...\n");
    return alloc_check();
}


cli_cmd_t __commands[] = {
    { "RESTART", "", { 0, 0, 0, 0, 0 }, do_restart, 1 },
    { "QUIT", "", { 0, 0, 0, 0, 0 }, do_restart, 1 },
    // LAN
    { "NODE", "", { ARG_STR, ARG_INT, 0, 0, 0 }, do_node, 1 },
    { "KILL", "", { ARG_STR, 0, 0, 0, 0 }, do_kill, 1 },
    { "LINK", "", { ARG_STR, ARG_STR, 0, 0, 0 }, do_link, 1 },
    { "UNLINK", "", { ARG_STR, ARG_STR, 0, 0, 0 }, do_unlink, 1 },
    { "TEMPO", "", { 0, 0, 0, 0, 0 }, do_tempo, 1 },
    { "TEXT", "", { ARG_STR, ARG_STR, 0, 0, 0 }, do_text, 1 },
    { "RMBUF", "", { ARG_STR, 0, 0, 0, 0 }, do_rmbuf, 1 },
    // SOCK
    { "SOCKET", "", { ARG_STR, ARG_STR, ARG_STR, ARG_STR, 0 }, do_socket, 3 },
    { "CLOSE", "", { ARG_STR, 0, 0, 0, 0 }, do_close, 1 },
    { "BIND", "", { ARG_STR, ARG_STR, 0, 0, 0 }, do_bind, 2 },
    { "CONNECT", "", { ARG_STR, ARG_STR, 0, 0, 0 }, do_connect, 2 },
    { "SEND", "", { ARG_STR, ARG_STR, 0, 0, 0 }, do_send, 2 },
    { "RECV", "", { ARG_STR, ARG_STR, 0, 0, 0 }, do_recv, 2 },
    { "ACCEPT", "", { ARG_STR, ARG_STR, ARG_INT, 0, 0 }, do_accept, 2 },
    // IP4
    { "IP4_CONFIG", "", { ARG_STR, ARG_STR, 0, 0, 0 }, do_ip4_config, 1 },
    { "PING", "", { ARG_STR, ARG_STR, 0, 0, 0 }, do_ping, 1 },

    { "TEST", "", { 0, 0, 0, 0, 0 }, do_test, 0 },
    { "EXPECT", "", { ARG_STR, ARG_STR, ARG_STR, 0, 0 }, do_expect, 2 },
    CLI_END_OF_COMMANDS,
};

int main(int argc, char **argv)
{
    return cli_main(argc, argv, NULL, NULL, NULL);
}
