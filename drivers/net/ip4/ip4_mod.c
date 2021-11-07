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

 /* - */
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

/* - */
int ip4_readaddr(uint8_t *addr, const char *str)
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
    memcpy(addr, buf, IP4_ALEN);
    uint16_t port = 0;
    if (*p == ':') {
        p++;
        port = htons(strtol(str, &p, 10));
        memcpy(&addr[4], &port, 2);
    }
    return 0;
}

/* - */
char *ip4_writeip(const uint8_t *ip, char *buf, int len)
{
    snprintf(buf, len, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    return buf;
}

/* - */
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
        /*memset(info->broadcast.ip, 0xff, IP4_ALEN);
        memset(info->broadcast.addr, 0xff, ETH_ALEN);
        info->broadcast.net = ifnet;
        info->broadcast.ttl = info->ttl;*/
        splock_init(&info->lock);
        splock_init(&info->qry_lock);
        bbtree_init(&info->qry_arps);
        bbtree_init(&info->qry_pings);
        hmp_put(&master->ifinfos, key, lg, info);
    }
    splock_unlock(&master->lock);
    return info;
}

/* - */
ip4_master_t *ip4_readmaster(netstack_t *stack)
{
    assert(stack);
    nproto_t *proto = net_protocol(stack, NET_AF_IP4);
    ip4_master_t *master = proto->data;
    assert(master != NULL);
    return master;
}


/* - */
void ip4_checkup(ifnet_t *net, int event, int param)
{
    if (event != NET_EV_LINK || !(net->flags & NET_CONNECTED))
        return;

    ip4_info_t *info = ip4_readinfo(net);
    if (info->subnet.ifnet == NULL)
        dhcp_discovery(net);
    else
        arp_whois(net, info->subnet.address, NULL);
}

ip4_class_t ip4_classes[] = {
    { "Any", { 0, 0, 0, 0} },
    { "Class A", { 255, 0, 0, 0} },
    { "Class B", { 255, 255, 0, 0} },
    { "Class C", { 255, 255, 255, 0} },
    { "Class D", { 255, 255, 255, 255} },
    { "Class E", { 255, 255, 255, 255} },
    { "Localhost", { 255, 0, 0, 0} },
};

/* - */
void ip4_setip(ifnet_t *net, const uint8_t *ip, const uint8_t *submsk, const uint8_t *gateway)
{
    char tmp[16];
    ip4_master_t *master = ip4_readmaster(net->stack);
    ip4_info_t *info = ip4_readinfo(net);
    splock_lock(&info->lock);

    int ipclass = ip4_identify(ip);
    assert(ipclass <= 6);
    if (ipclass < 0)
        return; // Invalid IP -- TODO trigger an error

    memcpy(info->subnet.address, ip, IP4_ALEN);
    memcpy(info->subnet.submask, ip4_classes[ipclass].mask, IP4_ALEN);
    for (int i = 0; i < IP4_ALEN; ++i)
        info->subnet.broadcast[i] = info->subnet.address[i] | ~info->subnet.submask[i];
    if (gateway != NULL)
        memcpy(info->subnet.gateway, gateway, IP4_ALEN);
    else
        memset(info->subnet.gateway, 0, IP4_ALEN);
    splock_lock(&master->lock);
    if (info->subnet.ifnet != NULL)
        ll_remove(&master->subnets, &info->subnet.node);
    info->subnet.ifnet = net;
    ll_append(&master->subnets, &info->subnet.node);
    splock_unlock(&master->lock);

    ip4_writeip(ip, tmp, 16);
    kprintf(-1, "Setup IP %s:%d, %s\n", net->stack->hostname, net->idx, tmp);
    splock_unlock(&info->lock);

    arp_whois(net, ip, NULL);
    if (gateway != 0 && memcmp(ip, gateway, IP4_ALEN) != 0)
        arp_whois(net, gateway, NULL);
}

/* - */
int ip4_teardown_stack(netstack_t *stack)
{
    nproto_t *proto = net_protocol(stack, NET_AF_IP4);
    nproto_t *tcp_proto = net_protocol(stack, NET_AF_TCP);
    nproto_t *udp_proto = net_protocol(stack, NET_AF_UDP);
    ip4_master_t *info = proto->data;
    if (info->ifinfos.count > 0)
        return -1;
    if (info->tcp_ports.count_ > 0)
        return -1;
    if (info->udp_ports.count_ > 0)
        return -1;

    net_unregister(stack, ip4_checkup);
    net_rm_protocol(stack, NET_AF_TCP);
    net_rm_protocol(stack, NET_AF_UDP);
    net_rm_protocol(stack, NET_AF_IP4);

    eth_unregister(stack, ETH_IP4, ip4_receive);
    eth_unregister(stack, ETH_ARP, arp_receive);
    lo_unregister(stack, ETH_IP4, ip4_receive);

    hmp_destroy(&info->ifinfos);
    kfree(info);
    kfree(proto);
    kfree(tcp_proto);
    kfree(udp_proto);
    return 0;
}


/* - */
int ip4_start(netstack_t *stack)
{
    nproto_t *proto = net_protocol(stack, NET_AF_IP4);
    if (proto != NULL)
        return -1;

    proto = kalloc(sizeof(nproto_t));
    ip4_master_t *master = kalloc(sizeof(ip4_master_t));
    splock_init(&master->plock);
    splock_init(&master->lock);
    splock_init(&master->rlock);
    bbtree_init(&master->udp_ports);
    bbtree_init(&master->tcp_ports);
    hmp_init(&master->ifinfos, 8);
    proto->data = master;
    ip4_proto(proto);
    proto->teardown = ip4_teardown_stack;
    net_set_protocol(stack, NET_AF_IP4, proto);

    nproto_t *proto_tcp = kalloc(sizeof(nproto_t));
    tcp_proto(proto_tcp);
    net_set_protocol(stack, NET_AF_TCP, proto_tcp);

    nproto_t *proto_udp = kalloc(sizeof(nproto_t));
    udp_proto(proto_udp);
    net_set_protocol(stack, NET_AF_UDP, proto_udp);

    eth_handshake(stack, ETH_IP4, ip4_receive);
    eth_handshake(stack, ETH_ARP, arp_receive);
    lo_handshake(stack, ETH_IP4, ip4_receive);
    net_handler(stack, ip4_checkup);
    return 0;
}

/* - */
void ip4_config(ifnet_t *net, const char *str)
{
    uint8_t buf[IP4_ALEN];
    char *ptr;
    char *arg;
    char *cpy = kstrdup(str);
    ip4_info_t *info = ip4_readinfo(net);
    for (arg = strtok_r(cpy, " \t\n", &ptr); arg != NULL; arg = strtok_r(NULL, " \t\n", &ptr)) {
        if (strcmp(arg, "dhcp-server") == 0) {
            info->use_dhcp_server = true;
            if (info->subnet.address[0] == 0) {
                ip4_readip(buf, "192.168.0.1");
                ip4_setip(net, buf, NULL, buf);
            } else
                ip4_setip(net, info->subnet.address, NULL, info->subnet.address);
        } else if (memcmp(arg, "ip=", 3) == 0) {
            ip4_readip(buf, arg + 3);
            ip4_setip(net, buf, NULL, NULL);
        }
    }

    kfree(cpy);
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

int ip4_setup()
{
    return ip4_start(net_stack());
}

int ip4_teardown()
{
    return -1;
}

EXPORT_MODULE(ip4, ip4_setup, ip4_teardown);

