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
#ifndef _KORA_SOCKET_H
#define _KORA_SOCKET_H 1

#define NPC_ETHER  0x10000
#define NPC_IPv4  (NPC_ETHER | 0x100)
#define NPC_IPv4_UDP  (NPC_IPv4 | 1)
#define NPC_IPv4_TCP  (NPC_IPv4 | 2)
#define NPC_IPv6  (NPC_ETHER | 0x200)


#define NAD_ETHER  6
#define NAD_IPv4  4
#define NAD_IPv6  16
#define NAD_FQDN  1

/* Open a client socket to a server */
int sock_open(int protocole, int addrtype, const char *buf);
/* Create an endpoint to listen incoming sockets */
int sock_listen(int protocole, int backlog, int addrtype, const char *buf);
/* Accept client socket to connect */
int sock_accept(int srv, long timeout);

/* Write data to a socket */
int sock_send(int sock, const void *buf, int lg);
/* Read data of a socket (blocking) */
int sock_recv(int sock, void *buf, int lg);
/* Ask the kernel to consider this socket as incoming once new data is received */
int sock_sleep(int sock);



#endif  /* _KORA_SOCKET_H */
