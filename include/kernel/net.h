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
#ifndef _KORA_NET_H
#define _KORA_NET_H 1

#include <bits/atomic.h>
#include <kora/llist.h>
#include <kora/splock.h>
#include <kora/bbtree.h>
#include <kora/time.h>
#include <stdint.h>
#include <string.h>
#include <sys/sem.h>
#include <bits/cdefs.h>

#define IOVLEN_MAX 64

 // IO vector
typedef struct iovec
{
    char *buf;
    size_t len;
} iovec_t;


typedef struct netstack netstack_t;
typedef struct net_ops net_ops_t;
typedef struct ifnet ifnet_t;
typedef struct skb skb_t;
typedef struct socket socket_t;
typedef struct nproto nproto_t;
typedef struct netmsg netmsg_t;
typedef struct nhandler nhandler_t;
typedef struct net_qry net_qry_t;

typedef int (*net_recv_t)(skb_t *);

#define NET_AF_EVT 0
#define NET_AF_LO 1
#define NET_AF_ETH 2
#define NET_AF_IP4 3
#define NET_AF_IP6 4
#define NET_AF_TCP 5
#define NET_AF_UDP 6

#define NET_MAX_HWADRLEN 16
#define NET_MAX_LOG 64
#define NET_MAX_RESPONSE 128

// Network stack
struct netstack {
    int running;
    char *hostname;
    char *domain;
    llhead_t list;
    splock_t lock;

    sem_t rx_sem;
    llhead_t rx_list;
    splock_t rx_lock;

    atomic_int id_max;

    bbtree_t protocols;
    llhead_t handlers;
};

// Network device operations
struct net_ops {
    void (*link)(ifnet_t *);
    int (*send)(ifnet_t *, skb_t *);
    int(*close)(ifnet_t *);
};

// Network interface
struct ifnet {
    int idx;
    unsigned mtu;
    splock_t lock;
    netstack_t *stack;
    uint8_t hwaddr[NET_MAX_HWADRLEN];
    llnode_t node;
    int protocol;
    nproto_t* proto;
    int flags;

    // Drivers operations
    void *drv_data;
    net_ops_t *ops;

    // Statistics
    long rx_packets;
    long rx_bytes;
    long rx_broadcast;
    // long rx_errors;
    long rx_dropped;
    long tx_packets;
    long tx_bytes;
    long tx_broadcast;
    long tx_errors;
    long tx_dropped;
};

// Socket kernel buffer
struct skb {
    int err;
    int protocol;
    unsigned pen;
    unsigned length;
    unsigned size;
    ifnet_t *ifnet;
    llnode_t node;
    char log[NET_MAX_LOG];
    uint8_t addr[NET_MAX_HWADRLEN * 2];
    unsigned addrlen;
    void *data;
    uint8_t buf[0];
};

// Network protocol
struct nproto {
    // TODO -- If we use a generic struct(void*, bbnode), no need to alloc proto!
    bbnode_t bnode;
    const char *name;
    unsigned addrlen;
    int (*socket)(socket_t*, int);
    int (*receive)(skb_t*);
    int (*accept)(socket_t *, socket_t *, skb_t *);
    int(*bind)(socket_t *, const uint8_t *, size_t);
    int(*connect)(socket_t *, const uint8_t *, size_t);
    long (*send)(socket_t*, const uint8_t*, const char*, size_t, int);
    long (*recv)(socket_t*, uint8_t*, char*, size_t, int);
    int(*close)(socket_t*);
    char *(*paddr)(const uint8_t*, char *, int);
    int (*clear)(netstack_t *, ifnet_t *);
    int (*teardown)(netstack_t *);
    void* data;
};

// Socket
struct socket {
    netstack_t *stack;
    nproto_t * proto;
    int protocol;
    uint8_t laddr[NET_MAX_HWADRLEN];
    uint8_t raddr[NET_MAX_HWADRLEN];
    mtx_t lock;
    splock_t rlock;
    sem_t rsem;
    int flags;
    llhead_t lskb;
};

// Device event handler
struct nhandler {
    void(*handler)(ifnet_t*, int, int);
    llnode_t node;
};

// Network message
struct netmsg {
    unsigned addrlen;
    uint8_t addr[NET_MAX_HWADRLEN];
    unsigned iolven;
    iovec_t iov[0];
};

// Generic network query holder
struct net_qry
{
    cnd_t cnd;
    mtx_t mtx;
    bool received;
    bool success;
    xtime_t start;
    xtime_t elapsed;
    llnode_t lnode;
    bbnode_t bnode;
    size_t len;
    uint8_t res[NET_MAX_RESPONSE];
};


// List of flags for ifnet, skb or socket
enum {
    NET_CONNECTED = (1 << 0),

    // Tell a packet buffer it's full
    NET_OVERFILL = (1 << 2),

    // Tell of a socket we specify a local address
    NET_ADDR_BINDED = (1 << 16),
    // Tell of a socket we specify a remote address
    NET_ADDR_CONNECTED = (1 << 17),
    // Tell the socket expect incomming connection
    NET_SOCK_LISTING = (1 << 18),
};

// List of defined device event
enum {
    NET_EV_LINK = 1,
    NET_EV_UNLINK,
    NET_EV_MAX,
};

#define net_log(s,m)  strncat((s)->log,(m),NET_MAX_LOG - strlen((s)->log))

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Network stack, devices & protocols

/* Push an event on an interface device (connected/disconnected) */
int net_event(ifnet_t* net, int event, int param);
/* Register an handler to listen to network interface events */
void net_handler(netstack_t* stack, void(*handler)(ifnet_t*, int, int));
/* Unregister an handler from the network interface events */
void net_unregister(netstack_t *stack, void(*handler)(ifnet_t *, int, int));
/* Search of a protocol registered on the network stack */
nproto_t* net_protocol(netstack_t* stack, int protocol);
/* Register a new protocol on the stack */
void net_set_protocol(netstack_t* stack, int protocol, nproto_t* proto);
/* Unregister a protocol oF the stack */
void net_rm_protocol(netstack_t *stack, int protocol);
/* Search for an network interface */
ifnet_t* net_interface(netstack_t* stack, int protocol, int idx);
/* Register a new network interface */
ifnet_t* net_device(netstack_t* stack, int protocol, uint8_t* hwaddr, net_ops_t* ops, void* driver);
/* Create the network stack (non static only for testing) */
netstack_t* net_create_stack();
/* Deamon thread to watch over incoming package and other
   device events (non static only for testing) */
void net_deamon(netstack_t* stack);
/* Accessor for the kernel network stack */
netstack_t* net_stack();
/* Setup of the kernel network stack */
void net_setup();

int net_destroy_stack(netstack_t *stack);

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Socket kernel buffer

/* Create a new tx packet */
skb_t *net_packet(ifnet_t *net);
/* Create a new rx packet and push it into received queue */
void net_skb_recv(ifnet_t *net, uint8_t *buf, unsigned len);
/* Send a tx packet to the network card */
int net_skb_send(skb_t *skb);
/* Trash a faulty tx packet */
int net_skb_trash(skb_t *skb);
/* Read data from a packet */
int net_skb_read(skb_t *skb, void *buf, unsigned len);
/* Write data on a packet */
int net_skb_write(skb_t *skb, const void *buf, unsigned len);
/* Get pointer on data from a packet and move cursor */
void *net_skb_reserve(skb_t *skb, unsigned len);

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Socket handling

/* Create an endpoint for communication */
socket_t* net_socket(netstack_t* stack, int protocol, int method);
/* Close a socket endpoint */
int net_socket_close(socket_t* sock);
/* Bind a name to a socket - set its local address */
int net_socket_bind(socket_t* sock, uint8_t* addr, size_t len);
/* Connect a socket with a single address - set its remote address */
int net_socket_connect(socket_t* sock, uint8_t* addr, size_t len);
/* Sending a series of data to an address */
long net_socket_send(socket_t* sock, const netmsg_t* msg, int flags);
/* Receiving a series of data from an address */
long net_socket_recv(socket_t* sock, netmsg_t* msg, int flags);
/* Writing data to a socket */
long net_socket_write(socket_t* sock, const char* buf, size_t len, int flags);
/* Reading data from a socket */
long net_socket_read(socket_t* sock, char* buf, size_t len, int flags);
/* Turn the socket into listening mode and look for a client socket */
socket_t* net_socket_accept(socket_t* sock, xtime_t timeout);
/* Wait for an incoming packet on this socket */
skb_t* net_socket_pull(socket_t* sock, uint8_t* addr, int length, xtime_t timeout);
/* Signal an incoming packet on this socket */
int net_socket_push(socket_t* sock, skb_t* skb);

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Loopback device and protocol

/* Registers a new protocol capable of using loopback */
int lo_handshake(netstack_t *stack, uint16_t protocol, net_recv_t recv);
/* Unregisters a protocol capable of using loopback */
int lo_unregister(netstack_t *stack, uint16_t protocol, net_recv_t recv);
/* Writes a loopback header on a tx packet */
int lo_header(skb_t *skb, uint16_t protocol, uint16_t port);
/* Entry point of the loopback module */
int lo_setup(netstack_t *stack);

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#define __swap16(s) ((uint16_t)((((s) & 0xFF00) >> 8) | (((s) & 0xFF) << 8)))
#define __swap32(l) ((uint32_t)((((l) & 0xFF000000) >> 24) | (((l) & 0xFF0000) >> 8) | (((l) & 0xFF00) << 8) | (((l) & 0xFF) << 24)))

#if __BYTE_ORDER == __LITTLE_ENDIAN
# define htons(s) __swap16(s)
# define ntohs(s) __swap16(s)
# define htonl(l) __swap32(l)
# define ntohl(l) __swap32(l)
#else
# define htons(s) (s)
# define ntohs(s) (s)
# define htonl(l) (l)
# define ntohl(l) (l)
#endif

#endif /* _KORA_NET_H */
