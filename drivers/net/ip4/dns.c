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

#define IP4_PORT_DNS_SRV 53

typedef struct dns_header dns_header_t;
struct dns_header {
    uint16_t transaction;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
};

#define DNS_OPT_STANDARD_QRY 0x0100

#define DNS_TYPE_A 0x0001 // Host address IPv4
#define DNS_TYPE_NS 0x0002 // Server name
#define DNS_TYPE_MD 0x0003 // (Deprecated MX)
#define DNS_TYPE_MF 0x0004 // (Deprecated MX)
#define DNS_TYPE_CNAME 0x0005 // Canonical name
#define DNS_TYPE_SOA 0x0006 // Autority zone
#define DNS_TYPE_WKS 0x000B // Internet service 
#define DNS_TYPE_HINFO 0x000D // Machine information 
#define DNS_TYPE_MX 0x000F // Mail exchange
#define DNS_TYPE_TXT 0x0010 // Text

#define DNS_CLASS_IN 0x0001 // Internet

skb_t *dns_packet(ifnet_t* net, ip4_route_t* route, int length, uint16_t lport, dns_header_t** phead)
{
    length += sizeof(dns_header_t);
    skb_t* skb = net_packet(net);
    if (udp_header(skb, route, length, IP4_PORT_DNS_SRV, lport, 0, 0) != 0)
        return net_skb_trash(skb);

    net_skb_log(skb, ",dns");
    dns_header_t *head = net_skb_reserve(skb, sizeof(dns_header_t));
    memset(head, 0, sizeof(dns_header_t));
    *phead = head;
    return skb;
}


int dns_query_ip4(ifnet_t *net, const char *domain, uint16_t lport, uint16_t transac)
{
    int len = strlen(domain); 
    len += 2; // Count of '.' + 1
    int length = len + 4;
    dns_header_t* head;
    skb_t* skb = dns_packet(net, NULL, length, lport, &head);
    if (skb == NULL)
        return -1;
    
    head->transaction = transac;
    head->flags = DNS_OPT_STANDARD_QRY;
    head->qdcount = 1;

    uint8_t l;
    l = strlen(domain);
    net_skb_write(skb, &l, 1);
    net_skb_write(skb, domain, l);

    l = strlen(domain);
    net_skb_write(skb, &l, 1);
    net_skb_write(skb, domain, l);

    uint16_t tmp;
    tmp = htons(DNS_TYPE_A);
    net_skb_write(skb, &tmp, 2);
    tmp = htons(DNS_CLASS_IN);
    net_skb_write(skb, &tmp, 2);
    return net_skb_send(skb);
}

