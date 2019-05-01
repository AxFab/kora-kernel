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
#include <kernel/core.h>
#include <kernel/net.h>
#include <string.h>
#include <time.h>
#include <threads.h>
#include "../check.h"


typedef void *(*pfunc_t)(void *);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
jmp_buf __tcase_jump;

netdev_t eth1;
thrd_t thread1;
uint8_t mac1[ETH_ALEN] = { 0x08, 0x04, 0x06, 0x46, 0xef, 0xc3 };

netdev_t eth2;
thrd_t thread2;
uint8_t mac2[ETH_ALEN] = { 0x08, 0x07, 0x02, 0x91, 0xa3, 0x6d };
uint8_t ip2[IP4_ALEN] = { 192, 168, 0, 1 };


atomic_int cnt = 300;
splock_t net_lock;

int pack = 0;

void net_tasklet(netdev_t *);
void net_start(netdev_t *ifnet)
{
    net_tasklet(ifnet);
    thrd_exit(0);
}

void async_wait(splock_t *lock, llhead_t *list, long timeout_us)
{
    struct timespec req;
    req.tv_sec = timeout_us / USEC_PER_SEC;
    req.tv_nsec = (timeout_us % USEC_PER_SEC) * 1000LL;
    if (lock != NULL)
        assert(splock_locked(lock));
    // TODO -- Push on the list
    if (lock != NULL)
        splock_unlock(lock);
    nanosleep(&req, NULL);
    atomic_dec(&cnt);
    if (cnt <= 0) {
        eth1.flags |= NET_QUIT;
        eth2.flags |= NET_QUIT;
    }

    if (lock != NULL)
        splock_lock(lock);
}


void task_create(void *start, void *arg, CSTR name)
{
    if ((void *)arg == &eth1)
        thrd_create(&thread1, (pfunc_t)net_start, arg);
    else if ((void *)arg == &eth2)
        thrd_create(&thread2, (pfunc_t)net_start, arg);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int sendUT(netdev_t *ifnet, skb_t *skb)
{
    splock_lock(&ifnet->lock);
    splock_lock(&net_lock);
    kprintf(KLOG_DBG, "Packet send by eth%d: %s (%d)\n", ifnet->no, skb->log,
            skb->length);
    // dump(skb->buf, skb->length);
    // kprintf(KLOG_DBG, "\n");
    // check packets
    splock_unlock(&net_lock);
    splock_unlock(&ifnet->lock);
    net_recv(ifnet == &eth1 ? &eth2 : &eth1, skb->buf, skb->length);
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
    host_init();

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
    eth2.flags |= NET_CONNECTED;
    eth2.domain = strdup("axfab.net");
    eth2.send = sendUT;
    eth2.link = linkUT;
    eth2.dhcp_srv = dhcp_create_server(ip2, 8);
    net_device(&eth2);

    int ret;
    thrd_join(thread1, &ret);
    thrd_join(thread2, &ret);
    return 0;
}
