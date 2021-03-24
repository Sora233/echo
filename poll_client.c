//
// Created by sora on 2021/3/25.
//

#include <sys/socket.h>
#include <poll.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include "def.h"

#define SIZE 100
#define POLL_READ (POLLIN | POLLPRI)

static int handle_echo(int srvfd) {
    struct pollfd events[SIZE];
    char buf[MAXLINE];
    events[0].fd = srvfd;
    events[0].events = POLL_READ;

    events[1].fd = STDIN_FILENO;
    events[1].events = POLL_READ;

    while (1) {
        int ret = poll(events, 2, -1);
        if (ret == -1) {
            perror("poll error");
            return -1;
        } else if (ret == 0) {
            continue;
        } else {
            if ((events[0].revents & POLLIN) || (events[0].revents & POLLPRI)) {
                bzero(buf, MAXLINE);
                int nread = read(srvfd, buf, MAXLINE);
                if (nread == -1) {
                    perror("read error");
                    break;
                } else if (nread == 0) {
                    printf("server closed.");
                    break;
                }
                printf("%s", buf);
            }
            if ((events[1].revents & POLLIN) || (events[1].revents & POLLPRI)) {
                scanf("%[^\n]%*c", buf);
                strcat(buf, "\n");
                write(srvfd, buf, strlen(buf) + 1);
            }
        }
    }
}

int main(int argc, const char *argv[]) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket error");
        return -1;
    }
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, IPADDRESS, &addr.sin_addr);
    addr.sin_port = htons(PORT);
    if (connect(fd, (const struct sockaddr *) &addr, addrlen) == -1) {
        perror("connect error");
        return -1;
    }

    printf("connect to server %s:%d.\n", IPADDRESS, PORT);
    handle_echo(fd);

}