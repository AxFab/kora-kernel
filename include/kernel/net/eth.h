#ifndef __KERNEL_NET_ETH_H
#define __KERNEL_NET_ETH_H 1

#include <kernel/net.h>

#define ETH_ALEN  6

typedef struct eth_header eth_header_t;

struct eth_header {
    uint8_t target[ETH_ALEN];
    uint8_t sender[ETH_ALEN];
    uint16_t type;
};

int eth_receive(skb_t *skb);
skb_t *eth_packet(ifnet_t *ifnet, const uint8_t *addr, uint16_t type);

#endif  /* __KERNEL_NET_ETH_H */
