#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "mySocket.h"
#include "myConst.h"
#include "myStdlib.h"
#include "myExecute.h"
#include "mySocketSet.h"

#define BACKLOG 10

typedef struct {
    char cmd[101];
    char parameter[256];
    socketSet *set;
    int i;
} threadArg;

void *threadWrapExec(void *arg);

int main(int argc, char *argv[]) {
    checkTerminalArguments(argc, argv, &controlPort, rootPath);
    int listenfd, connectfd;
    char message[1024], cmd[10], parameter[256], response[256];
    struct sockaddr_in addr;
    fd_set readfd;
    socketSet connectfds;
    int maxfd;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    connectfds.count = 0;
    listenfd = Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenfd == -1) {
        return 1;
    }
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(controlPort);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (Bind(listenfd, (struct sockaddr*)& addr, sizeof(addr)) == -1) {
        return 1;
    }
    if (Listen(listenfd, BACKLOG) == -1) {
        return 1;
    }
    while (1) {
        FD_ZERO(&readfd);
        FD_SET(listenfd, &readfd);
        int i;
        maxfd = listenfd;
        for (i = 0; i < connectfds.count; ++i) {
            FD_SET(connectfds.fdArray[i], &readfd);
            maxfd = maxfd > connectfds.fdArray[i] ? maxfd : connectfds.fdArray[i];
        }
        if (Select(maxfd + 1, &readfd, NULL, NULL, &tv) > 0) {
            if (FD_ISSET(listenfd, &readfd) && connectfds.count < FD_SETSIZE) {
                connectfd = Accept(listenfd, NULL, NULL);
                if (connectfd != -1) {
                    addSocketFd(&connectfds, connectfd);
                    strcpy(response, loginReply);
                    Send(connectfd, response, strlen(response), 0);
                }
            }
            for (i = 0; i < connectfds.count; ++i) {
                memset(response, 0, sizeof(response));
                if (FD_ISSET(connectfds.fdArray[i], &readfd)) {
                    int n = Recv(connectfds.fdArray[i], message, sizeof(message), MSG_DONTWAIT);
                    if (n == -1 || n == 0) {
                        close(connectfds.fdArray[i]);
                        FD_CLR(connectfds.fdArray[i], &readfd);
                        clearSocketFd(&connectfds, connectfds.fdArray[i]);
                    }
                    else {
                        if (n <= 2) { // ignore empty messages
                            continue;
                        }
                        message[n - 2] = 0;
                        memset(cmd, 0, sizeof(cmd));
                        memset(parameter, 0, sizeof(parameter));
                        memset(response, 0, sizeof(parameter));
                        parseCommand(message, cmd, parameter);
                        pthread_t tid;
                        threadArg arg;
                        arg.i = i;
                        arg.set = &connectfds;
                        memset(arg.cmd, 0, sizeof(arg.cmd));
                        strcpy(arg.cmd, cmd);
                        memset(arg.parameter, 0, sizeof(arg.parameter));
                        strcpy(arg.parameter, parameter);
                        pthread_create(&tid, NULL, threadWrapExec, &arg);
                    }
                }
            }
        }
    }
    close(listenfd);
    return 0;
}

/* wrap the executeCommand function in a thread
    arg: the pointer that point to the argument of the function executeCommand
 */
void *threadWrapExec(void *arg) {
    threadArg *t = (threadArg*)arg;
    executeCommand(t->cmd, t->parameter, t->set, t->i);
    return NULL;
}
