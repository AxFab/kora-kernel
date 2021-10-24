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
#include "net.h"

int eth_receive(skb_t *);
int lo_receive(skb_t *);

void net_skb_recv(ifnet_t *ifnet, const void *buf, size_t len)
{
    skb_t *skb = kalloc(sizeof(skb_t) + len);
    skb->size = len;
    skb->ifnet = ifnet;
    skb->pen = skb->buf;
    strcpy(skb->log, "");
    memcpy(skb->buf, buf, len);

    // QUEUE PUSH skb
    mtx_lock(&kNET.lock);
    ll_append(&kNET.rx_queue, &skb->node);
    cnd_signal(&kNET.pulse);
    mtx_unlock(&kNET.lock);
}

void net_send(skb_t *skb)
{
    // TODO - AIO
    int ret = skb->ifnet->send(skb->ifnet, skb);
}

void net_rxloop()
{
    int ret;
    skb_t *skb = NULL; // QUEUE POLL
    for (;;) {
        mtx_lock(&kNET.lock);
        while (kNET.rx_queue.count_ == 0)
            cnd_wait(&kNET.pulse, &kNET.lock);
        skb = ll_dequeue(&kNET.rx_queue, skb_t, node);
        mtx_unlock(&kNET.lock);
        if (skb == NULL)
            continue;
        switch (skb->ifnet->type) {
        case INET_ETH:
            ret = eth_receive(skb);
            break;
        case INET_LO:
            ret = lo_receive(skb);
            break;
        default:
            ret = -1;
            break;
        }
        if (ret != 0)
            kfree(skb);
    }
}

skb_t *net_packet(ifnet_t *ifnet)
{
    skb_t *skb = kalloc(sizeof(skb_t) + ifnet->mtu);
    skb->size = ifnet->mtu;
    skb->ifnet = ifnet;
    skb->pen = skb->buf;
    strcpy(skb->log, "");
    return skb;
}

void *net_pointer(skb_t *skb, size_t len)
{
    void *ptr = skb->pen;
    if (skb->len + len > skb->size) {
        // todo set error
        return NULL;
    }
    skb->len += len;
    skb->pen += len;
    return ptr;
}


__thread inet_t *__net = NULL;
