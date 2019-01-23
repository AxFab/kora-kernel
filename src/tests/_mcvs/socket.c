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
#include <kora/socket.h>
#include <WinSock2.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")

void sock_init()
{
    WSADATA WSAData;
    WSAStartup(MAKEWORD(2, 0), &WSAData);
}

void sock_fini()
{
    WSACleanup();
}

/* Open a client socket to a server */
int sock_open(int protocole, int addrtype, const char *buf)
{
    SOCKADDR_IN sin;
    sin.sin_addr.s_addr = inet_addr("127.0.0.1");
    sin.sin_family = AF_INET;
    sin.sin_port = 14148;
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    bind(sock, (SOCKADDR *)&sin, sizeof(sin));

    connect(sock, (SOCKADDR *)&sin, sizeof(sin));
    return sock;
}

/* Create an endpoint to listen incoming sockets */
int sock_listen(int protocole, int backlog, int addrtype, const char *buf)
{
    SOCKADDR_IN sin;
    sin.sin_addr.s_addr = inet_addr("127.0.0.1");
    sin.sin_family = AF_INET;
    sin.sin_port = 14148;
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    bind(sock, (SOCKADDR *)&sin, sizeof(sin));

    listen(sock, backlog);
    return sock;
}

/* Accept client socket to connect */
int sock_accept(int sock, long timeout)
{
    SOCKADDR_IN csin;
    int sz = sizeof(csin);
    for (;;) {
        SOCKET val = accept(sock, (SOCKADDR *)&csin, &sz);
        if (val != INVALID_SOCKET)
            return val;
    }
}

/* Write data to a socket */
int sock_send(int sock, const void *buf, int lg)
{
    return send((SOCKET)sock, buf, lg, 0);
}

/* Read data of a socket (blocking) */
int sock_recv(int sock, void *buf, int lg)
{
    return recv((SOCKET)sock, buf, lg, 0);
}

/* Ask the kernel to consider this socket as incoming once new data is received */
int sock_sleep(int sock)
{
    return -1;
}

