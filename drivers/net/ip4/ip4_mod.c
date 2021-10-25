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
#include <threads.h>
#include "ip4.h"
#include <kernel/core.h>


int ip4_readip(uint8_t *ip, const char *str)
{
    int i;
    char *p;
    uint8_t buf[4];
    for (i = 0; i < IP4_ALEN; ++i) {
        long v = strtol(str, &p, 10);
        if (v < 0 || v > 255)
            return -1;
        buf[i] = (uint8_t)v;
        if (p == str || (i != IP4_ALEN - 1 && *p != '.'))
            return -1;
        str = p + 1;
    }
    memcpy(ip, buf, IP4_ALEN);
    return 0;
}

char *ip4_writeip(const uint8_t *ip, char *buf, int len)
{
    snprintf(buf, len, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    return buf;
}

ip4_info_t *ip4_readinfo(ifnet_t *ifnet)
{
    ip4_info_t *info = ifnet->ipv4;
    if (info == NULL) {
        splock_lock(&ifnet->lock);
        info = kalloc(sizeof(ip4_info_t));
        ifnet->ipv4 = info;
        info->ttl = 128;
        info->use_dhcp = true;
        memset(info->broadcast.ip, 0xff, IP4_ALEN);
        memset(info->broadcast.addr, 0xff, ETH_ALEN);
        info->broadcast.net = ifnet;
        info->broadcast.ttl = info->ttl;
        splock_unlock(&ifnet->lock);
    }
    return info;
}

ip4_master_t *ip4_readmaster(netstack_t *stack)
{
    ip4_master_t *master = stack->ipv4;
    if (master == NULL) {
        splock_lock(&stack->lock);
        master = kalloc(sizeof(ip4_master_t));
        stack->ipv4 = master;
        hmp_init(&master->tcp_ports, 8);
        hmp_init(&master->udp_ports, 8);
        hmp_init(&master->routes, 8);
        splock_unlock(&stack->lock);
    }
    return master;
}

void ip4_checkup(ifnet_t *net) // TODO - How this can be called
{
    if (!(net->flags & NET_CONNECTED))
        return;

    ip4_info_t *info = ip4_readinfo(net);
    if (info->ip[0] == 0)
        dhcp_discovery(net);
}


void ip4_setip(ifnet_t *net, const uint8_t *ip, const uint8_t *submsk, const uint8_t *gateway)
{
    char tmp[16];
    ip4_info_t *info = ip4_readinfo(net);
    splock_lock(&info->lock);

    memcpy(info->ip, ip, IP4_ALEN);

    if (gateway != NULL)
        memcpy(info->gateway, gateway, IP4_ALEN);

    info->submsk[0] = 0;
    if (submsk != NULL)
        memcpy(info->submsk, submsk, IP4_ALEN);
    if (info->submsk[0] != 0xff) {
        memset(info->submsk, 0xff, IP4_ALEN);
        info->submsk[3] = 0;
        if (info->ip[0] < 192)
            info->submsk[2] = 0;
        if (info->ip[0] < 128) // unlikely, 127 should not be allowed!
            info->submsk[1] = 0;
    }

    for (int i = 0; i < IP4_ALEN; ++i)
        info->broadcast.ip[i] = info->ip[i] | ~info->submsk[i];

    kprintf(-1, "Setup IP %s:%d, %s\n", net->stack->hostname, net->idx, ip4_writeip(info->ip, tmp, 16));
    splock_unlock(&info->lock);

    arp_whois(net, info->ip);
    if (info->gateway[0] != 0 && memcmp(info->ip, info->gateway, IP4_ALEN) != 0)
        arp_whois(net, info->gateway);
}

void ip4_start(netstack_t *stack)
{
    eth_handshake(stack, ETH_IP4, ip4_receive);
    eth_handshake(stack, ETH_ARP, arp_receive);
}

void ip4_config(ifnet_t *net, const char *str)
{
    uint8_t buf[IP4_ALEN];
    char *ptr;
    char *arg;
    char *cpy = strdup(str);
    ip4_info_t *info = ip4_readinfo(net);
    splock_lock(&net->lock);
    for (arg = strtok_r(cpy, " \t\n", &ptr); arg != NULL; arg = strtok_r(NULL, " \t\n", &ptr)) {
        if (strcmp(arg, "dhcp-server") == 0) {
            info->use_dhcp_server = true;
            if (info->ip[0] == 0) {
                ip4_readip(buf, "192.168.0.1");
                ip4_setip(net, buf, NULL, buf);
            } else
                ip4_setip(net, info->ip, NULL, info->ip);
        } else if (memcmp(arg, "ip=", 3) == 0) {
            ip4_readip(buf, arg + 3);
            ip4_setip(net, buf, NULL, NULL);
        }

    }

    kfree(cpy);
}


socket_t *ip4_lookfor_socket(ifnet_t *net, uint16_t port, bool stream, const uint8_t *ip)
{
    ip4_master_t *master = ip4_readmaster(net->stack);
    hmap_t *map = stream ? &master->tcp_ports : &master->udp_ports;
    splock_lock(&master->lock);
    ip4_port_t *pt = hmp_get(map, (char *)&port, sizeof(uint16_t));
    splock_unlock(&master->lock);
    if (pt == NULL)
        return NULL;

    if (!pt->listen) {
        // TODO - Check this ip is allowed to answer?
        return pt->socket;
    }

    socket_t *client = hmp_get(&pt->clients, ip, IP4_ALEN);
    if (client != NULL)
        return client;

    // TODO - Create a new incoming socket...
    // socket_t* incoming = net_socket(net->stack, NET_AF_IP4);
    // net_connect(incoming, ip, port);
    // net_bind(incoming, NULL, port);
    // Inform `client` of the new socket !
    return NULL;
}

void ip4_setup()
{
#ifdef KORA_KRN
    ip4_start(net_stack());
#endif
}

void ip4_teardown()
{
}

EXPORT_MODULE(ip4, ip4_setup, ip4_teardown);

