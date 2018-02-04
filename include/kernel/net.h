/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015  <Fabien Bavent>
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
#ifndef _KERNEL_NET_H
#define _KERNEL_NET_H 1

#include <kernel/core.h>
#include <kernel/memory.h>

#define NET_ADDR_MAX_LG 16
#define DEV_NAME_SZ 32

typedef struct net_device net_device_t;
typedef struct net_ops net_ops_t;
typedef struct net_layer_info net_layer_info_t;
typedef struct socket socket_t;
typedef struct sk_buffer sk_buffer_t;

enum {
    NET_PROTO_ETHERNET,
    NET_PROTO_IPv4,
    NET_PROTO_IPv6,
    NET_PROTO_COUNT
};

enum {
    NET_ST_INIT,
    NET_ST_READY
};

struct net_device {
    // struct hmap_node
    char name[DEV_NAME_SZ];

    // struct net_device_stats stats
    int state; // DISCONNECTED / CONNECTED / NO INTERNET / CONNECTING / ERROR ...
    int features; // JUMBO / CHIDENTITY / UP / BROADCAST / MULTICAST / ...
    int index; // 0 ->eth0, 1 -> eth1 | -1 => lo ...

    int scope; // Link / Host

    net_ops_t *ops; // Driver methods
    // llhead_t tx_queue; // Sending queue
    // llhead_t rx_queue; // Received queue

    struct net_layer_info *protocoles[NET_PROTO_COUNT]; // List of protocole data.
    unsigned char hw_address[NET_ADDR_MAX_LG]; // MAC address
    // unsigned char addr_type; // hardware address type
    unsigned char addr_length; // hardware address length

    // bool dismantle; // Device is going to be freed.
};

struct net_ops {
    int (*send)(net_device_t *ndev, sk_buffer_t *skb);
};

struct net_layer_info {
    unsigned char address[NET_ADDR_MAX_LG]; // address
    unsigned char addr_length; // hardware address length
    unsigned char broadcast[NET_ADDR_MAX_LG]; // Broadcast address
    unsigned char underlying;
    unsigned char status;
};


struct socket {
    int no;
    // llhead_t tx_queue; // Sending queue
    // llhead_t rx_queue; // Received queue
};

struct sk_buffer {
    // llhode_t qnode; // To push on device or socket queue
    page_t page;
    char *address;
    int length;
    int seq_no;
    socket_t *socket;
    int flags;
};

struct net_stat {
    atomic_t rx_packets;
    atomic_t rx_bytes;
    atomic_t rx_broadcast;
    atomic_t rx_errors;
    atomic_t tx_packets;
    atomic_t tx_bytes;
    atomic_t tx_broadcast;
    atomic_t tx_errors;
    atomic_t tx_dropped;
};

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

net_device_t *net_register(const char *name, net_ops_t *ops,
                           const unsigned char *address, int length);
void net_receive(net_device_t *dev, const unsigned char *buffer, int length);

#endif /* _KERNEL_NET_H */
