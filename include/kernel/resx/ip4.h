#ifndef __KERNEL_RESX_IP4_H
#define __KERNEL_RESX_IP4_H 1

#include <stddef.h>

int eth_header(skb_t *skb, const uint8_t *target, uint16_t type);
int eth_receive(skb_t *skb);

int arp_query(netdev_t *ifnet, const uint8_t *ip);
int arp_receive(skb_t *skb);
int dhcp_discovery(netdev_t *ifnet);
int dhcp_receive(skb_t *skb, unsigned length);
int dns_receive(skb_t *skb, int length);
int host_mac_for_ip(uint8_t *mac, const uint8_t *ip, int trust);
int icmp_ping(netdev_t *ifnet, const uint8_t *ip);
int icmp_receive(skb_t *skb, unsigned len);
int ip4_header(skb_t *skb, const uint8_t *ip_addr, int identifier, int offset, int length, int protocol);
int ip4_receive(skb_t *skb);
int ntp_receive(skb_t *skb, int length);
socket_t *tcp_socket(netdev_t *ifnet, uint8_t *ip, int port);
int tcp_header(skb_t *skb, const uint8_t *ip, int length, int port, int src);
int tcp_packet(socket_t *socket);
int tcp_receive(skb_t *skb, unsigned length);
socket_t *udp_socket(netdev_t *ifnet, uint8_t *ip, int port);
int udp_header(skb_t *skb, const uint8_t *ip, int length, int port, int src);
int udp_packet(socket_t *socket);
int udp_receive(skb_t *skb, unsigned length);
dhcp_server_t *dhcp_create_server(uint8_t *ip_rooter, int netsz);
uint16_t ip4_checksum(skb_t *skb, size_t len);
void host_init();
void host_register(uint8_t *mac, uint8_t *ip, const char *hostname, const char *domain, int trust);


#endif  /* __KERNEL_RESX_IP4_H */
