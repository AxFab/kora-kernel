#include <kernel/net.h>
#include <string.h>

protocol_t *local_protocol;
netdev_t *local_network;
int (*__vtable_ip_receive)(skb_t*);


int lo_packet(socket_t *socket, const char *buf, size_t len)
{
    skb_t *skb = net_packet(socket->ifnet);
    int *p = net_pointer(skb, sizeof(int) * 4);
    p[0] = 1;
    p[1] = len;
    p[2] = rand();
    p[3] = socket->send_frag++;
    memcpy(net_pointer(skb, len), buf, len);
    return net_send(skb);
}

socket_t *__get_socket = NULL;

int lo_receive(skb_t *skb)
{
    int *p = net_pointer(skb, sizeof(int)*4);
    socket_t *socket = NULL;
    switch(p[0]) {
    case 1:
        socket = __get_socket;
        skb->data_len = p[1];
        skb->fragment = p[3];
        return net_sock_recv(socket, skb);
    case 2:
        if (__vtable_ip_receive == NULL)
            return -1;
        return __vtable_ip_receive(skb);
    default:
        return -1;
    }
}

int lo_max_frame(socket_t *socket)
{
    return socket->ifnet->mtu - sizeof(int) * 4;
}

int lo_connect(socket_t *socket, char *buf)
{
    __get_socket = socket;
    socket->ifnet = local_network;
    return 0;
}

int lo_dev_link(ifnet_t *ifnet)
{
    ifnet->flags |= NET_CONNECTED;
    return 0;
}

int lo_dev_send(ifnet_t *ifnet, skb_t *skb)
{
    net_recv(ifnet, local_protocol, skb->buf, skb->length);
    return 0;
}



void lo_setup()
{
    local_protocol = kalloc(sizeof(protocol_t));
    local_protocol->packet = lo_packet;
    local_protocol->receive = lo_receive;
    local_protocol->connect = lo_connect;
    local_protocol->max_frame = lo_max_frame;

    local_network = kalloc(sizeof(ifnet_t));
    local_network->mtu = 1500;
    local_network->link = lo_dev_link;
    local_network->send = lo_dev_send;
}


