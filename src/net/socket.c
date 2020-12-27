#include <kernel/net.h>
#include <kernel/stdc.h>

int net_socket_recv(socket_t *socket, skb_t *skb, int length)
{
    return -1;
}

socket_t* net_socket(int protocol)
{
    socket_t* socket = kalloc(sizeof(socket_t));
    socket->protocol = protocol;
    net_sock_t proto = NULL;
    if (proto == NULL || proto(socket) != 0) {
        kfree(socket);
        return NULL;
    }

    // TODO - if (protocol == NET_AF_TCP) 
    //    ret = tcp_socket(socket);
    //else if (protocol == NET_AF_UDP)
    //    ret = udp_socket(socket);

    return socket;
}

