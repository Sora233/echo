//
// Created by sora on 2021/3/23.
//

#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include "def.h"

#define MAXLINE 1024

static void handle_echo(int fd) {
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
    fd_set *readfs = (fd_set *) malloc(sizeof(fd_set));
    char buf[MAXLINE];
    while (1) {
        FD_ZERO(readfs);
        FD_SET(fd, readfs);
        FD_SET(STDIN_FILENO, readfs);
        int retval = select(fd + 1, readfs, NULL, NULL, NULL);
        if (retval == -1) {
            fprintf(stderr, "select error: %s.\n", strerror(errno));
            break;
        } else if (retval == 0) {
            continue;
        } else {
            if (FD_ISSET(STDIN_FILENO, readfs)) {
                scanf("%[^\n]%*c", buf);
                strcat(buf, "\n");
                write(fd, buf, strlen(buf) + 1);
            } else if (FD_ISSET(fd, readfs)) {
                retval = read(fd, buf, sizeof(buf));
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

int main(int argc, char *argv[]) {
    int fd;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    bzero(&addr, addrlen);

    fd = socket(AF_INET, SOCK_STREAM, 0);
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, IPADDRESS, &addr.sin_addr);
    addr.sin_port = htons(PORT);

    if (connect(fd, (const struct sockaddr *) &addr, addrlen) < 0) {
        fprintf(stderr, "connect fail:%s\n", strerror(errno));
        return -1;
    }
    printf("connect to server %s:%d.\n", IPADDRESS, PORT);
    handle_echo(fd);
    return 0;
}