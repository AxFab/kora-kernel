#include "ip4.h"

#define ARP_HW_ETH htonw(1)
#define ARP_PC_IP htonw(0x0800)

#define ARP_REQUEST htonw(1)
#define ARP_REPLY htonw(2)

#define ARP_TIMEOUT 10

typedef struct arp_header arp_header_t;

PACK(struct arp_header {
    uint16_t hardware;
    uint16_t protocol;
    uint8_t hw_length;
    uint8_t pc_length;
    uint16_t opcode;
});

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int arp_packet(ifnet_t* net, const uint8_t* mac, const uint8_t* ip, int opcode)
{
    skb_t* skb = net_packet(net);
    if (unlikely(skb == NULL))
        return -1;
    if (eth_header(skb, mac, ETH_ARP) != 0)
        return net_skb_trash(skb);

    net_skb_log(skb, ",arp");
    arp_header_t* header = net_skb_reserve(skb, sizeof(arp_header_t));
    if (header == NULL) {
        net_skb_log(skb, ":Unexpected end of data");
        return -1;
    }

    ip4_info_t* info = ip4_readinfo(skb->ifnet);
    header->hardware = ARP_HW_ETH;
    header->protocol = ARP_PC_IP;
    header->hw_length = ETH_ALEN;
    header->pc_length = IP4_ALEN;
    header->opcode = opcode;
    net_skb_write(skb, net->hwaddr, ETH_ALEN);
    net_skb_write(skb, info->ip, IP4_ALEN);
    net_skb_write(skb, mac, ETH_ALEN);
    net_skb_write(skb, ip, IP4_ALEN);

    return net_skb_send(skb);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int arp_whois(ifnet_t* net, const uint8_t* ip)
{
    uint8_t broadcast[ETH_ALEN];
    memset(broadcast, 0xff, ETH_ALEN);
    return arp_packet(net, broadcast, ip, ARP_REQUEST);
}

int arp_receive(skb_t* skb)
{
    net_skb_log(skb, ",arp");
    arp_header_t* header = net_skb_reserve(skb, sizeof(arp_header_t));
    if (header == NULL) {
        net_skb_log(skb, ":Unexpected end of data");
        return -1;
    }

    if (header->hardware != ARP_HW_ETH || header->protocol != ARP_PC_IP
        || header->hw_length != ETH_ALEN || header->pc_length != IP4_ALEN) {
        net_skb_log(skb, ":Unsupported format");
        return -1;
    }

    uint8_t source_mac[ETH_ALEN];
    uint8_t source_ip[IP4_ALEN];
    uint8_t target_mac[ETH_ALEN];
    uint8_t target_ip[IP4_ALEN];
    net_skb_read(skb, source_mac, ETH_ALEN);
    net_skb_read(skb, source_ip, IP4_ALEN);
    net_skb_read(skb, target_mac, ETH_ALEN);
    net_skb_read(skb, target_ip, IP4_ALEN);

    ip4_info_t* info = ip4_readinfo(skb->ifnet);
    if (header->opcode == ARP_REQUEST && memcmp(info->ip, target_ip, IP4_ALEN) == 0)
        arp_packet(skb->ifnet, source_mac, source_ip, ARP_REPLY);
    else if (header->opcode == ARP_REPLY)
        ip4_route_add(skb->ifnet, source_ip, source_mac);
    
    free(skb);
    return 0;
}

