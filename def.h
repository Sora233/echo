//
// Created by sora on 2021/3/25.
//

#ifndef ECHO_SELECT_DEF_H
#define ECHO_SELECT_DEF_H

#define IPADDRESS "127.0.0.1"
#define PORT 8787
#define MAXLINE     1024
#define LISTENQ 5

static int get_addrin_from_fd(int fd, struct sockaddr_in *addr) {
    socklen_t addrlen = sizeof(struct sockaddr_in);
    bzero(addr, addrlen);
    if (getpeername(fd, (struct sockaddr *) addr, &addrlen) == -1) {
        return -1;
    }
    return 0;
}

#endif //ECHO_SELECT_DEF_H
