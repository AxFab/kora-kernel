

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

void net_address_ipv4_port(char *buf, const char *addr, int port);

int open_socket(int protocol, int addr, const char *buf);

int open_server(int protocol, int addr, const char *bud, int backlog);
	
int accept_socket(int srv, long timeout);
		
int send(int sock, const void *buf, int lg);
int recv(int sock, void *buf, int lg); 



#endif  /* _KORA_SOCKET_H */
