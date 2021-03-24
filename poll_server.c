//
// Created by sora on 2021/3/25.
//

#include <poll.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include "def.h"

#define SIZE 100
#define POLL_READ (POLLIN | POLLPRI)

static int handle_accept(int srvfd) {
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    int connfd = accept(srvfd, (struct sockaddr *) &addr, &addrlen);
    if (connfd == -1) {
        if (errno == EINTR) {
            return EINTR;
        } else {
            perror("accept error");
            return -1;
        }
    }
    printf("new connection from %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
    return connfd;
}

static int handle_echo(int connfd, char *buf) {
    bzero(buf, MAXLINE);
    int nread = read(connfd, buf, MAXLINE);
    if (nread == -1) {
        perror("read error");
        close(connfd);
        return -1;
    } else if (nread == 0) {
        struct sockaddr_in addr;
        int ret = get_addrin_from_fd(connfd, &addr);
        if (ret == 0) {
            fprintf(stderr, "connection %s:%d closed\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
        } else {
            fprintf(stderr, "connection closed\n");
        }
        close(connfd);
        return 0;
    } else {
        struct sockaddr_in addr;
        int ret = get_addrin_from_fd(connfd, &addr);

        if (ret == 0) {
            printf("message from %s:%d : %s", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port), buf);
        } else {
            printf("message : %s", buf);
        }

        int nwrite = write(connfd, buf, strlen(buf));
        if (nwrite == -1) {
            perror("write error");
            close(connfd);
            return -1;
        }
    }
    return nread;
}

static void do_poll(int srvfd) {
    int i, ret, connfd;
    struct pollfd fds[SIZE];
    bzero(fds, sizeof(fds));
    char buf[MAXLINE];
    fds[0].fd = srvfd;
    fds[0].events = POLL_READ;
    for (i = 1; i < SIZE; i++) {
        fds[i].fd = -1;
    }
    while (1) {
        ret = poll(fds, SIZE, -1);
        if (ret == -1) {
            perror("poll error");
            return;
        } else if (ret == 0) {
            continue;
        }
        for (i = 1; i < SIZE; i++) {
            if (fds[i].fd > 0 && ((fds[i].revents & POLLIN) || (fds[i].revents & POLLPRI))) {
                ret = handle_echo(fds[i].fd, buf);
                if (ret <= 0) {
                    fds[i].fd = -1;
                    fds[i].events = 0;
                    fds[i].revents = 0;
                }
            }
        }

        if ((fds[0].revents & POLLIN) || (fds[0].revents & POLLPRI)) {
            connfd = handle_accept(srvfd);
            for (i = 1; i < SIZE; i++) {
                if (fds[i].fd < 0) {
                    fds[i].fd = connfd;
                    fds[i].events = POLL_READ;
                    break;
                }
            }
            if (i == SIZE) {
                printf("too many connections\n");
                close(connfd);
            }
        }
    }
}

int main(int argc, const char *argv[]) {
    int fd;
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        fprintf(stderr, "create socket failed: %s", strerror(errno));
        return -1;
    }
    bzero(&addr, len);

    addr.sin_family = AF_INET;
    inet_pton(AF_INET, IPADDRESS, &addr.sin_addr);
    addr.sin_port = htons(PORT);
    if (bind(fd, (const struct sockaddr *) &addr, len) == -1) {
        perror("bind error");
        return -1;
    }
    if (listen(fd, LISTENQ) == -1) {
        perror("listen error");
        return -1;
    }
    do_poll(fd);
}