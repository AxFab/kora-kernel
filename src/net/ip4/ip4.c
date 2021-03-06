#include <kernel/core.h>
#include "ip4.h"

typedef struct ip4_header ip4_header_t;


PACK(struct ip4_header {
    uint8_t header_length : 4;
    uint8_t version : 4;
    uint8_t service_type;
    uint16_t length;
    uint16_t identifier;
    uint16_t offset;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint8_t source[IP4_ALEN];
    uint8_t target[IP4_ALEN];
});

uint16_t ip4_checksum(uint16_t* ptr, int len)
{
    int i, sum = 0;
    for (i = -(int)(len / 2); i < 0; ++i)
        sum += ntohw(ptr[i]);
    if (sum > 0xFFFF)
        sum = (sum >> 16) + (sum & 0xFFFF);
    return htonw(~(sum & 0xFFFF) & 0xFFFF);
}

int ip4_header(skb_t* skb, ip4_route_t *route, int protocol, int length, uint16_t identifier, uint16_t offset)
{
    switch (route->net->protocol) {
    case NET_AF_ETH:
        if (eth_header(skb, route->addr, ETH_IP4) != 0)
            return -1;
        break;
    default: 
        net_skb_log(skb, "ipv4:Unknown protocol");
        return -1;
    }

    net_skb_log(skb, ",ipv4");
    ip4_info_t* info = ip4_readinfo(skb->ifnet);
    ip4_header_t* header = net_skb_reserve(skb, sizeof(ip4_header_t));
    if (header == NULL) {
        net_skb_log(skb, ":Unexpected end of data");
        return -1;
    }

    memset(header, 0, sizeof(ip4_header_t));
    header->version = 4; // Ipv4
    header->header_length = 5; // No option
    header->service_type = 0; // No features
    header->length = htonw(sizeof(ip4_header_t) + length);
    header->identifier = identifier;
    header->offset = offset;
    header->ttl = route->ttl;
    header->protocol = protocol;
    memcpy(header->source, info->ip, IP4_ALEN);
    memcpy(header->target, route->ip, IP4_ALEN);

    header->checksum = ip4_checksum((uint16_t*)header, sizeof(ip4_header_t));
    return 0;
}


int ip4_receive(skb_t* skb)
{
    net_skb_log(skb, ",ipv4");
    ip4_info_t* info = ip4_readinfo(skb->ifnet);
    ip4_header_t* header = net_skb_reserve(skb, sizeof(ip4_header_t));
    if (header == NULL) {
        net_skb_log(skb, ":Unexpected end of data");
        return -1;
    }

    uint16_t checksum = header->checksum;
    uint16_t length = htonw(header->length) - sizeof(ip4_header_t);
    header->checksum = 0;

    header->checksum = ip4_checksum((uint16_t*)header, sizeof(ip4_header_t));
    if (checksum != header->checksum) {
        net_skb_log(skb, ":Bad checksum");
        return -1;
    }

    // TODO -- If that's not for me, should I re-route !?

    // TODO - Save remote address ?
    switch (header->protocol) {
    case IP4_TCP:
        return tcp_receive(skb, length, header->identifier, header->offset);
    case IP4_UDP:
        return udp_receive(skb, length, header->identifier, header->offset);
    case IP4_ICMP:
        return icmp_receive(skb, length);
    default:
        net_skb_log(skb, ":Unknown protocol");
        return -1;
    }
}


