#include <string.h>
#include <stdbool.h>
#include <kora/socket.h>
#include <kora/splock.h>
#include <kora/hmap.h>
#include <threads.h>
#include <windows.h>
#include <stdio.h>

#define MTU 1500
bool cancel = false;
splock_t mac_lock;
HMP_map mac_table;
char mac_broadcast[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };


typedef struct msg msg_t;
struct msg {
    int request, length;
};

#define MR_INIT  1
#define MR_PACKET  2
#define MR_LINK  3
#define MR_UNLINK  4


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
    hmp_put(&mac_table, mac, 6, (void *)fd);
    splock_unlock(&mac_lock);
}

//
void send_host(int fd, msg_t *msg, char *frame)
{
    splock_lock(&mac_lock);
    sock_send(fd, msg, sizeof(*msg));
    if (msg->length != 0)
        sock_send(fd, frame, msg->length);
    splock_unlock(&mac_lock);
}

void broadcast(int unless, msg_t *msg, char *frame)
{

}

/* -=- */

int vfs_read() {}
int vfs_write() {}

// Handle the request of new host
int handler(int fd)
{
    int target;
    msg_t msg;
    char *frame = malloc(MTU);
    for (;;) {
        sock_recv(fd, &msg, sizeof(msg));
        if (msg.length)
            sock_recv(fd, frame, msg.length);
        switch (msg.request) {
        case MR_INIT:
            msg.request = MR_LINK;
            msg.length = 0;
            send_host(fd, &msg, NULL);
            new_host(fd, frame);
            break;
        case MR_PACKET:
            target = lookup(frame);
            if (target > 0)
                send_host(target, &msg, frame);
            else if (target == -1)
                broadcast(fd, &msg, frame);
            break;
        }
    }
}

int main()
{
    sock_init();
    splock_init(&mac_lock);
    hmp_init(&mac_table, 16);

    // listen on TCP:8080 for new host
    int srv = sock_listen(NPC_IPv4_TCP, 0, NAD_IPv4, "192.168.0.0/16:14148");

    // Wait for new hosts
    printf("Waiting...\n");
    while (!cancel) {
        int fd = sock_accept(srv, 50);
        if (fd == 0)
            continue;
        thrd_create(NULL, (thrd_start_t)handler, fd);
    }

    // Broadcast unlink !
    msg_t msg;
    msg.request = MR_UNLINK;
    msg.length = 0;
    broadcast(0, &msg, NULL);
    sock_fini();
    return 0;
}

