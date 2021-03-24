//
// Created by sora on 2021/3/25.
//
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "def.h"

#define MAXLINE 1024
#define EPOLL_EVENT_SIZE 100

static int handle_echo(int fd) {
    int epfd = epoll_create(EPOLL_EVENT_SIZE);
    struct epoll_event srvev, stdinev, events[EPOLL_EVENT_SIZE];
    char buf[MAXLINE];
    srvev.data.fd = fd;
    srvev.events = EPOLLIN;
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &srvev);

    stdinev.data.fd = STDIN_FILENO;
    stdinev.events = EPOLLIN;
    epoll_ctl(epfd, EPOLL_CTL_ADD, STDIN_FILENO, &stdinev);

    while (1) {
        int ret = epoll_wait(epfd, events, EPOLL_EVENT_SIZE - 1, -1);
        if (ret == -1) {
            perror("epoll error");
            break;
        } else {
            int i;
            for (i = 0; i < ret; i++) {
                if (events[i].data.fd == STDIN_FILENO) {
                    scanf("%[^\n]%*c", buf);
                    strcat(buf, "\n");
                    write(fd, buf, strlen(buf) + 1);
                } else if (events[i].data.fd == fd) {
                    int retval = read(fd, buf, sizeof(buf));
                    if (retval < 0) {
                        perror("read error");
                        break;
                    } else if (retval == 0) {
                        printf("server closed.\n");
                        break;
                    } else {
                        printf("%s", buf);
                    }
                }
            }
        }
    }
    return 0;
}

int main(int argc, const char *argv[]) {
    int fd;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    bzero(&addr, addrlen);

    fd = socket(AF_INET, SOCK_STREAM, 0);
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, IPADDRESS, &addr.sin_addr);
    addr.sin_port = htons(PORT);

    if (connect(fd, (const struct sockaddr *) &addr, addrlen) < 0) {
        fprintf(stderr, "connect %s:%d fail:%s\n", IPADDRESS, PORT, strerror(errno));
        return -1;
    }

    printf("connect to server %s:%d.\n", IPADDRESS, PORT);
    handle_echo(fd);
}