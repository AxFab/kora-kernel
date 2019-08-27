#ifndef __KERNEL_RESX__NET_H
#define __KERNEL_RESX__NET_H 1

#include <stddef.h>


char *net_ethstr(char *buf, uint8_t *mac);
char *net_ip4str(char *buf, uint8_t *ip);
int net_device(netdev_t *ifnet);
int net_read(skb_t *skb, void *buf, unsigned len);
int net_send(skb_t *skb);
int net_sock_recv(socket_t *socket, skb_t *skb);
int net_socket_read(socket_t *socket, char *buf, int len);
int net_socket_write(socket_t *socket, const char *buf, int len);
int net_trash(skb_t *skb);
int net_write(skb_t *skb, const void *buf, unsigned len);
skb_t *net_packet(netdev_t *ifnet);
socket_t *net_socket(int protocol, char *buf);
uint16_t net_ephemeral_port(socket_t *socket);
uint16_t net_rand_port();
void *net_pointer(skb_t *skb, unsigned len);
void net_recv(netdev_t *ifnet, protocol_t *protocol, uint8_t *buf, unsigned len);
void net_service();


#endif  /* __KERNEL_RESX__NET_H */
