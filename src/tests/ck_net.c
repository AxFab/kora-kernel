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
#include <kernel/core.h>
#include <kernel/net.h>
#include <string.h>
#include <time.h>

#include <pthread.h>

time64_t time64()
{
    return time(NULL) * 1000000L;
}


void vfs_read() {}



/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

netdev_t eth1;
pthread_t thread1;
char mac1[ETH_ALEN] = { 0x08, 0x04, 0x06, 0x46, 0xef, 0xc3 };

netdev_t eth2;
pthread_t thread2;
char mac2[ETH_ALEN] = { 0x08, 0x07, 0x02, 0x91, 0xa3, 0x6d };
char ip2[IP4_ALEN] = { 192, 168, 0, 1 };


int cnt = 8;
splock_t net_lock;

int pack = 0;
void gateway_stub(netdev_t *ifnet, skb_t *skb)
{
    switch (pack++) {
    case 1:
        assert(strcmp(skb->log, "eth:ipv4:udp:dhcp:") == 0);
        // Ignore first call
        break;
    case 2:
        assert(strcmp(skb->log, "eth:ipv4:udp:dhcp:") == 0);
        dhcp_offer(ifnet);
        break;

    }

}

void gateway_tasklet(netdev_t *ifnet)
{
    while ((eth2.flags & NET_QUIT) == 0) {

        skb_t *skb;
        splock_lock(&net_lock);
        splock_lock(&ifnet->lock);
        // kprintf(-1, "SRV\n");
        // while (timeout - time64() > 0) {
        skb = ll_dequeue(&ifnet->queue, skb_t, node);
        splock_unlock(&ifnet->lock);
        splock_unlock(&net_lock);
        if (skb == NULL) {
            // splock_unlock(&ifnet->lock);
            sleep(1);
            continue;
            // advent_wait(&ifnet->lock, ..., timeout - time64());
        } else {
            // splock_lock(&net_lock);
            // kprintf(-1, "RECEIVE\n");
            // splock_unlock(&net_lock);
            int ret = eth_receive(skb);
            // splock_lock(&net_lock);
            gateway_stub(ifnet, skb);
            // splock_unlock(&net_lock);
            if (ret != 0)
               ifnet->rx_errors++;
            kfree(skb);
        }


        sleep(1);
    }
}


void net_start(netdev_t *ifnet)
{
    if (ifnet == &eth1) {
        net_tasklet(ifnet);
        pthread_exit(NULL);
    } else {
        gateway_tasklet(ifnet);
        pthread_exit(NULL);
    }
}

void advent_wait()
{
    sleep(1);
    if (cnt-- <= 0) {
        eth1.flags |= NET_QUIT;
        eth2.flags |= NET_QUIT;
    }
}


void kernel_tasklet(void *start, long arg, CSTR name)
{
    if ((void*)arg == &eth1)
        pthread_create(&thread1, NULL, net_start, (void*)arg);
    else if ((void*)arg == &eth2)
        pthread_create(&thread2, NULL, net_start, (void*)arg);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int sendUT(netdev_t *ifnet, skb_t *skb)
{
    splock_lock(&net_lock);
    splock_lock(&ifnet->lock);
    kprintf(KLOG_DBG, "Packet send by eth%d: %s (%d)\n", ifnet->no, skb->log, skb->length);
    kdump(skb->buf, skb->length);
    kprintf(KLOG_DBG, "\n");
    splock_unlock(&ifnet->lock);
    net_recv(ifnet == &eth1 ? &eth2 : &eth1, skb->buf, skb->length);
    splock_unlock(&net_lock);
    return 0;
}

int linkUT(netdev_t *ifnet)
{
    // Start link
    ifnet->flags |= NET_CONNECTED;
    return 0;
}




/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int main()
{
    memset(&eth1, 0, sizeof(eth1));
    memcpy(eth1.eth_addr, mac1, ETH_ALEN);
    eth1.mtu = 1500;
    eth1.send = sendUT;
    eth1.link = linkUT;
    net_device(&eth1);


    memset(&eth2, 0, sizeof(eth2));
    memcpy(eth2.eth_addr, mac2, ETH_ALEN);
    memcpy(eth2.ip4_addr, ip2, ETH_ALEN);
    eth2.mtu = 1500;
    eth2.send = sendUT;
    eth2.link = linkUT;
    net_device(&eth2);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    return 0;
}