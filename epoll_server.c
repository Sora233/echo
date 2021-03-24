//
// Created by sora on 2021/3/24.
//

#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "def.h"

#define EPOLL_SIZE 1024
#define EPOLL_EVENT_SIZE 100

#define CONTEXT_FD(pev) (((echo_context*)((pev)->data.ptr))->fd)
#define CONTEXT_BUF(pev) (((echo_context*)((pev)->data.ptr))->buf)
#define CONTEXT(pev) ((echo_context*)((pev)->data.ptr))

typedef struct echo_context {
    int fd;
    char *buf;
} echo_context;

static int create_bind_socket(const char *ipaddr, int port) {
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
    inet_pton(AF_INET, ipaddr, &addr.sin_addr);
    addr.sin_port = htons(port);

    if (bind(fd, (const struct sockaddr *) &addr, len) == -1) {
        perror("bind error");
        return -1;
    }
    return fd;
}

static void epoll_operate(int epfd, int fd, int ctl, struct epoll_event *ev) {
    epoll_ctl(epfd, ctl, fd, ev);
}

static void epoll_add(int epfd, int fd, int state) {
    struct epoll_event ev;
    echo_context *ec = malloc(sizeof(echo_context));
    ec->fd = fd;
    ev.events = state;
    ev.data.ptr = ec;
    epoll_operate(epfd, fd, EPOLL_CTL_ADD, &ev);
}

static void epoll_delete(int epfd, struct epoll_event *ev) {
    if (ev->data.ptr == NULL) {
        return;
    }
    echo_context *ctx = ev->data.ptr;
    if (ctx->buf != NULL) {
        free(ctx->buf);
        ctx->buf = NULL;
    }
    epoll_operate(epfd, ctx->fd, EPOLL_CTL_DEL, ev);
}

static void epoll_modify(int epfd, struct epoll_event *ev) {
    epoll_operate(epfd, CONTEXT_FD(ev), EPOLL_CTL_MOD, ev);
}

static void handle_accept(int epfd, struct epoll_event *ev) {
    int clifd;
    struct sockaddr_in cliaddr;
    socklen_t cliaddrlen = sizeof(cliaddr);
    clifd = accept(CONTEXT_FD(ev), (struct sockaddr *) &cliaddr, &cliaddrlen);
    if (clifd == -1) {
        perror("accept error");
    } else {
        printf("new connection from %s:%d\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
        epoll_add(epfd, clifd, EPOLLIN);
    }
}

static void handle_read(int epfd, struct epoll_event *ev) {
    if (ev->data.ptr == NULL) {
        return;
    }
    echo_context *ctx = CONTEXT(ev);
    if (ctx->buf == NULL) {
        ctx->buf = (char *) malloc(sizeof(char) * MAXLINE);
    } else {
        bzero(ctx->buf, sizeof(char) * MAXLINE);
    }
    int nread = read(ctx->fd, ctx->buf, MAXLINE);
    int ret;
    struct sockaddr_in addr;
    ret = get_addrin_from_fd(ctx->fd, &addr);
    if (nread == -1) {
        perror("read error");
        close(ctx->fd);
        epoll_delete(epfd, ev);
    } else if (nread == 0) {
        if (ret == 0) {
            fprintf(stderr, "connection %s:%d closed\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
        } else {
            fprintf(stderr, "connection closed\n");
        }
        close(ctx->fd);
        epoll_delete(epfd, ev);
    } else {
        if (ret == 0) {
            printf("message from %s:%d : %s", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port), ctx->buf);
        } else {
            printf("message : %s", ctx->buf);
        }
        ev->events = EPOLLOUT;
        epoll_modify(epfd, ev);
    }
}

static void handle_write(int epfd, struct epoll_event *ev) {
    if (ev->data.ptr == NULL) {
        fprintf(stderr, "unknown echo");
        return;
    }
    echo_context *ctx = CONTEXT(ev);
    int nwrite = write(ctx->fd, ctx->buf, strlen(ctx->buf));
    if (nwrite == -1) {
        perror("write error");
        close(ctx->fd);
        epoll_delete(epfd, ev);
    } else {
        ev->events = EPOLLIN;
        epoll_modify(epfd, ev);
    }
}

static void do_epoll(int fd) {
    int ret, i;
    int epfd = epoll_create(EPOLL_SIZE);
    struct epoll_event events[EPOLL_EVENT_SIZE];
    epoll_add(epfd, fd, EPOLLIN);
    while (1) {
        ret = epoll_wait(epfd, events, EPOLL_EVENT_SIZE - 1, -1);
        if (ret == -1) {
            fprintf(stderr, "epoll_wait error: %s", strerror(errno));
            break;
        }
        for (i = 0; i < ret; i++) {
            if (events[i].events & EPOLLIN) {
                if (CONTEXT_FD(events + i) == fd) {
                    // new connection to accept
                    handle_accept(epfd, &events[i]);
                } else {
                    // ready to read
                    handle_read(epfd, &events[i]);
                }
            } else if (events[i].events & EPOLLOUT) {
                // echo data
                handle_write(epfd, &events[i]);
            }
        }
    }
    close(epfd);
}

int main(int argc, const char *argv[]) {
    int fd = create_bind_socket(IPADDRESS, PORT);
    if (fd == -1) {
        return -1;
    }
    listen(fd, LISTENQ);
    do_epoll(fd);
}
