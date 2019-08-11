#include "net.h"


skb_t *lo_packet(ifnet_t *ifnet, uint16_t type)
{
    skb_t *skb = net_packet(ifnet);
    uint16_t *ptr = net_pointer(skb, sizeof(uint16_t));
    *ptr = type;
    strcat(skb->log, ".lo");
    return skb;
}

int lo_receive(skb_t *skb)
{
    uint16_t *ptr = net_pointer(skb, sizeof(uint16_t));
    // TODO - Firewall
    inet_recv_t recv = NULL;
    if (recv == NULL)
        return -1;
    return recv(skb);
}


