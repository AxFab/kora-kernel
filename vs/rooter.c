#include <string.h>
#include <kora/socket.h>

#define MTU 1500
bool cancel = false;
splock_t mac_lock;
HMP_map mac_table;
char *mac_broadcast = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

// Search the socket connected to this mac address
int lookup(char *mac)
{
	int fd;
	splock_lock(&mac_lock);
	if (strcmp(mac, mac_broadcast) == 0)
	    fd = -1;
	else
	    fd = (int)hmp_get(&mac_table, mac, 6);
	splock_unlock(&mac_lock);
	return fd;
}

// Register a new host with this mac address
void new_host(int fd, char *mac)
{
	splock_lock(&mac_lock);
	hmp_put(&mac_table, mac, 6, (void*)fd);
	splock_unlock(&mac_lock);
}

// 
void send_msg(int fd, msg_t *msg, char *frame)
{
	splock_lock(&mac_lock);
	send(fd, msg, sizeof(*msg));
	if (msg.length != 0)
      send(fd, frame, msg.length);
	splock_unlock(&mac_lock);
}

void broadcast(int unless, msg_t *msg, char *frame)
{
	
}

// Handle the request of new host
void handle(int fd)
{
	msg_t msg;
	char *frame = malloc(MTU);
	for (;;) {
		recv(fd, &msg, sizeof(msg));
        if (msg.length)
            recv(fd, frame, msg.length);
        switch (msg.request) {
        case MR_INIT:
            msg.request = MR_LINK;
            msg.length = 0;
            send_host(fd, &msg, NULL);
            new_host(fd, frame);
            break;
        case MR_PACKET:
            int target = lookup(frame);
            if (target > 0)
               send_host(target, &msg, frame);
            else if (target == -1) {
                broadcast(fd, &msg, frame);
            break;
        }
	}
}
	
int main() 
{
	splock_init(&mac_lock);
	hmp_init(&mac_table);
	
	// listen on TCP:8080
	char addr[8];
	net_address_ipv4_port(addr, "192.168.0.0/16", 8080);
	int srv = open_server(NPC_IPv4_TCP, NAD_IPv4, addr, 128);
	
	// Wait for new hosts 
	while (!cancel) {
		int fd = accept_socket(srv, 50);
		if (fd == 0)
		    continue;
		thread_start(handler, fd);
	}
	
	// Broadcast unlink !
	msg_t msg;
	msg.request = MR_UNLINK;
	msg.length = 0;
	broadcast (0, &msg, NULL);
	return 0;
}

