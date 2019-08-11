#include "lnet.h"
#include "net.h"
#include "ip4.h"
#include <stdio.h>

typedef struct lnet lnet_t;
typedef struct lendpoint lendpoint_t;

struct lnet {
    int len;
    int idx;
    lendpoint_t *slots[0];
};

struct lendpoint {
    int idx;
    lnet_t *lan;
    ifnet_t *ifnet;
    inet_t *net;
};


void eth_stringfy(char *buf, uint8_t *addr)
{
    snprintf(buf, 30, "%02x:%02x:%02x:%02x:%02x:%02x",
             addr[0], addr[1], addr[2],
             addr[3], addr[4], addr[5]);
}

int lnet_send(ifnet_t *ifnet, skb_t *skb)
{
    lendpoint_t *source = (void *)ifnet->data;
    lnet_t *lan = source->lan;

    eth_header_t *head = (void *)skb->buf;
    int idx = head->target[5] % lan->len;
    lendpoint_t *target = lan->slots[idx];
    if (target == NULL || memcmp(head->target, target->ifnet->hwaddr, ETH_ALEN) != 0)
        return 0;
    __net = target->net;
    char mac1[30];
    char mac2[30];
    eth_stringfy(mac1, source->ifnet->hwaddr);
    eth_stringfy(mac2, target->ifnet->hwaddr);
    printf("Packet send from <%s> to <%s> :: %s \n", mac1, mac2, skb->log);
    net_recv(target->ifnet, skb->buf, skb->len);
    return 0;
}

lnet_t *lnet_switch(int count)
{
    lnet_t *lan = kalloc(sizeof(lnet_t) + sizeof(void *) * count);
    lan->len = count;
    return lan;
}

ifnet_t *lnet_endpoint(lnet_t *lan, inet_t *net)
{
    if (lan->idx >= lan->len)
        return NULL;
    ifnet_t *ifnet = kalloc(sizeof(ifnet_t));
    lendpoint_t *cnx = kalloc(sizeof(lendpoint_t));
    cnx->idx = lan->idx++;
    cnx->ifnet = ifnet;
    cnx->lan = lan;
    cnx->net = net;
    ifnet->hwaddr[0] = 0x80;
    ifnet->hwaddr[1] = 0xa7;
    ifnet->hwaddr[2] = rand() & 0xff;
    ifnet->hwaddr[3] = rand() & 0xff;
    ifnet->hwaddr[4] = rand() & 0xff;
    ifnet->hwaddr[5] = cnx->idx;
    lan->slots[cnx->idx] = cnx;
    ifnet->send = lnet_send;
    ifnet->mtu = 1500;
    ifnet->data = cnx;
    return ifnet;
}

// -=-=-=-=-=-=-=-=-=-=-

int main()
{
    uint8_t ad0[] = { 192, 168, 10, 1 };
    uint8_t ad1[] = { 192, 168, 10, 6 };
    uint8_t adm[] = { 192, 168, 10, 10 };
    uint8_t adn[] = { 192, 168, 10, 20 };
    uint8_t mask[] = { 255, 255, 252, 0 };
    uint8_t zr[] = { 0, 0, 0, 0 };

    inet_t in[3];
    int i;
    for (i = 0; i < 3; ++i) {
        mtx_init(&in[i].lock, mtx_plain);
        cnd_init(&in[i].pulse);
        llist_init(&in[i].rx_queue);
        llist_init(&in[i].ip4_cnxs);
    }
    lnet_t *lan = lnet_switch(8);
    ifnet_t *pc1 = lnet_endpoint(lan, &in[0]);
    ifnet_t *pc2 = lnet_endpoint(lan, &in[1]);
    ifnet_t *pc3 = lnet_endpoint(lan, &in[2]);

    skb_t *skb = eth_packet(pc1, pc2->hwaddr, 0);
    net_send(skb);


    __net = &in[0];
    ip4_config(pc1, ad0, mask, zr);

    __net = &in[1];
    ip4_config(pc2, ad1, mask, zr);
    // dhcp_server(pc1, adm, adn);
    socket_t sock;

    __net = &in[2];
    int ret = ip4_connect(&sock, ad1);
    assert(ret != 0); // We are not connected

    return 0;
}


