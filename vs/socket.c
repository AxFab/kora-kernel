#include <kora/socket.h>
#include <winsock2.h>
#include <windows.h>
#pragma lib()

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
	sin.sin_addr.s_addr = ifnet_addr("127.0.0.1");
	sin.sin_family = AF_IFNET;
	sin.sin_port = 14148;
	SOCKET sock = socket(AF_IFNET, SOCK_STREAM, 0);
	bind(sock
}

/* Create an endpoint to listen incoming sockets */
int sock_listen(int protocole, int backlog, int addrtype, const char *buf);
/* Accept client socket to connect */
int sock_accept(int srv, long timeout);

/* Write data to a socket */
int send(int sock, const void *buf, int lg);
/* Read data of a socket (blocking) */
int recv(int sock, void *buf, int lg); 
/* Ask the kernel to consider this socket as incoming once new data is received */
int sock_sleep(int sock);

