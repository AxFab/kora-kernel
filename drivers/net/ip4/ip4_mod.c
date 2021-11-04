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
    assert(ifnet && ifnet->stack && ifnet->proto);
    ip4_master_t *master = ip4_readmaster(ifnet->stack);
    char key[8];
    int lg = snprintf(key, 8, "%s.%d", ifnet->proto->name, ifnet->idx);
    splock_lock(&master->lock);
    ip4_info_t *info = hmp_get(&master->ifinfos, key, lg);
    if (info == NULL) {
        info = kalloc(sizeof(ip4_info_t));
        info->ttl = 128;
        info->use_dhcp = true;
        memset(info->broadcast.ip, 0xff, IP4_ALEN);
        memset(info->broadcast.addr, 0xff, ETH_ALEN);
        info->broadcast.net = ifnet;
        info->broadcast.ttl = info->ttl;
        hmp_put(&master->ifinfos, key, lg, info);
    }
    splock_unlock(&master->lock);
    return info;
}

ip4_master_t *ip4_readmaster(netstack_t *stack)
{
    assert(stack);
    nproto_t *proto = net_protocol(stack, NET_AF_IP4);
    ip4_master_t *master = proto->data;
    assert(master != NULL);
    return master;
}

void ip4_checkup(ifnet_t *net, int event, int param) 
{
    if (event != NET_EV_LINK || !(net->flags & NET_CONNECTED))
        return;

    ip4_info_t *info = ip4_readinfo(net);
    if (info->ip[0] == 0)
        dhcp_discovery(net);
    else
        arp_whois(net, info->ip);
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

//nproto_t ip4_tcp_proto = {
//};

int ip4_start(netstack_t *stack)
{
    nproto_t* proto = net_protocol(stack, NET_AF_IP4);
    if (proto != NULL)
        return -1;

    proto = kalloc(sizeof(nproto_t));
    ip4_master_t* master = kalloc(sizeof(ip4_master_t));
    hmp_init(&master->sockets, 8);
    hmp_init(&master->routes, 8);
    proto->data = master;
    ip4_proto(proto);
    net_set_protocol(stack, NET_AF_IP4, proto);

    eth_handshake(stack, ETH_IP4, ip4_receive);
    eth_handshake(stack, ETH_ARP, arp_receive);
    lo_handshake(stack, ETH_IP4, ip4_receive);
    net_handler(stack, ip4_checkup);

    nproto_t* proto_tcp = kalloc(sizeof(nproto_t));
    tcp_proto(proto_tcp);
    net_set_protocol(stack, NET_AF_TCP, proto_tcp);

    nproto_t* proto_udp = kalloc(sizeof(nproto_t));
    udp_proto(proto_udp);
    net_set_protocol(stack, NET_AF_UDP, proto_udp);

    return 0;
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



// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

struct ip4_port
{
    bbtree_t connected;
};

int ip4_bind_key(char *key, uint16_t port, uint16_t method)
{
    memcpy(key, &port, sizeof(uint16_t));
    memcpy(&key[sizeof(uint16_t)], &method, sizeof(uint16_t));
    return 2 * sizeof(uint16_t);
}

/* Bind a socket to a local port in order to redirect incoming packet */
int ip4_bind_socket(socket_t *sock)
{
    char key[8];
    uint16_t port = *((uint16_t *)&sock->laddr[4]);
    uint16_t method = 0;
    if (sock->protocol == NET_AF_TCP)
        method = IP4_TCP;
    else if (sock->protocol == NET_AF_UDP)
        method = NET_AF_UDP;
    else if (sock->protocol == NET_AF_IP4) {
        // TODO -- look on socket->data...
        method == 0;
    }

    int len = ip4_bind_key(key, port, method);
    ip4_master_t *master = ip4_readmaster(sock->stack);
    splock_lock(&master->lock);
    ip4_port_t *iport = hmp_get(&master->sockets, key, len);
    if (iport == NULL) {
        iport = kalloc(sizeof(ip4_port_t));
        bbtree_init(&iport->connected);
        hmp_put(&master->sockets, key, len, sock);
    }
    uint32_t ipval = *((uint32_t *)sock->laddr);
    bbslot_t *slot = bbtree_search_eq(&iport->connected, ipval, bbslot_t, node);
    if (slot != NULL) {
        splock_unlock(&master->lock);
        return -1;
    }

    // TODO -- Check address is not of a server !?

    slot = kalloc(sizeof(bbslot_t));
    slot->data = sock;
    slot->node.value_ = ipval;
    bbtree_insert(&iport->connected, slot);
    splock_unlock(&master->lock);
    return 0;
}

socket_t *ip4_lookfor_socket(ifnet_t *net, uint16_t port, uint16_t method, const uint8_t *ip)
{
    char key[8];
    int len = ip4_bind_key(key, port, method);
    ip4_master_t *master = ip4_readmaster(net->stack);
    splock_lock(&master->lock);
    ip4_port_t *iport = hmp_get(&master->sockets, key, len);
    if (iport == NULL) {
        splock_unlock(&master->lock);
        return NULL;
    }

    uint32_t ipval = *((uint32_t *)ip);
    bbslot_t *slot = bbtree_search_eq(&iport->connected, ipval, bbslot_t, node);
    socket_t *sock = slot ? slot->data : NULL;
    // TODO -- Look for server too
    splock_unlock(&master->lock);
    return sock;
}

int ip4_setup()
{
    return ip4_start(net_stack());
}

int ip4_teardown()
{
    return -1;
}

EXPORT_MODULE(ip4, ip4_setup, ip4_teardown);

