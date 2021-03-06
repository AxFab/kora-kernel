#include "ip4.h"
#include <threads.h>
#include <kernel/core.h>

typedef struct icmp_header icmp_header_t;


PACK(struct icmp_header {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint32_t data;
});

struct icmp_info {
    hmap_t pings;
    splock_t lock;
};

struct icmp_ping {
    mtx_t mtx;
    cnd_t cnd;
    uint32_t data;
    uint16_t id;
    time_t time;
    char* buf;
    int len;
    bool received;
};

#define ICMP_PONG 0
#define ICMP_PING 8

icmp_info_t* icmp_readinfo(ifnet_t* net)
{
    splock_lock(&net->lock);
    icmp_info_t* info = net->icmp;
    if (info == NULL) {
        info = kalloc(sizeof(icmp_info_t));
        hmp_init(&info->pings, 8);
        net->icmp = info;
    }
    splock_unlock(&net->lock);
    return info;
}

int icmp_packet(ifnet_t *net, ip4_route_t *route, int type, int code, uint32_t data, const char *buf, int len)
{
    skb_t* skb = net_packet(net);
    if (unlikely(skb == NULL))
        return -1;
    if (ip4_header(skb, route, IP4_ICMP, len + sizeof(icmp_header_t), 0, 0) != 0)
        return net_skb_trash(skb);

    net_skb_log(skb, ",icmp");
    icmp_header_t* header = net_skb_reserve(skb, sizeof(icmp_header_t));
    if (header == NULL) {
        net_skb_log(skb, ":Unexpected end of data");
        return -1;
    }

    header->type = type;
    header->code = code;
    header->data = data;
    header->checksum = 0;
    header->checksum = ip4_checksum(header, sizeof(icmp_header_t));
    if (len > 0)
        net_skb_write(skb, buf, len);
    return net_skb_send(skb);
}


icmp_ping_t *icmp_ping(ip4_route_t *route, const char* buf, int len)
{
    uint16_t id = rand16();
    short seq = 1;
    uint32_t data = id | (seq << 16);
    icmp_ping_t *ping = kalloc(sizeof(icmp_ping_t));
    if (len > 0) {
        ping->len = len;
        ping->buf = kalloc(len);
        memcpy(ping->buf, buf, len);
    }
    ping->id = id;
    ping->data = data;
    ping->time = time(NULL);
    ping->received = false;
    mtx_init(&ping->mtx, mtx_plain);
    cnd_init(&ping->cnd);
    icmp_info_t* info = icmp_readinfo(route->net);
    splock_lock(&info->lock);
    hmp_put(&info->pings, &ping->id, 2, ping);
    splock_unlock(&info->lock);
    int ret = icmp_packet(route->net, route, ICMP_PING, 0, data, buf, len);
    if (ret == 0)
        return ping;

    splock_lock(&info->lock);
    hmp_remove(&info->pings, &id, 2);
    splock_unlock(&info->lock);
    kfree(ping->buf);
    kfree(ping);
    return NULL;
}

int icmp_receive(skb_t* skb, int length)
{
    net_skb_log(skb, ",icmp");
    icmp_header_t* header = net_skb_reserve(skb, sizeof(icmp_header_t));
    if (header == NULL) {
        net_skb_log(skb, ":Unexpected end of data");
        return -1;
    }

    length -= sizeof(icmp_header_t);
    char* buf = NULL;
    if (length > 0) 
        buf = net_skb_reserve(skb, length);
    
    if (header->type == ICMP_PING) {
        // TODO - Find who to send it to ?
        // icmp_packet(skb->ifnet, NULL, ICMP_PONG, 0, header->data, buf, length);
    }
    else if (header->type == ICMP_PONG) {
        icmp_info_t* info = icmp_readinfo(skb->ifnet);
        uint16_t id = header->data & 0xFFFF;
        icmp_ping_t* ping = hmp_get(&info->pings, &id, 2);
        if (ping != NULL) {
            // TODO - Check the payload!
            mtx_lock(&ping->mtx);
            splock_lock(&info->lock);
            hmp_remove(&info->pings, &id, 2);
            splock_unlock(&info->lock);
            ping->received = true;
            cnd_signal(&ping->cnd);
            mtx_unlock(&ping->mtx);
        }
    }

    free(skb);
    return 0;
}

