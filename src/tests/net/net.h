/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2021  <Fabien Bavent>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   - - - - - - - - - - - - - - -
 */
#ifndef __KERNEL_NET_H
#define __KERNEL_NET_H 1

#include "utils.h"
#include "threads.h"

typedef struct inet inet_t;
typedef struct ifnet ifnet_t;
typedef struct skb skb_t;
typedef struct socket socket_t;

typedef int (*inet_recv_t)(skb_t *);

struct inet {
    mtx_t lock;
    cnd_t pulse;
    llhead_t rx_queue;

    llhead_t ip4_cnxs;
};

extern __thread inet_t *__net;
#define kNET (*__net)

struct ifnet {
    int type;
    int flags;
    int mtu;
    int (*link)(ifnet_t *ifnet);
    int (*send)(ifnet_t *ifnet, skb_t *skb);
    uint8_t hwaddr[16];
    void *data;
};

struct skb {
    int size;
    int len;
    ifnet_t *ifnet;
    char *pen;
    llnode_t node;
    char log[16];
    char buf[0];
};

struct socket {
    void *route;
    int (*connect)(socket_t *socket, uint8_t *addr);
    skb_t *(*packet)(socket_t *socket);
};

void net_recv(ifnet_t *ifnet, const void *buf, size_t len);

void net_send(skb_t *skb);

void net_rxloop();

skb_t *net_packet(ifnet_t *ifnet);

void *net_pointer(skb_t *skb, size_t len);

#define INET_ETH  1
#define INET_LO  2


#define __swap16(w) ((uint16_t)((((w) & 0xFF00) >> 8) | (((w) & 0xFF) << 8)))
#define __swap32(l) ((uint32_t)((((l) & 0xFF000000) >> 24) | (((l) & 0xFF0000) >> 8) | (((l) & 0xFF00) << 8) | (((l) & 0xFF) << 24)))

#define htonw(w) __swap16(w)
#define ntohw(w) __swap16(w)

#define htonl(l) __swap32(l)
#define ntohl(w) __swap32(l)



#endif  /* __KERNEL_NET_H */
