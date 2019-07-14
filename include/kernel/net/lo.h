#ifndef __KERNEL_NET_LO_H
#define __KERNEL_NET_LO_H 1

#include <kernel/net.h>


int lo_receive(skb_t *skb);
skb_t *lo_packet(ifnet_t *ifnet, uint16_t type);

#endif  /* __KERNEL_NET_LO_H */
