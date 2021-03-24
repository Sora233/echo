//
// Created by sora on 2021/3/23.
//

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <assert.h>
#include <unistd.h>
#include "def.h"

#define SIZE 100

typedef struct server_context_st {
    int clifds[SIZE];
} server_context_st;

static server_context_st *s_srv_ctx = NULL;

static int max(int a, int b) {
    return a > b ? a : b;
}

static int create_server_proc(const char *ip, int port) {
    int fd;
    struct sockaddr_in servaddr;
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        fprintf(stderr, "create socket fail, erron:%d, reason:%s\n", errno, strerror(errno));
        return -1;
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &servaddr.sin_addr);
    servaddr.sin_port = htons(port);

    if (bind(fd, (const struct sockaddr *) &servaddr, sizeof(servaddr)) == -1) {
        perror("bind error: ");
        return -1;
    }

    listen(fd, LISTENQ);

    return fd;
}

static int accept_client_proc(int srvfd) {
    struct sockaddr_in cliaddr;
    socklen_t cliaddrlen = sizeof(cliaddr);
    int clifd = -1;
    int i = 0;
    printf("accpet clint proc is called.\n");
    while (1) {
        clifd = accept(srvfd, (struct sockaddr *) &cliaddr, &cliaddrlen);
        if (clifd == -1) {
            if (errno == EINTR) {
                continue;
            } else {
                fprintf(stderr, "accept fail, error:%s\n", strerror(errno));
                return -1;
            }
        }
        break;
    }
    fprintf(stdout, "accept a new client: %s:%d\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
    for (; i < SIZE; i++) {
        if (s_srv_ctx->clifds[i] < 0) {
            s_srv_ctx->clifds[i] = clifd;
            break;
        }
    }
    if (i == SIZE) {
        fprintf(stderr, "too many clients.\n");
        close(clifd);
        return -1;
    }
    return 0;
}

static int handle_client_msg(int fd, char *buf) {
    assert(buf);
    printf("recv buf is: %s", buf);
    write(fd, buf, strlen(buf) + 1);
    return 0;
}

static void recv_client_msg(fd_set *readfds) {
    int i = 0, n = 0;
    int clifd;
    char buf[MAXLINE] = {0};
    for (i = 0; i < SIZE; i++) {
        clifd = s_srv_ctx->clifds[i];
        if (clifd < 0) {
            continue;
        }
        if (FD_ISSET(clifd, readfds)) {
            n = read(clifd, buf, MAXLINE);
            if (n <= 0) {
                struct sockaddr_in addr;
                if (get_addrin_from_fd(clifd, &addr) == -1) {
                    perror("get_addrin_from_fd");
                } else {
                    printf("connection closed: %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
                }
                FD_CLR(clifd, readfds);
                close(clifd);
                s_srv_ctx->clifds[i] = -1;
            } else {
                handle_client_msg(clifd, buf);
            }
        }
    }
}

static void do_select(int srvfd) {
    int clifd = -1;
    int retval = 0;
    int maxfd = srvfd;
    int i = 0;
    struct timeval tv;
    fd_set *readfds = (fd_set *) malloc(sizeof(fd_set));
    while (1) {
        FD_ZERO(readfds);
        FD_SET(srvfd, readfds);
        maxfd = srvfd;
        for (i = 0; i < SIZE; i++) {
            clifd = s_srv_ctx->clifds[i];
            if (clifd != -1) {
                FD_SET(clifd, readfds);
            }
            maxfd = max(clifd, maxfd);
        }
        tv.tv_sec = 30;
        tv.tv_usec = 0;
        retval = select(maxfd + 1, readfds, NULL, NULL, &tv);
        if (retval == -1) {
            fprintf(stderr, "select error: %s.\n", strerror(errno));
            return;
        }
        if (retval == 0) {
            fprintf(stdout, "select is timeout.\n");
            continue;
        }
        if (FD_ISSET(srvfd, readfds)) {
            accept_client_proc(srvfd);
        } else {
            recv_client_msg(readfds);
        }
    }
}

static void server_uninit() {
    if (s_srv_ctx != NULL) {
        free(s_srv_ctx);
        s_srv_ctx = NULL;
    }
}

static int server_init() {
    s_srv_ctx = (server_context_st *) malloc(sizeof(struct server_context_st));
    if (s_srv_ctx == NULL) {
        return -1;
    }
    bzero(s_srv_ctx, sizeof(server_context_st));
    memset(s_srv_ctx->clifds, -1, sizeof(s_srv_ctx->clifds));
    return 0;
}

int main(int argc, const char *argv[]) {
    int srvfd;
    if (server_init() == -1) {
        return -1;
    }
    srvfd = create_server_proc(IPADDRESS, PORT);
    if (srvfd < 0) {
        fprintf(stderr, "socket create or bind fail.\n");
        server_uninit();
        return -1;
    }
    do_select(srvfd);
    server_uninit();
    return 0;
}
