#include <kernel/net.h>
#include <kernel/files.h>
#include <kora/bbtree.h>
#include <string.h>



int net_sock_recv(socket_t *socket, skb_t *skb)
{
    if (skb == NULL) {
        // Look for the next packet
        skb = bbtree_search_eq(&socket->packets, socket->next_frag, skb_t, bnode);
        if (skb == NULL)
            return 0;
    } else if (skb->fragment != socket->next_frag) {
        // Save this new packet
        skb->bnode.value_ = skb->fragment;
        bbtree_insert(&socket->packets, &skb->bnode);
        return 0;
    }


    for (;;) {
        char *buf = (char *)&skb->buf[skb->pen];
        int sz = pipe_write(socket->input, buf, skb->data_len, IO_ATOMIC | IO_NO_BLOCK);
        if (sz != skb->data_len) {
            // Save packet to retry later
            bbtree_insert(&socket->packets, &skb->bnode);
            return 0;
        }
        bbtree_remove(&socket->packets, skb->bnode.value_);
        free(skb);
        socket->next_frag++;

        // Look for the next packet
        skb = bbtree_search_eq(&socket->packets, socket->next_frag, skb_t, bnode);
        if (skb == NULL)
            return 0;
    }
}



int net_socket_read(socket_t *socket, char *buf, int len)
{
    int bytes = 0;
    while (len) {
        net_sock_recv(socket, NULL);
        int sz = pipe_read(socket->input, buf, len, IO_ATOMIC);
        if (sz <= 0)
            return -1;
        buf += sz;
        len -= sz;
        bytes += sz;
    }
    return bytes;
}

int net_socket_write(socket_t *socket, const char *buf, int len)
{
    int bytes = 0;
    int max = socket->protocol->max_frame(socket);
    while (len > 0) {
        int sz = socket->protocol->packet(socket, buf, MIN(len, max));
        if (sz <= 0)
            return -1;
        buf += sz;
        len -= sz;
        bytes += sz;
    }
    return bytes;
}

extern protocol_t *local_protocol;

socket_t *net_socket(int protocol, char *buf)
{
    socket_t *socket = kalloc(sizeof(socket_t));
    socket->input = pipe_create();

    // Depends of protocol
    socket->protocol = local_protocol;
    bbtree_init(&socket->packets);

    if (socket->protocol == NULL || socket->protocol->connect(socket, buf) != 0) {
        kfree(socket);
        return NULL;
    }
    return socket;
}

