#include <sys/socket.h>
#include <stdio.h>
#include <errno.h>

#ifndef __MYSOCKET__
#define __MYSOCKET__
/* All the following functions are just socket APIs that
   are added error print.
 */
int Socket(int domain, int type, int protocol) {
    int sockfd = socket(domain, type, protocol);
    if (sockfd == -1) {
        printf("Error socket(): %s(%d)\n", strerror(errno), errno);
    }
    return sockfd;
}

int Bind(int sockfd, const struct sockaddr* address, socklen_t address_len) {
    int result = bind(sockfd, address, address_len);
    if (result == -1) {
        printf("Error bind(): %s(%d)\n", strerror(errno), errno);
    }
    return result;
}

int Listen(int sockfd, int backlog) {
    int result = listen(sockfd, backlog);
    if (result == -1) {
        printf("Error listen(): %s(%d)\n", strerror(errno), errno);
    }
    return result;
}

int Accept(int sockfd, struct sockaddr *addr, socklen_t *len) {
    int result = accept(sockfd, addr, len);
    if (result == -1) {
        printf("Error accept(): %s(%d)\n", strerror(errno), errno);
    }
    return result;
}

int Select(int maxfd, fd_set *readfd, fd_set *writefd, fd_set *exceptfd, struct timeval *tv) {
    int result = select(maxfd, readfd, writefd, exceptfd, tv);
    if (result == -1) {
        printf("Error select(): %s(%d)\n", strerror(errno), errno);
    }
    return result;
}

int Recv(int sockfd, void *buf, int len, int flags) {
    int result = recv(sockfd, buf, len, flags);
    if (result == -1) {
        printf("Error recv(): %s(%d)\n", strerror(errno), errno);
    }
    return result;
}

int Send(int sockfd, void *buf, int len, int flags) {
    int result = send(sockfd, buf, len, flags);
    if (result == -1) {
        printf("Error send(): %s(%d)\n", strerror(errno), errno);
    }
    return result;
}

int Connect(int sockfd, struct sockaddr* addr, socklen_t len) {
    int result = connect(sockfd, addr, len);
    if (result == -1) {
        printf("Error connect(): %s(%d)\n", strerror(errno), errno);
    }
    return result;
}

int Getpeername(int sockfd, struct sockaddr *addr, socklen_t *len) {
    int result = getpeername(sockfd, addr, len);
    if (result == -1) {
        printf("Error getpeername(): %s(%d)\n", strerror(errno), errno);
    }
    return result;
}
#endif