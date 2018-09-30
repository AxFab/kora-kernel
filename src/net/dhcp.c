/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2018  <Fabien Bavent>
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
#include <kernel/net.h>
#include <kora/hmap.h>
#include <string.h>

#define BOOT_REQUEST 1
#define BOOT_REPLY 2

#define DHCP_MAGIC  htonl(0x63825363)

#define DHCP_DISCOVER 1
#define DHCP_OFFER 2
#define DHCP_REQUEST 3
#define DHCP_DECLINE 4
#define DHCP_PACK 5
#define DHCP_PNACK 6
#define DHCP_RELEASE 7
#define DHCP_INFORM 8

#define DHCP_OPT_SUBNETMASK 1
#define DHCP_OPT_ROOTER 3
#define DHCP_OPT_DNSIP 6
#define DHCP_OPT_HOSTNAME 12
#define DHCP_OPT_DOMAIN 15
#define DHCP_OPT_BROADCAST 28
#define DHCP_OPT_QRYIP 50
#define DHCP_OPT_LEASETIME 51
#define DHCP_OPT_MSGTYPE 53
#define DHCP_OPT_SERVERIP 54
#define DHCP_OPT_QRYLIST 55
#define DHCP_OPT_RENEWALTIME 58
#define DHCP_OPT_REBINDINGTIME 59
#define DHCP_OPT_VENDOR 60
#define DHCP_OPT_CLIENTMAC 61


#define DHCP_LEASE_IP  (1 << 0)
#define DHCP_LEASE_HOSTNAME  (1 << 1)
#define DHCP_LEASE_FREE  (1 << 8)

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

typedef struct dhcp_info dhcp_info_t;
typedef struct dhcp_lease dhcp_lease_t;

typedef struct DHCP_header DHCP_header_t;

PACK(struct DHCP_header {
    uint8_t opcode;
    uint8_t htype;
    uint8_t hlen;
    uint8_t hops;
    uint32_t xid;
    uint16_t secs;
    uint16_t flags;
    uint8_t ciaddr[4];
    uint8_t yiaddr[4];
    uint8_t siaddr[4];
    uint8_t giaddr[4];
    uint8_t chaddr[16];
    char sname[64];
    char file[128];
});


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

struct dhcp_info {
    uint32_t uid;
    uint8_t msg_type;
    char *hostname;
    char *domain;
    char *vendor;
    uint8_t rooter_ip[IP4_ALEN];
    uint8_t client_ip[IP4_ALEN];
    uint8_t dns_ip[IP4_ALEN];
    uint8_t broadcast_ip[IP4_ALEN];
    uint8_t submask_ip[IP4_ALEN];
    uint8_t dhcp_ip[IP4_ALEN];
    uint8_t query_ip[IP4_ALEN];
    uint32_t lease_time;
    uint32_t renewal_time;
    uint32_t rebinding_time;
    uint8_t client_mac[ETH_ALEN];
};

struct dhcp_lease {
    uint8_t mac[ETH_ALEN];
    uint8_t ip[IP4_ALEN];
    uint32_t lease_timeout;
    int flags;
    splock_t lock;
    time64_t timeout;
    char *hostname;
};

struct dhcp_server {
    splock_t lock;
    HMP_map leases;
    dhcp_lease_t **ip_table;
    int ip_count;
    uint8_t ip_rooter[IP4_ALEN];
    uint8_t ip_network[IP4_ALEN];
    int netsz;
};

int dhcp_packet(netdev_t *ifnet, const uint8_t *ip, uint32_t uid, int mode,
                const dhcp_info_t *info, const dhcp_lease_t *lease);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

static void dhcp_readstr(skb_t *skb, uint8_t *options, char **buf)
{
    net_read(skb, &options[2], options[1]);
    options[2 + options[1]] = '\0';
    if (*buf)
        kfree(*buf);
    *buf = strdup((char *)&options[2]);
}

static int dhcp_parse(skb_t *skb, dhcp_info_t *info, int length)
{
    uint8_t options[32];
    while (length > 2) {
        net_read(skb, options, 2);
        length -= 2 + options[1];
        if (options[1] >= 30)
            return -1;
        switch (options[0]) {
        case DHCP_OPT_SUBNETMASK:
            if (options[1] != 4)
                return -1;
            net_read(skb, info->submask_ip, IP4_ALEN);
            break;
        case DHCP_OPT_ROOTER:
            if (options[1] != 4)
                return -1;
            net_read(skb, info->rooter_ip, IP4_ALEN);
            break;
        case DHCP_OPT_DNSIP:
            if (options[1] != 4)
                return -1;
            net_read(skb, info->dns_ip, IP4_ALEN);
            break;
        case DHCP_OPT_HOSTNAME:
            dhcp_readstr(skb, options, &info->hostname);
            break;
        case DHCP_OPT_DOMAIN:
            dhcp_readstr(skb, options, &info->domain);
            break;
        case DHCP_OPT_BROADCAST:
            if (options[1] != 4)
                return -1;
            net_read(skb, info->broadcast_ip, IP4_ALEN);
            break;
        case DHCP_OPT_QRYIP:
            if (options[1] != 4)
                return -1;
            net_read(skb, info->query_ip, IP4_ALEN);
            break;
        case DHCP_OPT_LEASETIME:
            if (options[1] != sizeof(uint32_t))
                return -1;
            net_read(skb, &info->lease_time, sizeof(uint32_t));
            info->lease_time = htonl(info->lease_time);
            break;
        case DHCP_OPT_MSGTYPE:
            net_read(skb, &info->msg_type, 1); // SIZE
            break;
        case DHCP_OPT_SERVERIP:
            if (options[1] != 4)
                return -1;
            net_read(skb, info->dhcp_ip, IP4_ALEN);
            break;
        case DHCP_OPT_QRYLIST:
            net_read(skb, &options[2], options[1]);
            // TODO -- save on64bitq flags register, ignore above
            break;
        case DHCP_OPT_RENEWALTIME:
            if (options[1] != sizeof(uint32_t))
                return -1;
            net_read(skb, &info->renewal_time, sizeof(uint32_t));
            info->renewal_time = htonl(info->renewal_time);
            break;
        case DHCP_OPT_REBINDINGTIME:
            if (options[1] != sizeof(uint32_t))
                return -1;
            net_read(skb, &info->rebinding_time, sizeof(uint32_t));
            info->rebinding_time = htonl(info->rebinding_time);
            break;
        case DHCP_OPT_VENDOR:
            dhcp_readstr(skb, options, &info->vendor);
            break;
        case DHCP_OPT_CLIENTMAC:
            net_read(skb, &options[2], 1);
            if (options[1] != ETH_ALEN + 1 || options[2] != 1)
                return -1;
            net_read(skb, &info->client_mac, ETH_ALEN);
            break;
        case 0xFF:
            return 0;
        default:
            net_read(skb, &options[2], options[1]);
            break;
        }
    }
    return 0;
}

static skb_t *dhcp_header(netdev_t *ifnet, const uint8_t *ip, uint32_t uid,
                          int opcode, int options_len)
{
    skb_t *skb = net_packet(ifnet, 576);
    if (skb == NULL)
        return NULL;
    if (udp_header(skb, ip, sizeof(DHCP_header_t) + options_len, UDP_PORT_DHCP,
                   UDP_PORT_DHCP_S) != 0) {
        net_trash(skb);
        return NULL;
    }
    strncat(skb->log, "dhcp:", NET_LOG_SIZE);
    DHCP_header_t *header = net_pointer(skb, sizeof(DHCP_header_t));
    if (header == NULL) {
        net_trash(skb);
        return NULL;
    }
    memset(header, 0, sizeof(DHCP_header_t));
    header->opcode = opcode;
    header->htype = 1;
    header->hlen = 6;
    header->xid = uid;

    memcpy(header->ciaddr, ifnet->ip4_addr, IP4_ALEN);
    if (opcode == BOOT_REPLY)
        memcpy(header->yiaddr, ip, IP4_ALEN);
    if (ip != ip4_broadcast)
        memcpy(header->siaddr, opcode == BOOT_REQUEST ? ip : ifnet->ip4_addr, IP4_ALEN);
    // gateway
    memcpy(&header->chaddr, opcode == BOOT_REQUEST ? ifnet->eth_addr :
           skb->eth_addr, ETH_ALEN);

    // Magic
    uint32_t magic = DHCP_MAGIC;
    net_write(skb, &magic, 4);
    return skb;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

static int dhcp_count_options_request(netdev_t *ifnet, const uint8_t *ip,
                                      const dhcp_info_t *info)
{
    int opts = 0;
    if ((info != NULL && !ip4_null(info->query_ip)) || !ip4_null(ifnet->ip4_addr))
        opts += 6;
    if (ip != ip4_broadcast)
        opts += 6;
    if (info != NULL && info->hostname)
        opts += 2 + strlen(info->hostname);
    else if (ifnet->hostname) // TODO issue with this
        opts += 2 + strlen(ifnet->hostname);
    opts += 7;
    return opts;
}

static void dhcp_write_options_request(netdev_t *ifnet, skb_t *skb,
                                       uint8_t option[32], const uint8_t *ip, const dhcp_info_t *info)
{
    if (info != NULL && !ip4_null(info->query_ip)) {
        option[0] = DHCP_OPT_QRYIP;
        option[1] = IP4_ALEN;
        memcpy(&option[2], info->query_ip, IP4_ALEN);
        net_write(skb, option, option[1] + 2);
    } else if (!ip4_null(ifnet->ip4_addr)) {
        option[0] = DHCP_OPT_QRYIP;
        option[1] = IP4_ALEN;
        memcpy(&option[2], ifnet->ip4_addr, IP4_ALEN);
        net_write(skb, option, option[1] + 2);
    }

    if (ip != ip4_broadcast) {
        option[0] = DHCP_OPT_SERVERIP;
        option[1] = IP4_ALEN;
        memcpy(&option[2], ip, IP4_ALEN);
        net_write(skb, option, option[1] + 2);
    }

    if (info != NULL && info->hostname) {
        option[0] = DHCP_OPT_HOSTNAME;
        option[1] = (uint8_t)strlen(info->hostname);
        memcpy(&option[2], info->hostname, MIN(32, option[1]) - 2);
        net_write(skb, option, option[1] + 2);
    } else if (ifnet->hostname) { // TODO issue with this
        option[0] = DHCP_OPT_HOSTNAME;
        option[1] = (uint8_t)strlen(ifnet->hostname);
        memcpy(&option[2], ifnet->hostname, MIN(32, option[1]) - 2);
        net_write(skb, option, option[1] + 2);
    }

    option[0] = DHCP_OPT_QRYLIST;
    option[1] = 5;
    option[2] = DHCP_OPT_SUBNETMASK;
    option[3] = DHCP_OPT_DOMAIN;
    option[4] = DHCP_OPT_ROOTER;
    option[5] = DHCP_OPT_DNSIP;
    option[6] = DHCP_OPT_LEASETIME;
    net_write(skb, option, option[1] + 2);
}

static int dhcp_count_options_reply(netdev_t *ifnet, const uint8_t *ip,
                                    const dhcp_info_t *info, const dhcp_lease_t *lease)
{
    int opts = 3 * 6;
    if (lease != NULL)
        opts += 6;
    if (info->domain)
        opts += 2 + strlen(info->domain);
    return opts;
}

static void dhcp_write_options_reply(netdev_t *ifnet, skb_t *skb,
                                     uint8_t option[32], const uint8_t *ip, const dhcp_info_t *info,
                                     const dhcp_lease_t *lease)
{
    option[0] = DHCP_OPT_SERVERIP;
    option[1] = IP4_ALEN;
    memcpy(&option[2], ifnet->ip4_addr, IP4_ALEN);
    net_write(skb, option, option[1] + 2);

    option[0] = DHCP_OPT_SUBNETMASK;
    option[1] = IP4_ALEN;
    uint32_t submask = htonl(~((1 << ifnet->dhcp_srv->netsz) - 1));
    memcpy(&option[2], &submask, IP4_ALEN);
    net_write(skb, option, option[1] + 2);

    option[0] = DHCP_OPT_ROOTER;
    option[1] = IP4_ALEN;
    memcpy(&option[2], ifnet->dhcp_srv->ip_rooter, IP4_ALEN);
    net_write(skb, option, option[1] + 2);

    if (lease != NULL) {
        option[0] = DHCP_OPT_LEASETIME;
        uint32_t leasetime = htonl((lease->lease_timeout) / USEC_PER_SEC);
        option[1] = sizeof(uint32_t);
        memcpy(&option[2], &leasetime, sizeof(uint32_t));
        net_write(skb, option, option[1] + 2);
    }

    if (info->domain) {
        option[0] = DHCP_OPT_DOMAIN;
        option[1] = (uint8_t)strlen(info->domain);
        strncpy((char *)&option[2], info->domain, MIN(32, option[1]) - 2);
        net_write(skb, option, option[1] + 2);
    }
    // TODO -- DNS + REQUESTED
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

static int dhcp_on_offer(skb_t *skb, dhcp_info_t *info)
{
    netdev_t *ifnet = skb->ifnet;
    splock_lock(&ifnet->lock);
    /* Ignore unsolicited packets */
    if (ifnet->dhcp_transaction != info->uid || (ifnet->dhcp_mode != DHCP_DISCOVER
            && ifnet->dhcp_mode != DHCP_REQUEST)) {
        splock_unlock(&ifnet->lock);
        return -1;
    }

    /* Check proposal validity */
    bool valid = true;
    if (memcmp(skb->ip4_addr, info->dhcp_ip, IP4_ALEN) != 0)
        valid = false;

    if (!valid) {
        splock_unlock(&ifnet->lock);
        return -1;
    }

    host_register(skb->eth_addr, info->dhcp_ip, NULL, info->domain, HOST_TEMPORARY);
    // If not interested, decline ASAP
    /*
    if (ifnet->dhcp_mode == DHCP_REQUEST) {
        // Store as backup plan, refuse later!
        splock_unlock(ifnet->lock);
        return 0;
    }*/

    ifnet->dhcp_mode = DHCP_REQUEST;
    ifnet->dhcp_lastsend = time64();
    assert(ifnet->dhcp_transaction != 0);
    memcpy(info->query_ip, info->client_ip, IP4_ALEN);
    // STORE INFO AS IN PROPATION
    splock_unlock(&ifnet->lock);
    int ret = dhcp_packet(ifnet, info->dhcp_ip, info->uid, DHCP_REQUEST, info,
                          NULL);
    if (ret != 0) {
        splock_lock(&ifnet->lock);
        ifnet->dhcp_mode = DHCP_DISCOVER;
        splock_unlock(&ifnet->lock);
    }
    return 0;
}

static int dhcp_on_ack(skb_t *skb, dhcp_info_t *info)
{
    netdev_t *ifnet = skb->ifnet;
    splock_lock(&ifnet->lock);
    /* Ignore unsolicited packets */
    if (ifnet->dhcp_transaction != info->uid || ifnet->dhcp_mode != DHCP_REQUEST) {
        splock_unlock(&ifnet->lock);
        return -1;
    }

    /* Check proposal validity */
    bool valid = true;
    if (memcmp(skb->ip4_addr, info->dhcp_ip, IP4_ALEN) != 0)
        valid = false;

    if (!valid) {
        splock_unlock(&ifnet->lock);
        return -1;
    }

    /* We did request this IP, so take it */
    ifnet->dhcp_mode = 0;
    ifnet->dhcp_transaction = 0;
    ifnet->dhcp_lastsend = time64();
    memcpy(ifnet->ip4_addr, info->client_ip, IP4_ALEN);
    memcpy(ifnet->gateway_ip, info->rooter_ip, IP4_ALEN);
    ifnet->subnet_bits = 8; // TODO - info->submask_ip

    splock_unlock(&ifnet->lock);
    arp_query(ifnet, ifnet->gateway_ip);
    return 0;
}

static int dhcp_on_nack(skb_t *skb, dhcp_info_t *info)
{
    netdev_t *ifnet = skb->ifnet;
    splock_lock(&ifnet->lock);
    /* Ignore unsolicited packets */
    if (ifnet->dhcp_transaction != info->uid || ifnet->dhcp_mode != DHCP_REQUEST) {
        splock_unlock(&ifnet->lock);
        return -1;
    }

    // TODO -- Drop this one and request something else
    splock_unlock(&ifnet->lock);
    return 0;
}

static int dhcp_handle(skb_t *skb, dhcp_info_t *info)
{
    // TODO -- do we trust this server
    switch (info->msg_type) {
    case DHCP_OFFER:
        return dhcp_on_offer(skb, info);
    case DHCP_PACK:
        return dhcp_on_ack(skb, info);
    case DHCP_PNACK:
        return dhcp_on_nack(skb, info);
    case DHCP_INFORM:
        return 0;
    default:
        return -1;
    }
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

static int dhcp_on_discover(skb_t *skb, dhcp_server_t *srv, dhcp_info_t *info)
{
    int idx;
    splock_lock(&srv->lock);
    dhcp_lease_t *lease = (dhcp_lease_t *)hmp_get(&srv->leases,
                          (char *)info->client_mac, ETH_ALEN);
    if (lease == NULL) {
        lease = (dhcp_lease_t *)kalloc(sizeof(dhcp_lease_t));
        memcpy(lease->mac, info->client_mac, ETH_ALEN);
        lease->flags = 0;
        hmp_put(&srv->leases, (char *)info->client_mac, ETH_ALEN, lease);
    }
    splock_lock(&lease->lock);
    splock_unlock(&srv->lock);

    if (!(lease->flags & DHCP_LEASE_IP)) {
        // We need to find an IP...
        idx = -1;
        // TODO - is hostname known by DNS
        if (!ip4_null(info->query_ip)) {
            memcpy(lease->ip, info->query_ip, IP4_ALEN);
            idx = 0; // CHECK IS AVAILABLE
        }

        if (idx < 0) {
            for (idx = 0; idx < srv->ip_count; ++idx) {
                if (srv->ip_table[idx] == NULL || srv->ip_table[idx]->flags & DHCP_LEASE_FREE) {
                    memcpy(lease->ip, srv->ip_network, IP4_ALEN);
                    lease->ip[3] = idx; // Do increment
                    break;
                }
            }
            if (idx >= srv->ip_count) {
                // TODO - REMOVE FROM HMAP!
                return 0; // No luck all IP are taken!
            }
        }
        // Register in ip_table !
        srv->ip_table[idx] = lease;
        lease->flags |= DHCP_LEASE_IP;
    }

    if (!(lease->flags & DHCP_LEASE_HOSTNAME) && info->hostname) {
        lease->flags |= DHCP_LEASE_HOSTNAME;
        lease->hostname = strdup(info->hostname);
    }

    lease->timeout = time64() + info->lease_time * USEC_PER_SEC;

    host_register(lease->mac, lease->ip, lease->hostname, skb->ifnet->domain,
                  HOST_TEMPORARY);
    dhcp_packet(skb->ifnet, lease->ip, info->uid, DHCP_OFFER, info, lease);
    splock_unlock(&lease->lock);
    return 0;
}

static int dhcp_on_request(skb_t *skb, dhcp_server_t *srv, dhcp_info_t *info)
{
    splock_lock(&srv->lock);
    dhcp_lease_t *lease = (dhcp_lease_t *)hmp_get(&srv->leases,
                          (char *)info->client_mac, ETH_ALEN);
    if (lease == NULL) {
        dhcp_packet(skb->ifnet, info->client_ip, info->uid, DHCP_PNACK, info, NULL);
        splock_unlock(&srv->lock);
        return 0;
    }
    splock_lock(&lease->lock);
    splock_unlock(&srv->lock);

    // CHECK
    lease->timeout = time64() + info->lease_time * USEC_PER_SEC;
    memcpy(info->client_ip, info->query_ip, IP4_ALEN);
    dhcp_packet(skb->ifnet, info->client_ip, info->uid, DHCP_PACK, info, lease);
    splock_unlock(&lease->lock);
    return 0;
}

static int dhcp_on_decline(skb_t *skb, dhcp_server_t *srv, dhcp_info_t *info)
{
    splock_lock(&srv->lock);
    dhcp_lease_t *lease = (dhcp_lease_t *)hmp_get(&srv->leases,
                          (char *)info->client_mac, ETH_ALEN);
    if (lease == NULL) {
        // TODO -- Ack ?
        splock_unlock(&srv->lock);
        return 0;
    }

    // TODO - Is this been proposed to someone, ack ?
    splock_unlock(&srv->lock);
    return 0;
}

static int dhcp_on_release(skb_t *skb, dhcp_server_t *srv, dhcp_info_t *info)
{
    splock_lock(&srv->lock);
    dhcp_lease_t *lease = (dhcp_lease_t *)hmp_get(&srv->leases,
                          (char *)info->client_mac, ETH_ALEN);
    if (lease == NULL) {
        dhcp_packet(skb->ifnet, info->client_ip, info->uid, DHCP_PNACK, info, NULL);
        splock_unlock(&srv->lock);
        return 0;
    }
    splock_lock(&lease->lock);
    splock_unlock(&srv->lock);

    // CHECK - IP
    lease->timeout = 0;
    lease->flags |= DHCP_LEASE_FREE; // UNLESS RESERVED
    // !? dhcp_packet(ifnet, info->client_ip, info->uid, DHCP_PACK, info, NULL);
    splock_unlock(&lease->lock);
    return 0;
}

static int dhcp_serve(skb_t *skb, dhcp_server_t *srv, dhcp_info_t *info)
{
    switch (info->msg_type) {
    case DHCP_DISCOVER:
        return dhcp_on_discover(skb, srv, info);
    case DHCP_REQUEST:
        return dhcp_on_request(skb, srv, info);
    case DHCP_DECLINE:
        return dhcp_on_decline(skb, srv, info);
    case DHCP_RELEASE:
        return dhcp_on_release(skb, srv, info);
    case DHCP_INFORM:
        return 0;
    default:
        return -1;
    }
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

#define DHCP_DELAY  (5 * USEC_PER_SEC / 1000)

static int dhcp2boot[] = {
    0,
    BOOT_REQUEST, // DHCP_DISCOVER  1
    BOOT_REPLY, // DHCP_OFFER  2
    BOOT_REQUEST, // DHCP_REQUEST  3
    BOOT_REQUEST, // DHCP_DECLINE  4
    BOOT_REPLY, // DHCP_PACK  5
    BOOT_REPLY, // DHCP_PNACK  6
    BOOT_REQUEST, // DHCP_RELEASE  7
    BOOT_REQUEST, // DHCP_INFORM  8
};

int dhcp_packet(netdev_t *ifnet, const uint8_t *ip, uint32_t uid, int mode,
                const dhcp_info_t *info, const dhcp_lease_t *lease)
{
    uint8_t option[32];
    int bt_mode = dhcp2boot[mode];

    int opts = 3 + 9 + 8 + 1;
    if (bt_mode == BOOT_REQUEST)
        opts += dhcp_count_options_request(ifnet, ip, info);
    else
        opts += dhcp_count_options_reply(ifnet, ip, info, lease);

    skb_t *skb = dhcp_header(ifnet, ip, uid, bt_mode, opts);
    if (skb == NULL)
        return -1;

    // Options - message type, client mac, vendor
    option[0] = DHCP_OPT_MSGTYPE;
    option[1] = 1;
    option[2] = mode;
    net_write(skb, option, option[1] + 2);

    option[0] = DHCP_OPT_CLIENTMAC;
    option[1] = ETH_ALEN + 1;
    option[2] = 1;
    memcpy(&option[3], bt_mode == BOOT_REQUEST ? ifnet->eth_addr : skb->eth_addr,
           ETH_ALEN);
    net_write(skb, option, option[1] + 2);

    option[0] = DHCP_OPT_VENDOR;
    const char *vendor = "KoraOS";
    option[1] = (uint8_t)strlen(vendor);
    strncpy((char *)&option[2], vendor, 32 - 2);
    net_write(skb, option, option[1] + 2);

    if (bt_mode == BOOT_REQUEST)
        dhcp_write_options_request(ifnet, skb, option, ip, info);
    else
        dhcp_write_options_reply(ifnet, skb, option, ip, info, lease);

    // End of options
    option[0] = 0xFF;
    net_write(skb, option, 1);
    return net_send(skb);
}

int dhcp_discovery(netdev_t *ifnet)
{
    splock_lock(&ifnet->lock);
    if (ifnet->dhcp_mode != 0 && ifnet->dhcp_lastsend < time64() - DHCP_DELAY) {
        splock_unlock(&ifnet->lock);
        return -1;
    }
    ifnet->dhcp_mode = DHCP_DISCOVER;
    ifnet->dhcp_lastsend = time64();
    ifnet->dhcp_transaction = rand32();
    splock_unlock(&ifnet->lock);
    return dhcp_packet(ifnet, ip4_broadcast, ifnet->dhcp_transaction, DHCP_DISCOVER,
                       NULL, NULL);
}

int dhcp_receive(skb_t *skb, unsigned length)
{
    strncat(skb->log, "dhcp:", NET_LOG_SIZE);
    DHCP_header_t *header = net_pointer(skb, sizeof(DHCP_header_t));
    if (header == NULL || length < sizeof(DHCP_header_t))
        return -1;
    if (header->htype != 1 || header->hlen != 6)
        return -1;

    uint32_t magic = DHCP_MAGIC;
    if (net_read(skb, &magic, 4) != 0 || magic != DHCP_MAGIC)
        return -1;

    dhcp_info_t info;
    memset(&info, 0, sizeof(info));
    memcpy(info.client_mac, header->chaddr, ETH_ALEN);
    info.uid = header->xid;
    memcpy(info.client_ip, header->yiaddr, IP4_ALEN);
    memcpy(info.dhcp_ip, header->siaddr, IP4_ALEN);
    memcpy(info.rooter_ip, header->giaddr, IP4_ALEN);
    if (dhcp_parse(skb, &info, length - sizeof(DHCP_header_t) - 4) != 0)
        return -1;

    int ret;
    if (header->opcode == BOOT_REPLY)
        ret = dhcp_handle(skb, &info);
    else if (header->opcode == BOOT_REQUEST && skb->ifnet->dhcp_srv != NULL)
        ret = dhcp_serve(skb, skb->ifnet->dhcp_srv, &info);
    else
        ret = -1;

    if (info.domain)
        kfree(info.domain);
    if (info.hostname)
        kfree(info.hostname);
    if (info.vendor)
        kfree(info.vendor);
    return ret;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

dhcp_server_t *dhcp_create_server(uint8_t *ip_rooter, int netsz)
{
    dhcp_server_t *srv = (dhcp_server_t *)kalloc(sizeof(dhcp_server_t));
    splock_init(&srv->lock);
    hmp_init(&srv->leases, 16);
    srv->ip_count = (1 << netsz);
    memcpy(srv->ip_rooter, ip_rooter, IP4_ALEN);
    memcpy(srv->ip_network, ip_rooter, IP4_ALEN);
    uint32_t subnet_mask = htonl((1 << netsz) - 1);
    *((uint32_t *)srv->ip_network) &= ~subnet_mask;
    srv->netsz = netsz;
    srv->ip_table = (dhcp_lease_t **)kalloc(srv->ip_count * sizeof(dhcp_lease_t *));

    srv->ip_table[0] = (dhcp_lease_t *)kalloc(sizeof(dhcp_lease_t));
    // TODO -- not part of IPs range
    srv->ip_table[1] = (dhcp_lease_t *)kalloc(sizeof(dhcp_lease_t));
    // TODO - taken by rooter
    // Add DNS / DHCP
    return srv;
}

