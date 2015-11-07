#include <sys/select.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>

#ifndef __MYSOCKETSET__
#define __MYSOCKETSET__

#define GUEST 0
#define PASS 1
#define LOGIN 2
#define PORT 3
#define PASV 4

typedef struct {
    unsigned int count;
    int fdArray[FD_SETSIZE];
    int fdState[FD_SETSIZE];
    char username[FD_SETSIZE][10];
    struct sockaddr_in addrs[FD_SETSIZE];
    int pasvFdArray[FD_SETSIZE];
    char renamefile[FD_SETSIZE][257];
    int rename;
} socketSet;

/* add a socket file descriptor into the set fdset
    fdset: a socket file descriptor set
    sockfd: the descriptor of the socket that wanted to be added to the set
*/
void addSocketFd(socketSet *fdset, int sockfd) {
    int i;
    for (i = 0; i < fdset->count; ++i) {
        if (fdset->fdArray[i] == sockfd) {
            return;
        }
    }
    fdset->fdArray[i] = sockfd;
    fdset->fdState[i] = GUEST;
    fdset->pasvFdArray[i] = -1;
    memset(fdset->username[i], 0, sizeof(fdset->username[i]));
    memset(&fdset->addrs[i], 0, sizeof(fdset->addrs[i]));
    ++fdset->count;
}

/* remove a socket file descriptor from the set fdset
    fdset: a socket file descriptor set
    sockfd: the descriptor of the socket that wanted to be rmoved from the set
 */
void clearSocketFd(socketSet *fdset, int sockfd) {
    int i;
    for (i = 0; i < fdset->count; ++i) {
        if (fdset->fdArray[i] == sockfd) {
            --fdset->count;
            if (fdset->count > 0 && i < fdset->count) {
                fdset->fdArray[i] = fdset->fdArray[fdset->count];
                fdset->fdState[i] = fdset->fdState[fdset->count];
                fdset->pasvFdArray[i] = fdset->pasvFdArray[fdset->count];
                fdset->addrs[i] = fdset->addrs[fdset->count];
                memset(fdset->username[i], 0, sizeof(fdset->username[i]));
                strcpy(fdset->username[i], fdset->username[fdset->count]);
            }
            return;
        }
    }
}
#endif
