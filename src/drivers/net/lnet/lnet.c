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
#include <WinSock2.h>
#include <kernel/net.h>
#include <threads.h>
#include "lnet.h"
#pragma comment(lib, "ws2_32.lib")

#define send(s,m,l) send(s,m,l,0)
#define recv(s,m,l) recv(s,m,l,0)

void lnet_link(lnet_dev_t *ifnet)
{
    if (ifnet->fd == 0) {
        return;
    }
    lnet_msg_t msg;
    msg.request = MR_INIT;
    msg.length = ETH_ALEN;
    send(ifnet->fd, &msg, sizeof(msg));
    send(ifnet->fd, &ifnet->n.eth_addr, ETH_ALEN);
}

void lnet_send(lnet_dev_t *ifnet, skb_t *skb)
{
    lnet_msg_t msg;
    msg.request = MR_PACKET;
    msg.length = skb->length;
    send(ifnet->fd, &msg, sizeof(msg));
    send(ifnet->fd, skb->buf, skb->length);
}

void lnet_idle(lnet_dev_t *ifnet)
{
    lnet_msg_t msg;
    char *frame = kalloc(ifnet->n.mtu);
    for (;;Sleep(3000)) {
        if (ifnet->fd == 0) {

            SOCKADDR_IN sin;
            sin.sin_addr.s_addr = inet_addr("127.0.0.1");
            sin.sin_family = AF_INET;
            sin.sin_port = 14148;
            SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock == -1)
                continue;
            sin.sin_port = 14149;
            if (bind(sock, (SOCKADDR *)&sin, sizeof(sin)) != 0)
                continue;
            sin.sin_port = 14148;
            if (connect(sock, (SOCKADDR *)&sin, sizeof(sin)) != 0)
                continue;
            ifnet->fd = sock;
        }

        while (ifnet->fd) {
            recv(ifnet->fd, &msg, sizeof(msg));
            if (msg.length != 0)
                recv(ifnet->fd, frame, msg.length);
            switch (msg.request) {
            case MR_LINK:
                ifnet->n.flags |= NET_CONNECTED;
                break;
            case MR_PACKET:
                net_recv(&ifnet->n, frame, msg.length);
                break;
            case MR_UNLINK:
                close(ifnet->fd);
                ifnet->fd = 0;
                break;
            }
        }
    }
    kfree(frame);
}

void lnet_setup()
{
#ifdef _WIN32
    WSADATA WSAData;
    WSAStartup(MAKEWORD(2, 0), &WSAData);
#endif

    int i;
    lnet_dev_t *ifnet = kalloc(sizeof(lnet_dev_t));
    for (i = 0; i < ETH_ALEN; ++i)
        ifnet->n.eth_addr[i] = rand16();
    ifnet->n.mtu = 1500;
    ifnet->n.link = lnet_link;
    ifnet->n.send = lnet_send;
    thrd_t thrd;
    thrd_create(&thrd, (thrd_start_t)lnet_idle, ifnet);
    net_device(ifnet);
}

void lnet_teardown()
{
#ifdef _WIN32
    WSACleanup();
#endif
}



MODULE(lnet, lnet_setup, lnet_teardown);
