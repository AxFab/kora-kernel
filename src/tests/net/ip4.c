#include "ip4.h"
#include "eth.h"
#include "lo.h"

#define IP4_ALEN  4

typedef struct ip4_cnx ip4_cnx_t;
typedef struct ip4_route ip4_route_t;
typedef struct ip4_frame ip4_frame_t;

struct ip4_cnx {
    ifnet_t *ifnet;
    uint8_t address[IP4_ALEN];
    uint8_t mask[IP4_ALEN];
    uint8_t gateway[IP4_ALEN];
    llnode_t node;
};

struct ip4_route {
    uint8_t addr[IP4_ALEN];
    uint8_t mask[IP4_ALEN];
    uint8_t mac[ETH_ALEN];
    ifnet_t *ifnet;
    int ttl;
};

struct ip4_frame {
    unsigned id;
    unsigned off;
    unsigned len;
    unsigned type;
};


typedef struct ip4_header ip4_header_t;

struct ip4_header {
    uint8_t header_length: 4;
    uint8_t version: 4;
    uint8_t service_type;
    uint16_t length;
    uint16_t identifier;
    uint16_t offset;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint8_t source[4];
    uint8_t target[4];
};

#define  ETH_IP4  1
#define  LO_IP4  2



uint16_t ip4_checksum(uint16_t *ptr, size_t len)
{
    int i, sum = 0;
    for (i = -(int)(len / 2); i < 0; ++i)
        sum += ntohw(ptr[i]);
    if (sum > 0xFFFF)
        sum = (sum >> 16) + (sum & 0xFFFF);
    return htonw(~(sum & 0xFFFF) & 0xFFFF);
}

int ip4_check_cnx(ip4_cnx_t *cnx, uint8_t *ip)
{
    uint32_t addr = htonl(*((uint32_t*)ip));
    uint32_t mask = htonl(*((uint32_t*)cnx->mask));
    uint32_t lan = htonl(*((uint32_t*)cnx->address));
    return (addr & mask) == (lan & mask);
}

ip4_route_t *ip4_arp_whois(ifnet_t *ifnet, uint8_t *ip, int retry, long delay)
{

    return NULL;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int ip4_receive(skb_t *skb)
{
    ip4_header_t *head =  net_pointer(skb, sizeof(ip4_header_t));
    if (head == NULL)
        return -1;
    uint16_t checksum = head->checksum;
    // uint16_t length = htonw(header->length) - sizeof(IP4_header_t);
    head->checksum = 0;
    if (checksum != ip4_checksum((uint16_t*)head, sizeof(ip4_header_t)))
        return -1;

    switch (head->protocol) {
    // case IP4_TCP:
    //     return tcp_receive(skb, length);
    // case IP4_UDP:
    //     return udp_receive(skb, length);
    // case IP4_ICMP:
    //     if (header->offset != 0)
    //         return -1;
    //     return icmp_receive(skb, length);
    default:
        return -1;
    }
}




skb_t *ip4_packet(ifnet_t *ifnet, ip4_route_t *route, ip4_frame_t *frame)
{
    uint8_t ip_sender[IP4_ALEN];
    // Find IP of ifnet, if none failure!

    skb_t *skb;
    if (ifnet->type == INET_ETH)
        skb = eth_packet(ifnet, route->mac, ETH_IP4);
    else if (ifnet->type == INET_LO)
        skb = lo_packet(ifnet, LO_IP4);
    else
        return NULL;

    ip4_header_t *head =  net_pointer(skb, sizeof(ip4_header_t));
    head->version = 4;
    head->header_length = 5;

    head->ttl = route->ttl;

    memcpy(head->source, ip_sender, IP4_ALEN);
    memcpy(head->target, route->addr, IP4_ALEN);
    head->checksum = 0;
    head->checksum = ip4_checksum((uint16_t*)head, sizeof(ip4_header_t));
    return skb;
}


skb_t *ip4_sock_packet(socket_t *sock)
{
    ip4_route_t *route = sock->route;
    ip4_frame_t *frame = NULL;
    skb_t *skb = ip4_packet(route->ifnet, route, frame);
    return skb;
}

int ip4_connect(socket_t *socket, uint8_t *ip)
{
    ip4_cnx_t *cnx = NULL;
    // look on route cache table
    ip4_route_t *route = NULL;
    socket->connect = ip4_connect;
    socket->packet = ip4_sock_packet;
    for ll_each(&kNET.ip4_cnxs, cnx, ip4_cnx_t, node) {
        if (ip4_check_cnx(cnx, ip))
            break;
    }
    if (cnx != NULL) {
        // Send ARP request
        route = ip4_arp_whois(cnx->ifnet, ip, 3, 500);
        socket->route = route;
        return route ? 0 : -1;
    }

    // Look for a gateway
    socket->route = route;
    return route ? 0 : -1;
}

void ip4_config(ifnet_t *ifnet, uint8_t *addr, uint8_t *mask, uint8_t *gateway)
{
    ip4_cnx_t *cnx = NULL;
    for ll_each(&kNET.ip4_cnxs, cnx, ip4_cnx_t, node) {
        if (cnx->ifnet == ifnet)
            break;
    }


    if (cnx == NULL) {
        cnx = kalloc(sizeof(ip4_cnx_t));
        cnx->ifnet = ifnet;
        ll_append(&kNET.ip4_cnxs, &cnx->node);
    }

    memcpy(cnx->address, addr, IP4_ALEN);
    memcpy(cnx->mask, mask, IP4_ALEN);
    memcpy(cnx->gateway, gateway, IP4_ALEN);
}



void ip4_setup()
{
    // eth_register(ETH_IP4, ip4_receive);
    // lo_register(ETH_LO, ip4_receive);

    // net_protocol(INET_RAW_IP4, ip4_config, ip4_sock_packet
    // net_protocol(INET_TCP_IP4, ip4_config, tcp_sock_packet
    // net_protocol(INET_UDP_IP4, ip4_config, udp_sock_packet

    // Start `dhcpd'
}

