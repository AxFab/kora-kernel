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
#include "ip4.h"

#define ARP_HW_ETH htons(1)
#define ARP_PC_IP htons(0x0800)

#define ARP_REQUEST htons(1)
#define ARP_REPLY htons(2)

#define ARP_TIMEOUT 10

typedef struct arp_header arp_header_t;

PACK(struct arp_header {
    uint16_t hardware;
    uint16_t protocol;
    uint8_t hw_length;
    uint8_t pc_length;
    uint16_t opcode;
});

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

static void arp_forget_with(ip4_info_t *info, net_qry_t *qry)
{
    splock_lock(&info->qry_lock);
    if (!qry->received)
        bbtree_remove(&info->qry_arps, qry->bnode.value_);
    splock_unlock(&info->qry_lock);
}

/* Send an ARP packet from local and provided parameters */
static int arp_packet(ifnet_t *ifnet, const uint8_t *mac, const uint8_t *ip, int opcode)
{
    assert(ifnet != NULL && ifnet->stack != NULL && ifnet->proto != NULL);
    if (ifnet->protocol != NET_AF_ETH)
        return - 1;

    skb_t *skb = net_packet(ifnet);
    if (unlikely(skb == NULL))
        return -1;
    if (unlikely(eth_header(skb, mac, ETH_ARP) != 0))
        return net_skb_trash(skb);

    net_log(skb, ",arp");
    arp_header_t *header = net_skb_reserve(skb, sizeof(arp_header_t));
    if (unlikely(header == NULL)) {
        net_log(skb, ":Unexpected end of data");
        return -1;
    }

    ip4_info_t *info = ip4_readinfo(ifnet);
    assert(info != NULL);
    header->hardware = ARP_HW_ETH;
    header->protocol = ARP_PC_IP;
    header->hw_length = ETH_ALEN;
    header->pc_length = IP4_ALEN;
    header->opcode = opcode;
    net_skb_write(skb, ifnet->hwaddr, ETH_ALEN);
    // TODO -- Check subnet is configured
    net_skb_write(skb, info->subnet.address, IP4_ALEN);
    net_skb_write(skb, mac, ETH_ALEN);
    net_skb_write(skb, ip, IP4_ALEN);
    return net_skb_send(skb);
}

/* Handle an ARP package, respond to request and save result of response */
int arp_receive(skb_t *skb)
{
    assert(skb && skb->ifnet);
    net_log(skb, ",arp");
    arp_header_t *header = net_skb_reserve(skb, sizeof(arp_header_t));
    if (header == NULL) {
        net_log(skb, ":Unexpected end of data");
        return -1;
    }

    if (header->hardware != ARP_HW_ETH || header->protocol != ARP_PC_IP
        || header->hw_length != ETH_ALEN || header->pc_length != IP4_ALEN) {
        net_log(skb, ":Unsupported format");
        return -1;
    }

    uint8_t source_mac[ETH_ALEN];
    uint8_t source_ip[IP4_ALEN];
    uint8_t target_mac[ETH_ALEN];
    uint8_t target_ip[IP4_ALEN];
    net_skb_read(skb, source_mac, ETH_ALEN);
    net_skb_read(skb, source_ip, IP4_ALEN);
    net_skb_read(skb, target_mac, ETH_ALEN);
    net_skb_read(skb, target_ip, IP4_ALEN);

    ip4_info_t *info = ip4_readinfo(skb->ifnet);
    assert(info);
    // TODO -- Check subnet is configured
    if (memcmp(info->subnet.address, target_ip, IP4_ALEN) != 0)
        return -1;
    if (header->opcode == ARP_REQUEST)
        arp_packet(skb->ifnet, source_mac, source_ip, ARP_REPLY);
    else if (header->opcode == ARP_REPLY) {
        if (info->subnet.ifnet == NULL)
            return -1;

        // TODO -- Check this is not unsollisited packet
        // TODO -- Protect aggainst ARP poisonning
        if (memcmp(info->subnet.gateway, source_ip, IP4_ALEN) == 0)
            memcpy(info->subnet.gateway_addr, source_mac, ETH_ALEN);

        // TODO -- Should we populate the IP cache
        splock_lock(&info->qry_lock);
        net_qry_t *qry = bbtree_search_eq(&info->qry_arps, *((uint32_t *)&source_ip), net_qry_t, bnode);
        if (qry != NULL)
            bbtree_remove(&info->qry_arps, qry->bnode.value_);
        splock_unlock(&info->qry_lock);

        if (qry != NULL) {
            qry->elapsed = xtime_read(XTIME_CLOCK) - qry->start;
            memcpy(qry->res, source_mac, ETH_ALEN);
            qry->len = ETH_ALEN;
            mtx_lock(&qry->mtx);
            qry->received = qry->success = true;
            cnd_signal(&qry->cnd);
            mtx_unlock(&qry->mtx);
        }
    }

    kfree(skb);
    return 0;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

/* Send an ARP request to look for harware address of the following ip */
int arp_whois(ifnet_t *net, const uint8_t *ip, net_qry_t *qry)
{
    ip4_info_t *info = NULL;
    if (qry != NULL) {
        info = ip4_readinfo(net);
        memset(qry, 0, sizeof(net_qry_t));
        cnd_init(&qry->cnd);
        mtx_init(&qry->mtx, mtx_plain);
        qry->start = xtime_read(XTIME_CLOCK);
        mtx_lock(&qry->mtx);

        splock_lock(&info->qry_lock);
        qry->bnode.value_ = *((uint32_t *)ip);
        bbtree_insert(&info->qry_arps, &qry->bnode);
        splock_unlock(&info->qry_lock);
    }

    int ret = arp_packet(net, eth_broadcast, ip, ARP_REQUEST);
    if (ret == 0)
        return ret;

    if (qry != NULL) {
        mtx_lock(&qry->mtx);
        arp_forget_with(info, qry);
    }
    return -1;
}

/* Unregistered a timedout ARP query */
void arp_forget(ifnet_t *net, net_qry_t *qry)
{
    ip4_info_t *info = ip4_readinfo(net);
    assert(info != NULL && qry != NULL);
    arp_forget_with(info, qry);
}
