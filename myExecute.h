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
#include "mySocketSet.h"
#include "mySocket.h"

#define RESPONSE 256
#define FILEBUFFER 1024
#define BACKLOG 10
#define PATHLENGTH 1024

unsigned short controlPort = 21;
char rootPath[PATHLENGTH] = "/tmp";

/* check if the argument from the terminal is right
    argc: the number of argument
    argv: the arguments
    if any error, exit the program
 */
void checkTerminalArguments(int argc, char *argv[], unsigned short *port, char *rootPath) {
    int flag = 0;
    if (argc > 5 || argc == 2 || argc == 4) {// wrong argument, show usage
        flag = 1;
    }
    if (argc == 3) {
        if (strcmp(argv[1], "-root") != 0 && strcmp(argv[1], "-port") != 0) {
            flag = 1;
        }
    }
    else if (argc == 5) {
        if (strcmp(argv[1], "-root") != 0 && strcmp(argv[1], "-port") != 0) {
            flag = 1;
        }
        if (strcmp(argv[3], "-root") != 0 && strcmp(argv[3], "-port") != 0) {
            flag = 1;
        }
    }
    if (flag) {
        printf("usage: server [-port source_port] [-root /choose/your/root/path]\n");
        exit(1);
    }
    else {
        if (argc >= 3) {
            if (strcmp(argv[1], "-port") == 0) {
                if (checkDigit(argv[2]) == -1) {
                    printf("port's argument should be a number between 1024 and 65535\n");
                    exit(1);
                }
                int n = myAtoi(argv[2]);
                if (n >= 1024 && n <= 65535) {
                    *port = n;
                }
                else {
                    printf("port's argument should be a number between 1024 and 65535\n");
                    exit(1);
                }
            }
            else {
                struct stat dir;
                int err = stat(argv[2], &dir);
                if (err == -1) {
                    if (errno == ENOENT) {
                        printf("%s does not exist.\n", argv[2]);
                    }
                    else {
                        perror("stat");
                    }
                    exit(1);
                }
                else {
                    if (S_ISDIR(dir.st_mode)) {
                        strcpy(rootPath, argv[2]);
                    }
                    else {
                        printf("%s is not a directory.\n", argv[2]);
                        exit(1);
                    }
                }
            }
        }
        if (argc == 5) {
            if (strcmp(argv[3], "-port") == 0) {
                if (checkDigit(argv[4]) == -1) {
                    printf("port's argument should be a number between 1024 and 65535\n");
                    exit(1);
                }
                int n = myAtoi(argv[4]);
                if (n >= 1024 && n <= 65535) {
                    *port = n;
                }
                else {
                    printf("port's argument should be a number between 1024 and 65535\n");
                    exit(1);
                }
            }
            else {
                struct stat dir;
                int err = stat(argv[4], &dir);
                if (err == -1) {
                    if (errno == ENOENT) {
                        printf("%s does not exist.\n", argv[4]);
                    }
                    else {
                        perror("stat");
                    }
                    exit(1);
                }
                else {
                    if (S_ISDIR(dir.st_mode)) {
                        strcpy(rootPath, argv[4]);
                    }
                    else {
                        printf("%s is not a directory.\n", argv[4]);
                        exit(1);
                    }
                }
            }
        }
    }
    chdir(rootPath);
    char path[PATHLENGTH];
    getcwd(path, sizeof(path));
    memset(rootPath, 0, PATHLENGTH);
    strcpy(rootPath, path);
    printf("port: %d, root: %s\n", controlPort, rootPath);
}

/* execute the USER command
    parameter: possible parameter
    set: the current accepted client Socket set
    i: the index of the current socket in the set
 */
void executeUSER(char *parameter, socketSet *set, int i) {
    char response[RESPONSE];
    memset(response, 0, RESPONSE);
    int n;
    if (set->fdState[i] == GUEST) {
        set->fdState[i] = PASS;
        strcpy(response, USERlogin);
        strcat(response, parameter);
        strcat(response, ".\r\n");
        strcpy(set->username[i], parameter);
    }
    else if (set->fdState[i] == PASS) {
        strcpy(response, USERlogin);
        strcat(response, parameter);
        strcat(response, ".\r\n");
        strcpy(set->username[i], parameter);
    }
    else {
        strcpy(response, USERdeny);
    }
    n = Send(set->fdArray[i], response, strlen(response), 0);
    if (n == -1 || n == 0) {
        close(set->fdArray[i]);
        clearSocketFd(set, set->fdArray[i]);
    }
}

/* execute the XXXX command
    parameter: possible parameter
    set: the current accepted client Socket set
    i: the index of the current socket in the set
 */
void executePASS(char *parameter, socketSet * set, int i) {
    char response[RESPONSE];
    memset(response, 0, RESPONSE);
    int n;
    if (set->fdState[i] == GUEST || set->fdState[i] == LOGIN || set->fdState[i] == PORT || set->fdState[i] == PASV) {
        strcpy(response, PASSloginFirst);
    }
    else if (set->fdState[i] == PASS) {
        if (strcmp(set->username[i], "anonymous") == 0) {
            set->fdState[i] = 2;
            strcpy(response, LoginSuccess);
        }
        else {
            set->fdState[i] = 0;
            strcpy(response, PassFail);
        }
    }
    n = Send(set->fdArray[i], response, strlen(response), 0);
    if (n == -1 || n == 0) {
        close(set->fdArray[i]);
        clearSocketFd(set, set->fdArray[i]);
    }
}

void executeQUIT(char *parameter, socketSet *set, int i) {
    char response[RESPONSE];
    memset(response, 0, RESPONSE);
    strcpy(response, Goodbye);
    Send(set->fdArray[i], response, strlen(response), 0);
    close(set->fdArray[i]);
    clearSocketFd(set, set->fdArray[i]);
}

void executeSYST(char *parameter, socketSet *set, int i) {
    char response[RESPONSE];
    memset(response, 0, RESPONSE);
    int n;
    if (set->fdState[i] == LOGIN || set->fdState[i] == PORT || set->fdState[i] == PASV) {
        strcpy(response, SystmeInfo);
    }
    else {
        strcpy(response, LoginFirst);
    }
    n = Send(set->fdArray[i], response, strlen(response), 0);
    if (n == -1 || n == 0) {
        close(set->fdArray[i]);
        clearSocketFd(set, set->fdArray[i]);
    }
}

void executeTYPE(char *parameter, socketSet *set, int i) {
    char response[RESPONSE];
    memset(response, 0, RESPONSE);
    int n;
    if (set->fdState[i] == LOGIN || set->fdState[i] == PORT || set->fdState[i] == PASV) {
        if (strcmp(parameter, "I") == 0 || strcmp(parameter, "i") == 0 || strcmp(parameter, "A") == 0 || strcmp(parameter, "a") == 0) {
            strcpy(response, "200 Type set to ");
            strcat(response, parameter);
            strcat(response, ".\r\n");
        }
        else {
            strcpy(response, WrongArgument);
        }
    }
    else {
        strcpy(response, LoginFirst);
    }
    n = Send(set->fdArray[i], response, strlen(response), 0);
    if (n == -1 || n == 0) {
        close(set->fdArray[i]);
        clearSocketFd(set, set->fdArray[i]);
    }
}

void executePORT(char *parameter, socketSet *set, int i) {
    char response[RESPONSE];
    memset(response, 0, RESPONSE);
    int n;
    if (set->fdState[i] == LOGIN || set->fdState[i] == PORT || set->fdState[i] == PASV) {
        n = parseAddress(parameter, &set->addrs[i]);
        if (n == 0) {
            set->fdState[i] = PORT;
            strcpy(response, CmdSuccess);
        }
        else if (n == -1) {
            set->fdState[i] = LOGIN;
            strcpy(response, BadParameter);
        }
        else {
            set->fdState[i] = LOGIN;
            strcpy(response, WrongArgument);
        }
    }
    else {
        strcpy(response, LoginFirst);
    }
    n = Send(set->fdArray[i], response, strlen(response), 0);
    if (n == -1 || n == 0) {
        close(set->fdArray[i]);
        clearSocketFd(set, set->fdArray[i]);
    }
}

void executePASV(char *parameter, socketSet *set, int i) {
    char response[RESPONSE];
    memset(response, 0, RESPONSE);
    int n;
    if (set->fdState[i] == LOGIN || set->fdState[i] == PORT || set->fdState[i] == PASV) {
        struct sockaddr_in transAddr;
        //struct hostent *hostEntry;
        char *hostIp;
        //char hostname[255];
        char bufnum[11];
        unsigned short randPort = rand() % 45535 + 20000;
        memset(&transAddr, 0, sizeof(transAddr));
        transAddr.sin_family = AF_INET;
        transAddr.sin_port= htons(randPort);
        transAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        set->pasvFdArray[i] = Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        strcpy(response, WrongArgument);
        if (set->pasvFdArray[i] != -1) {
            if (Bind(set->pasvFdArray[i], (struct sockaddr*)&transAddr, sizeof(transAddr)) != -1) {
                if (Listen(set->pasvFdArray[i], BACKLOG) != -1) {
                    //gethostname(hostname, 255);
                    //hostEntry = gethostbyname(hostname);
                    //hostIp = inet_ntoa(*(struct in_addr *)*hostEntry->h_addr_list);
                    hostIp = getPcIp();
                    convertIP(hostIp);
                    memset(response, 0, sizeof(response));
                    strcpy(response, EnterPasv);
                    strcat(response, hostIp);
                    strcat(response, ",");
                    myItoa(bufnum, randPort / 256);
                    strcat(response, bufnum);
                    strcat(response, ",");
                    myItoa(bufnum, randPort % 256);
                    strcat(response, bufnum);
                    strcat(response, ")\r\n");
                    set->fdState[i] = PASV;
                }
            }
        }
    }
    else {
        strcpy(response, LoginFirst);
    }
    n = Send(set->fdArray[i], response, strlen(response), 0);
    if (n == -1 || n == 0) {
        close(set->fdArray[i]);
        clearSocketFd(set, set->fdArray[i]);
    }
}

void executeRETR(char *parameter, socketSet *set, int i) {
    char response[RESPONSE];
    memset(response, 0, RESPONSE);
    int n = 0;
    if (set->fdState[i] == LOGIN || set->fdState[i] == PORT || set->fdState[i] == PASV) {
        if (parameter[0] == '\0') {
            strcpy(response, BadParameter);
        }
        else if (set->fdState[i] == LOGIN) {
            strcpy(response, NoPort);
        }
        else {
            int sockfd;
            if (set->fdState[i] == PORT) {
                sockfd = Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                n = Connect(sockfd, (struct sockaddr*)&set->addrs[i], sizeof(set->addrs[i]));
            }
            else {
                sockfd = Accept(set->pasvFdArray[i], NULL, NULL);
            }
            if (sockfd != -1 && n != -1) {
                struct stat fileInfo;
                char buf[101], port[6];
                n = stat(parameter, &fileInfo);
                if (n == -1) {
                    // ...
                }
                myItoa(buf, fileInfo.st_size);
                FILE *fp;
                if (parameter[0] == '/') { // absolute path
                    char fileName[PATHLENGTH * 2];
                    strcpy(fileName, rootPath);
                    strcat(fileName, parameter);
                    fp = fopen(fileName, "rb");
                }
                else { // relative path
                    fp = fopen(parameter, "rb");
                }
                if (fp && S_ISREG(fileInfo.st_mode)) {
                    if (set->fdState[i] == PORT) {
                        strcpy(response, OpenDataConnection);
                        strcat(response, parameter);//FILENAME
                        strcat(response, " (");
                        strcat(response, buf);//bytes
                        strcat(response, " bytes)\r\n");
                    }
                    else {
                        struct sockaddr_in peerAddr;
                        socklen_t addr_size = sizeof(struct sockaddr_in);
                        int res = Getpeername(sockfd, (struct sockaddr *)&peerAddr, &addr_size);
                        if (res != -1) {
                            strcpy(response, DataConnectionAccept);
                            strcat(response, inet_ntoa(peerAddr.sin_addr));//IP
                            strcat(response, ":");
                            myItoa(port, ntohs(peerAddr.sin_port));// port
                            strcat(response, port);
                            strcat(response, "; transfer starting for ");
                            strcat(response, parameter);//FILENAME
                            strcat(response, " (");
                            strcat(response, buf);//bytes
                            strcat(response, " bytes)\r\n");
                        }
                        else {
                            strcpy(response, DataConnectionAccept);
                            strcat(response, "?.?.?.?");//IP
                            strcat(response, ":");
                            strcat(response, "????");
                            strcat(response, "; transfer starting for ");
                            strcat(response, parameter);//FILENAME
                            strcat(response, " (");
                            strcat(response, buf);//bytes
                            strcat(response, " bytes)\r\n");
                        }
                    }
                    n = Send(set->fdArray[i], response, strlen(response), 0);
                    memset(response, 0, sizeof(response));
                    if (n == 0 || n == -1) {
                        close(sockfd);
                        close(set->fdArray[i]);
                        clearSocketFd(set, set->fdArray[i]);
                    }
                    char fileBuffer[FILEBUFFER];
                    do {
                        n = fread(fileBuffer, 1, sizeof(fileBuffer), fp);
                        if (ferror(fp)) {
                            strcpy(response, AccessFileErr);
                            break;
                        }
                        if (Send(sockfd, fileBuffer, n, 0) == -1) {
                            close(sockfd);
                            close(set->fdArray[i]);
                            clearSocketFd(set, set->fdArray[i]);
                            break;
                        }
                    } while(n != 0);
                    if (!ferror(fp)) {
                        strcpy(response, FileSendOk);
                    }
                    close(sockfd);
                }
                else {
                    strcpy(response, "550 \"");
                    strcat(response, parameter);
                    strcat(response, "\"");
                    if (errno == ENOENT) {  // file doesn't exist
                        strcat(response, NoFile);
                    }
                    else if (errno == EACCES) { // permission denied
                        strcat(response, PermissionDeny);
                    }
                }
                fclose(fp);
            }
            else {
                strcpy(response, NoDataConnection);
            }
            set->fdState[i] = LOGIN;
        }
    }
    else {
        strcpy(response, LoginFirst);
    }
    n = Send(set->fdArray[i], response, strlen(response), 0);
    if (n == -1 || n == 0) {
        close(set->fdArray[i]);
        clearSocketFd(set, set->fdArray[i]);
    }
}

void executeSTOR(char *parameter, socketSet *set, int i) {
    char response[RESPONSE];
    memset(response, 0, RESPONSE);
    int n = 0;
    if (set->fdState[i] == LOGIN || set->fdState[i] == PORT || set->fdState[i] == PASV) {
        if (parameter[0] == '\0') {
            strcpy(response, BadParameter);
        }
        else if (set->fdState[i] == LOGIN) {
            strcpy(response, NoPort);
        }
        else {
            int sockfd;
            if (set->fdState[i] == PORT) {
                sockfd = Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                n = Connect(sockfd, (struct sockaddr*)&set->addrs[i], sizeof(set->addrs[i]));
            }
            else {
                sockfd = Accept(set->pasvFdArray[i], NULL, NULL);
            }
            if (sockfd != -1 && n != -1) {
                char port[6];
                char filename[256];
                getFilename(parameter, filename);
                FILE *fp = fopen(filename, "wb");
                if (fp) {
                    if (set->fdState[i] == PORT) {
                        strcpy(response, OpenDataConnection);
                        strcat(response, parameter);//FILENAME
                        strcat(response, "\r\n");
                    }
                    else {
                        struct sockaddr_in peerAddr;
                        socklen_t addr_size = sizeof(struct sockaddr_in);
                        int res = Getpeername(sockfd, (struct sockaddr *)&peerAddr, &addr_size);
                        if (res != -1) {
                            strcpy(response, DataConnectionAccept);
                            strcat(response, inet_ntoa(peerAddr.sin_addr));//IP
                            strcat(response, ":");
                            myItoa(port, ntohs(peerAddr.sin_port));// port
                            strcat(response, port);
                            strcat(response, "; transfer starting for ");
                            strcat(response, parameter);//FILENAME
                            strcat(response, "\r\n");
                        }
                        else {
                            strcpy(response, DataConnectionAccept);
                            strcat(response, "?.?.?.?");//IP
                            strcat(response, ":");
                            strcat(response, "????");
                            strcat(response, "; transfer starting for ");
                            strcat(response, parameter);//FILENAME
                            strcat(response, "\r\n");
                        }
                    }
                    n = Send(set->fdArray[i], response, strlen(response), 0);
                    memset(response, 0, sizeof(response));
                    if (n == 0 || n == -1) {
                        close(sockfd);
                        close(set->fdArray[i]);
                        clearSocketFd(set, set->fdArray[i]);
                    }
                    char fileBuffer[FILEBUFFER];
                    do {
                        n = Recv(sockfd, fileBuffer, sizeof(fileBuffer), 0);
                        if (n == -1) {
                            close(sockfd);
                            close(set->fdArray[i]);
                            clearSocketFd(set, set->fdArray[i]);
                            break;
                        }
                        fwrite(fileBuffer, n, 1, fp);
                        if (ferror(fp)) {
                            strcpy(response, AccessFileErr);
                            break;
                        }
                    } while(n != 0);
                    if (!ferror(fp)) {
                        strcpy(response, FileSendOk);
                    }
                    close(sockfd);
                }
                else {
                    strcpy(response, CannotStor);
                }
                fclose(fp);
            }
            else {
                strcpy(response, NoDataConnection);
            }
            set->fdState[i] = LOGIN;
        }
    }
    else {
        strcpy(response, LoginFirst);
    }
    n = Send(set->fdArray[i], response, strlen(response), 0);
    if (n == -1 || n == 0) {
        close(set->fdArray[i]);
        clearSocketFd(set, set->fdArray[i]);
    }
}

void executePWD(char *parameter, socketSet *set, int i) {
    char response[RESPONSE];
    memset(response, 0, RESPONSE);
    int n;
    if (set->fdState[i] == LOGIN || set->fdState[i] == PORT || set->fdState[i] == PASV) {
        char currentPath[PATHLENGTH];
        getcwd(currentPath, sizeof(currentPath));
        if (strcmp(currentPath, rootPath) == 0) {
            sprintf(response, PwdReply, "/");
        }
        else {
            if (strlen(rootPath) == 1) {
                sprintf(response, CwdReply, currentPath);
            }
            else {
                sprintf(response, CwdReply, &currentPath[strlen(rootPath)]);
            }
        }
    }
    else {
        strcpy(response, LoginFirst);
    }
    n = Send(set->fdArray[i], response, strlen(response), 0);
    if (n == -1 || n == 0) {
        close(set->fdArray[i]);
        clearSocketFd(set, set->fdArray[i]);
    }
}

void executeLIST(char *parameter, socketSet *set, int i) {
    char response[RESPONSE];
    memset(response, 0, RESPONSE);
    int n = 0;
    if (set->fdState[i] == LOGIN || set->fdState[i] == PORT || set->fdState[i] == PASV) {
        if (set->fdState[i] == LOGIN) {
            strcpy(response, NoPort);
        }
        else {
            int sockfd;
            if (set->fdState[i] == PORT) {
                sockfd = Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                n = Connect(sockfd, (struct sockaddr*)&set->addrs[i], sizeof(set->addrs[i]));
            }
            else {
                sockfd = Accept(set->pasvFdArray[i], NULL, NULL);
            }
            if (sockfd != -1 && n != -1) {
                char port[6];
                if (set->fdState[i] == PORT) {
                    strcpy(response, OpenDataConnection);
                    strcat(response, "directory list.\r\n");
                }
                else {
                    struct sockaddr_in peerAddr;
                    socklen_t addr_size = sizeof(struct sockaddr_in);
                    int res = Getpeername(sockfd, (struct sockaddr *)&peerAddr, &addr_size);
                    if (res != -1) {
                        strcpy(response, DataConnectionAccept);
                        strcat(response, inet_ntoa(peerAddr.sin_addr));//IP
                        strcat(response, ":");
                        myItoa(port, ntohs(peerAddr.sin_port));// port
                        strcat(response, port);
                        strcat(response, "; transfer starting.\r\n");
                    }
                    else {
                        strcpy(response, DataConnectionAccept);
                        strcat(response, "?.?.?.?");//IP
                        strcat(response, ":");
                        strcat(response, "????");
                        strcat(response, "; transfer starting.\r\n");
                    }
                }
                n = Send(set->fdArray[i], response, strlen(response), 0);
                memset(response, 0, sizeof(response));
                if (n == 0 || n == -1) {
                    close(sockfd);
                    close(set->fdArray[i]);
                    clearSocketFd(set, set->fdArray[i]);
                }
                int p = dup(1);
                dup2(sockfd, 1);
                system("ls -l");
                dup2(p, 1);
                close(sockfd);
                strcpy(response, TransferOk);
            }
            else {
                strcpy(response, NoDataConnection);
            }
            set->fdState[i] = LOGIN;
        }
    }
    else {
        strcpy(response, LoginFirst);
    }
    n = Send(set->fdArray[i], response, strlen(response), 0);
    if (n == -1 || n == 0) {
        close(set->fdArray[i]);
        clearSocketFd(set, set->fdArray[i]);
    }
}

void executeCWD(char *parameter, socketSet *set, int i) {
    char response[RESPONSE];
    memset(response, 0, RESPONSE);
    int n;
    if (set->fdState[i] == LOGIN || set->fdState[i] == PORT || set->fdState[i] == PASV) {
        char path[PATHLENGTH];
        if (parameter[0] == '/' && rootPath[1] != '\0') {
            strcpy(path, rootPath);
            strcat(path, parameter);
        }
        else if (parameter[0] == '\0') {
            strcpy(path, ".");
        }
        else {
            strcpy(path, parameter);
        }
        if (strcmp(parameter, "..") == 0) {
            char cwd[PATHLENGTH];
            getcwd(cwd, sizeof(cwd));
            if (strcmp(cwd, rootPath) == 0) {
                n = 0;
            }
            else {
                n = chdir(path);
            }
        }
        else {
            n = chdir(path);
        }
        if (n == -1) {
            sprintf(response, CwdFail, parameter);
        }
        else {
            char currentPath[PATHLENGTH];
            getcwd(currentPath, sizeof(currentPath));
            if (strcmp(currentPath, rootPath) == 0) {
                sprintf(response, CwdReply, "/");
            }
            else {
                if (strlen(rootPath) == 1) {
                    sprintf(response, CwdReply, currentPath);
                }
                else {
                    sprintf(response, CwdReply, &currentPath[strlen(rootPath)]);
                }
            }
        }
    }
    else {
        strcpy(response, LoginFirst);
    }
    n = Send(set->fdArray[i], response, strlen(response), 0);
    if (n == -1 || n == 0) {
        close(set->fdArray[i]);
        clearSocketFd(set, set->fdArray[i]);
    }
}

void executeDELE(char *parameter, socketSet *set, int i) {
    char response[RESPONSE];
    memset(response, 0, RESPONSE);
    int n;
    if (set->fdState[i] == LOGIN || set->fdState[i] == PORT || set->fdState[i] == PASV) {
        if (parameter[0] == '\0') {
            strcpy(response, BadParameter);
        }
        else {
            char filename[PATHLENGTH];
            if (parameter[0] == '/' && rootPath[1] != '\0') {
                strcpy(filename, rootPath);
                strcat(filename, parameter);
            }
            else {
                char cwd[PATHLENGTH];
                getcwd(cwd, sizeof(cwd));
                strcpy(filename, cwd);
                strcat(filename, "/");
                strcat(filename, parameter);
            }
            n = remove(filename);
            if (n == -1) {
                strcpy(response, "550 \"");
                strcat(response, parameter);
                strcat(response, "\"");
                strcat(response, NoFile);
            }
            else {
                if (strlen(rootPath) == 1) {
                    sprintf(response, DeleFile, filename);
                }
                else {
                    sprintf(response, DeleFile, &filename[strlen(rootPath)]);
                }
            }
        }
    }
    else {
        strcpy(response, LoginFirst);
    }
    n = Send(set->fdArray[i], response, strlen(response), 0);
    if (n == -1 || n == 0) {
        close(set->fdArray[i]);
        clearSocketFd(set, set->fdArray[i]);
    }
}

void executeMKD(char *parameter, socketSet *set, int i) {
    char response[RESPONSE];
    memset(response, 0, RESPONSE);
    int n;
    if (set->fdState[i] == LOGIN || set->fdState[i] == PORT || set->fdState[i] == PASV) {
        if (parameter[0] == '\0') {
            strcpy(response, BadParameter);
        }
        else {
            char dirname[PATHLENGTH];
            if (parameter[0] == '/' && rootPath[1] != '\0') {
                strcpy(dirname, rootPath);
                strcat(dirname, parameter);
            }
            else {
                char cwd[PATHLENGTH];
                getcwd(cwd, sizeof(cwd));
                strcpy(dirname, cwd);
                strcat(dirname, "/");
                strcat(dirname, parameter);
            }
            n = mkdir(dirname, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
            if (n == -1) {
                sprintf(response, MkdFail, parameter);
            }
            else {
                if (strlen(rootPath) == 1) {
                    sprintf(response, MkdReply, dirname);
                }
                else {
                    sprintf(response, MkdReply, &dirname[strlen(rootPath)]);
                }
            }
        }
    }
    else {
        strcpy(response, LoginFirst);
    }
    n = Send(set->fdArray[i], response, strlen(response), 0);
    if (n == -1 || n == 0) {
        close(set->fdArray[i]);
        clearSocketFd(set, set->fdArray[i]);
    }
}

void executeRMD(char *parameter, socketSet *set, int i) {
    char response[RESPONSE];
    memset(response, 0, RESPONSE);
    int n;
    if (set->fdState[i] == LOGIN || set->fdState[i] == PORT || set->fdState[i] == PASV) {
        if (parameter[0] == '\0') {
            strcpy(response, BadParameter);
        }
        else {
            char dirname[PATHLENGTH];
            if (parameter[0] == '/' && rootPath[1] != '\0') {
                strcpy(dirname, rootPath);
                strcat(dirname, parameter);
            }
            else {
                char cwd[PATHLENGTH];
                getcwd(cwd, sizeof(cwd));
                strcpy(dirname, cwd);
                strcat(dirname, "/");
                strcat(dirname, parameter);
            }
            n = rmdir(dirname);
            if (n == -1) {
                sprintf(response, RmdFail, parameter);
            }
            else {
                if (strlen(rootPath) == 1) {
                    sprintf(response, RmdReply, dirname);
                }
                else {
                    sprintf(response, RmdReply, &dirname[strlen(rootPath)]);
                }
            }
        }
    }
    else {
        strcpy(response, LoginFirst);
    }
    n = Send(set->fdArray[i], response, strlen(response), 0);
    if (n == -1 || n == 0) {
        close(set->fdArray[i]);
        clearSocketFd(set, set->fdArray[i]);
    }
}

void executeRNFR(char *parameter, socketSet *set, int i) {
    char response[RESPONSE];
    memset(response, 0, RESPONSE);
    int n;
    if (set->fdState[i] == LOGIN || set->fdState[i] == PORT || set->fdState[i] == PASV) {
        if (parameter[0] == '\0') {
            strcpy(response, BadParameter);
        }
        else {
            char dirname[PATHLENGTH];
            if (parameter[0] == '/' && rootPath[1] != '\0') {
                strcpy(dirname, rootPath);
                strcat(dirname, parameter);
            }
            else {
                char cwd[PATHLENGTH];
                getcwd(cwd, sizeof(cwd));
                strcpy(dirname, cwd);
                strcat(dirname, "/");
                strcat(dirname, parameter);
            }
            struct stat fileInfo;
            n = stat(dirname, &fileInfo);
            if (n == -1) {
                if (errno == EACCES) {
                    sprintf(response, RnfrFail2, parameter);
                }
                else {
                    strcpy(response, RnfrFail);
                }
            }
            else {
                set->rename = 1;
                strcpy(set->renamefile[i], dirname);
                if (strlen(rootPath) == 1) {
                    sprintf(response, RnfrReply, dirname);
                }
                else {
                    sprintf(response, RnfrReply, &dirname[strlen(rootPath)]);
                }
            }
        }
    }
    else {
        strcpy(response, LoginFirst);
    }
    n = Send(set->fdArray[i], response, strlen(response), 0);
    if (n == -1 || n == 0) {
        close(set->fdArray[i]);
        clearSocketFd(set, set->fdArray[i]);
    }
}

void executeRNTO(char *parameter, socketSet *set, int i) {
    char response[RESPONSE];
    memset(response, 0, RESPONSE);
    int n;
    if (set->fdState[i] == LOGIN || set->fdState[i] == PORT || set->fdState[i] == PASV) {
        if (parameter[0] == '\0') {
            strcpy(response, BadParameter);
        }
        else if (set->rename != 1) {
            strcpy(response, BadSequence);
        }
        else {
            set->rename = 0;
            char dirname[PATHLENGTH];
            if (parameter[0] == '/' && rootPath[1] != '\0') {
                strcpy(dirname, rootPath);
                strcat(dirname, parameter);
            }
            else {
                char cwd[PATHLENGTH];
                getcwd(cwd, sizeof(cwd));
                strcpy(dirname, cwd);
                strcat(dirname, "/");
                strcat(dirname, parameter);
            }
            n = rename(set->renamefile[i], dirname);
            if (n == -1) {
                sprintf(response, RntoFail, set->renamefile[i]);
            }
            else {
                if (strlen(rootPath) == 1) {
                    sprintf(response, RntoReply, set->renamefile[i], dirname);
                }
                else {
                    sprintf(response, RntoReply, &set->renamefile[i][strlen(rootPath)], &dirname[strlen(rootPath)]);
                }
            }
        }
    }
    else {
        strcpy(response, LoginFirst);
    }
    n = Send(set->fdArray[i], response, strlen(response), 0);
    if (n == -1 || n == 0) {
        close(set->fdArray[i]);
        clearSocketFd(set, set->fdArray[i]);
    }
}
/* execute command from client
    cmd: the command to execute
    parameter: possible parameter
    set: the current accepted client Socket set
    i: the index of the current socket in the set
 */
void executeCommand(char *cmd, char *parameter, socketSet *set, int i) {
    if (strcmp(cmd, "USER") == 0) {
        executeUSER(parameter, set, i);
    }
    else if (strcmp(cmd, "PASS") == 0) {
        executePASS(parameter, set, i);
    }
    else if (strcmp(cmd, "QUIT") == 0) {
        executeQUIT(parameter, set, i);
    }
    else if (strcmp(cmd, "SYST") == 0) {
        executeSYST(parameter, set, i);
    }
    else if (strcmp(cmd, "TYPE") == 0) {
        executeTYPE(parameter, set, i);
    }
    else if (strcmp(cmd, "PORT") == 0) {
        executePORT(parameter, set, i);
    }
    else if (strcmp(cmd, "PASV") == 0) {
        executePASV(parameter, set, i);
    }
    else if (strcmp(cmd, "RETR") == 0) {
        executeRETR(parameter, set, i);
    }
    else if (strcmp(cmd, "STOR") == 0) {
        executeSTOR(parameter, set, i);
    }
    else if (strcmp(cmd ,"PWD") == 0) {
        executePWD(parameter, set, i);
    }
    else if (strcmp(cmd, "LIST") == 0) {
        executeLIST(parameter, set, i);
    }
    else if (strcmp(cmd, "CWD") == 0) {
        executeCWD(parameter, set, i);
    }
    else if (strcmp(cmd, "CDUP") == 0) {
        executeCWD("..", set, i);
    }
    else if (strcmp(cmd, "DELE") == 0) {
        executeDELE(parameter, set, i);
    }
    else if (strcmp(cmd, "MKD") == 0) {
        executeMKD(parameter, set, i);
    }
    else if (strcmp(cmd, "RMD") == 0) {
        executeRMD(parameter, set, i);
    }
    else if (strcmp(cmd, "RNFR") == 0) {
        executeRNFR(parameter, set, i);
    }
    else if (strcmp(cmd, "RNTO") == 0) {
        executeRNTO(parameter, set, i);
    }
    else {
        char response[RESPONSE] = {'\0'};
        strcpy(response, UnknownCommand);
        int n = Send(set->fdArray[i], response, strlen(response), 0);
        if (n == -1 || n == 0) {
            close(set->fdArray[i]);
            clearSocketFd(set, set->fdArray[i]);
        }
    }
}
