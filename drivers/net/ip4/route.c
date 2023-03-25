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
#include "ip4.h"
#include <kernel/core.h>

/* Identify the IP class of an IP address */
int ip4_identify(const uint8_t *ip)
{
    uint32_t ipval = ntohl(*((const uint32_t *)ip));
    int ip_class = 0;
    if (ipval == 0)
        ip_class = 0; // INET_ANY
    else if (ipval < 0x01000000)
        ip_class = -1; // Unautorized ?
    else if (ipval < 0x7F000000)
        ip_class = 1; // CLASS A - mask 255.0.0.0 (1.0.0.0 - 127.0.0.0)
    else if (ipval < 0x80000000)
        ip_class = 6; // LOCALHOST - mask 255.0.0.0 (127.x.x.x)
    else if (ipval < 0xC0000000)
        ip_class = 2; // CLASS B - mask 255.255.0.0  (128.0.0.0 - 191.255.0.0)
    else if (ipval < 0xE0000000)
        ip_class = 3; // CLASS C - mask 255.255.255.0  (192.0.0.0 - 223.255.255.0)
    else if (ipval < 0xF0000000)
        ip_class = 4; // CLASS D - mask 255.255.255.? (224.0.0.0 - 239.255.255.25)
    else
        ip_class = 5; // CLASS E - mask 255.255.255.? (240.0.0.0 - 255.255.255.255)

    if (ip_class == 4 || ip_class == 5)
        ip_class = -1; // Don't support multicast yet
    return ip_class;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

/* Look for a gateway when needing to reach outside of the sub-network */
static ip4_subnet_t *ip4_find_gateway(netstack_t *stack)
{
    ip4_master_t *master = ip4_readmaster(stack);
    splock_lock(&master->rlock);
    // TODO -- Should we use a round robin for ifnet selection (gold feature)
    // Regular routing the gateway is fixed !
    // TODO -- We should mark bad gateway (without internet connction)
    ip4_subnet_t *subnet;
    for ll_each(&master->subnets, subnet, ip4_subnet_t, node)
    {
        uint32_t gateway = ntohl(*((const uint32_t *)subnet->gateway));
        if (gateway != 0) {
            splock_unlock(&master->rlock);
            return subnet;
        }
    }
    splock_unlock(&master->rlock);
    return NULL;
}

/* Validate a route once we found it and look for hardware address */
static int ip4_check_route(ip4_subnet_t *subnet, ip4_route_t *route, const uint8_t *ip)
{
    ip4_info_t *info = ip4_readinfo(subnet->ifnet);
    // The address is part of this network
    route->net = subnet->ifnet;
    route->ttl = info->ttl;
    if (subnet->ifnet->protocol == NET_AF_LO)
        return 0;
    else if (subnet->ifnet->protocol != NET_AF_ETH)
        return -1; // Not supported

    // TODO -- Look on cache...

    net_qry_t qry;
    arp_whois(subnet->ifnet, ip, &qry);
#if defined WIN32 || defined KORA_KRN
    struct timespec tm = { .tv_sec =  1, .tv_nsec = 0 };
#else
    xtime_t rl = xtime_read(XTIME_CLOCK);
    struct timespec tm = { .tv_sec = rl / 1000000LL + 1, .tv_nsec = 0 };
#endif
    int rcnd = cnd_timedwait(&qry.cnd, &qry.mtx, &tm);
    if (rcnd == thrd_success)
        mtx_unlock(&qry.mtx);
    if (!qry.success) {
        arp_forget(subnet->ifnet, &qry);
        return -1;
    }

    // TODO -- Put on cache ?
    memcpy(route->addr, qry.res, ETH_ALEN);
    return 0;
}

/* Find a route for broadcast */
int ip4_broadcast_route(netstack_t *stack, ip4_route_t *route, ifnet_t *ifnet)
{
    ip4_subnet_t *subnet = NULL;
    if (ifnet != NULL) {
        ip4_info_t *info = ip4_readinfo(ifnet);
        route->ttl = info->ttl;
        if (info->subnet.ifnet != NULL)
            subnet = &info->subnet;
    }

    if (subnet != NULL) {
        uint32_t mkval = ntohl(*((const uint32_t *)subnet->submask));
        uint32_t ip = ntohl(*((const uint32_t *)subnet->address));
        ip = ip | ~mkval;
        memcpy(route->ip, &ip, IP4_ALEN);
    } else {
        if (ifnet != NULL) {
            route->net = ifnet;
        }
        else {
            subnet = ip4_find_gateway(stack);
            if (subnet == NULL)
                return -1;
            ip4_info_t *info = ip4_readinfo(subnet->ifnet);
            route->net = subnet->ifnet;
            route->ttl = info->ttl;
        }
        uint32_t ip = 0xFFFFFFFF;
        memcpy(route->ip, &ip, IP4_ALEN);
    }

    if (route->net->protocol == NET_AF_ETH)
        memcpy(route->addr, mac_broadcast, ETH_ALEN);
    else
        return -1;

    return 0;
}

/* Find a route for an IP */
int ip4_find_route(netstack_t *stack, ip4_route_t *route, const uint8_t *ip)
{
    ip4_subnet_t *subnet;
    ip4_master_t *master = ip4_readmaster(stack);
    uint32_t ipval = ntohl(*((const uint32_t *)ip));
    memcpy(route->ip, ip, IP4_ALEN);
    splock_lock(&master->rlock);
    for ll_each(&master->rtable, subnet, ip4_subnet_t, node)
    {
        uint32_t mkval = ntohl(*((const uint32_t *)subnet->submask));
        uint32_t ntval = ntohl(*((const uint32_t *)subnet->address));
        if ((ipval & mkval) != (ntval & mkval))
            continue;
        splock_unlock(&master->rlock);
        return ip4_check_route(subnet, route, ip);
    }

    for ll_each(&master->subnets, subnet, ip4_subnet_t, node)
    {
        uint32_t mkval = ntohl(*((const uint32_t *)subnet->submask));
        uint32_t ntval = ntohl(*((const uint32_t *)subnet->address));
        if ((ipval & mkval) != (ntval & mkval))
            continue;
        splock_unlock(&master->rlock);
        return ip4_check_route(subnet, route, ip);
    }

    splock_unlock(&master->rlock);

    // If that's outside - look for a gateway
    subnet = ip4_find_gateway(stack);
    if (subnet == NULL)
        return -1;
    
    route->net = subnet->ifnet;
    memcpy(route->addr, subnet->gateway_addr, subnet->ifnet->proto->addrlen);
    return 0;
}
