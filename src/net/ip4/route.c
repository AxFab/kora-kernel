#include "ip4.h"
#include <kernel/core.h>


ip4_route_t* ip4_route_broadcast(ifnet_t *net)
{
    ip4_info_t* info = ip4_readinfo(net);
    return &info->broadcast;
}

ip4_route_t* ip4_route(netstack_t *stack, const uint8_t *ip)
{
    ip4_master_t* master = ip4_readmaster(stack);
    splock_lock(&master->lock);
    ip4_route_t *route = hmp_get(&master->routes, ip, 4);
    splock_unlock(&master->lock);
    return route;
}

void ip4_route_add(ifnet_t* net, const uint8_t* ip, const uint8_t* mac)
{
    char buf_mac[20];
    char buf_ip[20];
    ip4_writeip(ip, buf_ip, 20);
    eth_writemac(mac, buf_mac, 20);
    kprintf(-1, "New route IP:%s -> MAC:%s\n", buf_ip, buf_mac);

    ip4_info_t* info = ip4_readinfo(net);
    ip4_route_t* route = NULL;

    ip4_master_t* master = ip4_readmaster(net->stack);
    splock_lock(&master->lock);
    route = hmp_get(&master->routes, ip, 4);
    splock_unlock(&master->lock);
    if (route != NULL) // TODO - Check we have the correct route
        return;

    route = kalloc(sizeof(ip4_route_t));
    memcpy(route->ip, ip, IP4_ALEN);
    memcpy(route->addr, mac, ETH_ALEN);
    route->net = net;
    route->ttl = info->ttl;

    splock_lock(&master->lock);
    hmp_put(&master->routes, ip, 4, route);
    splock_unlock(&master->lock);
}
