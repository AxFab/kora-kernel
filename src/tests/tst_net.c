#include <kernel/core.h>
#include <kernel/files.h>
#include <kernel/net.h>
#include <threads.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include "check.h"

void futex_init();
void net_service();
void lo_setup();

void sock_recv(socket_t *sock, const char *buf, int len, int frag)
{
    skb_t *skb = kalloc(sizeof(skb_t) + len);
    memcpy(skb->buf, buf, len);
    skb->length = len;
    skb->size = len;
    skb->data_len = len;
    skb->fragment = frag;
    net_sock_recv(sock, skb);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


TEST_CASE(tst_sock_01)
{
    lo_setup();

    thrd_t service;
    thrd_create(&service, (thrd_start_t)net_service, NULL);

    socket_t *socket = net_socket(0, NULL);

    // Receiving
    sock_recv(socket, "ABCDEF", 6, 0);
    sock_recv(socket, "MNOPQR", 6, 2);
    sock_recv(socket, "YZ", 2, 4);
    sock_recv(socket, "GHIJKL", 6, 1);
    sock_recv(socket, "STUVWX", 6, 3);

    // Read
    int sz;
    char buf[32];
    memset(buf, 0, 32);
    sz = net_socket_read(socket, buf, 26);
    ck_ok(sz == 26 && strcmp(buf, "ABCDEFGHIJKLMNOPQRSTUVWXYZ") == 0);

    // Write
    socket->send_frag = 5;
    net_socket_write(socket, "abcdefghijklmnopqrstuvwxyz", 26);

    // Read
    memset(buf, 0, 32);
    //sz = net_socket_read(socket, buf, 26);
    //ck_ok(sz == 26 && strcmp(buf, "abcdefghijklmnopqrstuvwxuz") == 0);
}


jmp_buf __tcase_jump;
SRunner runner;

int main()
{
    kSYS.cpus = calloc(sizeof(struct kCpu), 0x500);
    futex_init();

    suite_create("Network");
    fixture_create("Sockets");
    tcase_create(tst_sock_01);

    free(kSYS.cpus);
    return summarize();
}


