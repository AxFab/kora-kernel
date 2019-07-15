#include "eth.h"


skb_t *eth_packet(ifnet_t *ifnet, uint8_t *addr, uint16_t type)
{
    skb_t *skb = net_packet(ifnet);
    eth_header_t *head = net_pointer(skb, sizeof(eth_header_t));
    memcpy(head->sender, ifnet->hwaddr, ETH_ALEN);
    memcpy(head->target, addr, ETH_ALEN);
    head->type = type;
    strcat(skb->log, ".eth");
    return skb;
}


int eth_receive(skb_t *skb)
{
    eth_header_t *head = net_pointer(skb, sizeof(eth_header_t));
    // TODO - AIO
    inet_recv_t recv = NULL;
    if (recv == NULL)
        return -1;
    return recv(skb);
}


