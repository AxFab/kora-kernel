/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2019  <Fabien Bavent>
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
#include <string.h>

typedef struct DNS_header DNS_header_t;
PACK(struct DNS_header {
    uint16_t id;
    uint16_t opcode;
    uint16_t qd_count;
    uint16_t an_count;
    uint16_t ns_count;
    uint16_t ar_count;
});



int dns_packet(netdev_t *ifnet)
{
    skb_t *skb = net_packet(ifnet, 500);
    if (skb == NULL)
        return -1;
    int length = 0;
    if (udp_header(skb, ifnet->dns_ip, length, UDP_PORT_DNS, 0) != 0)
        return net_trash(skb);
    strncat(skb->log, "dns:", NET_LOG_SIZE);
    DNS_header_t header;
    header.id = rand16();
    header.opcode = htonw(0x100); // Simple Req
    header.qd_count = 1;
    net_write(skb, &header, sizeof(header));
    // Write domaine
    // Write record
    // Write IP
    // push dns request
    return net_send(skb);
}

int dns_receive(skb_t *skb, int length)
{
    DNS_header_t header;
    strncat(skb->log, "dns:", NET_LOG_SIZE);
    net_read(skb, &header, sizeof(header));
    kprintf(-1, "DNS, \n");
    return 0;
}

