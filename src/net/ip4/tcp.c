#include "ip4.h"

typedef struct tcp_header tcp_header_t;

PACK(struct tcp_header {
    uint16_t src_port;
    uint16_t dest_port;
    uint32_t seq_no;
    uint32_t ack_no;
    uint16_t flags;
    uint16_t window;
    uint16_t checksum;
    uint16_t urgent;
});


int tcp_header(skb_t* skb, ip4_route_t* route, int length, int port, int src, uint16_t identifier, uint16_t offset)
{
    if (ip4_header(skb, route, IP4_TCP, length + sizeof(tcp_header_t), identifier, offset) != 0)
        return -1;

    net_skb_log(skb, ",tcp");
    tcp_header_t* header = net_skb_reserve(skb, sizeof(tcp_header_t));
    if (header == NULL) {
        net_skb_log(skb, ":Unexpected end of data");
        return -1;
    }

    memset(header, 0, sizeof(tcp_header_t));
    header->src_port = htonw(src);
    header->dest_port = htonw(port);
    //header->seq_no = seq;
    //header->ack_no = ack;
    //header->flags = flags;
    //header->window = window;
    //header->urgent = urgent;
    header->checksum = 0;
    header->checksum = ip4_checksum((uint16_t*)skb, sizeof(tcp_header_t) + 8);
    // TODO - ip options
    return -1;
}


int tcp_receive(skb_t* skb, int length, uint16_t identifier, uint16_t offset)
{
    net_skb_log(skb, ",tcp");
    tcp_header_t* header = net_skb_reserve(skb, sizeof(tcp_header_t));
    if (header == NULL) {
        net_skb_log(skb, ":Unexpected end of data");
        return -1;
    }

    // TODO - ip options
    length -= sizeof(tcp_header_t);

    uint16_t port = htonw(header->dest_port);
    socket_t* socket = ip4_lookfor_socket(skb->ifnet, port, true, NULL);
    if (socket == NULL)
        return -1;
    return net_socket_recv(socket, skb, length);
}


int tcp_socket(socket_t* socket)
{
    return -1;
}







