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
#include <string.h>

typedef struct NTP_header NTP_header_t;

PACK(struct NTP_header {
    uint8_t flags;
    uint8_t stratum;
    uint8_t polling_interval;
    uint8_t clock_precision;
    uint32_t root_delay;
    uint32_t clock_dispertion;
    uint32_t clock_id;
    uint32_t clock_update;
    uint64_t origin_time;
    uint64_t receive_time;
    uint64_t transmit_time;
});

int ntp_packet(netdev_t *ifnet)
{
    uint8_t ntp_server_ip[IP4_ALEN];
    skb_t *skb = net_packet(ifnet, 128);
    if (skb == NULL)
        return -1;
    if (udp_header(skb, ntp_server_ip, sizeof(NTP_header_t), UDP_PORT_NTP, 0) != 0)
        return net_trash(skb);
    strncat(skb->log, "ntp:", NET_LOG_SIZE);
    NTP_header_t header;
    (void)header;
    // ...
    return net_send(skb);
}

int ntp_receive(skb_t *skb, int length)
{
    NTP_header_t header;
    strncat(skb->log, "ntp:", NET_LOG_SIZE);
    net_read(skb, &header, sizeof(header));
    kprintf(-1, "NTP, \n");
    return 0;
}

