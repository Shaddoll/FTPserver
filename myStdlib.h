#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ifaddrs.h>

#ifndef __MYSTDLIB__
#define __MYSTDLIB__
/* parse the message that clients send
   message: the message recv
   cmd: the command
   parameter: the parameter of the message (if any)
   return 1 if no parameter, 2 parameter
 */
int parseCommand(char *message, char *cmd, char *parameter) {
    int len = strlen(message);
    int i, j;
    for (i = 0; i < len; ++i) {
        if (message[i] == ' ') {
            break;
        }
        cmd[i] = toupper(message[i]);
    }
    cmd[i++] = '\0';
    for (j = 0; i < len; ++j, ++i) {
        parameter[j] = message[i];
    }
    parameter[j] = '\0';
    if (j > 0) {
        return 2;
    }
    return 1;
}


/* parse the socket address of the paradigm(h1, h2, h3, h4, p1, p2)
   , validate it and store it
    parameter: the socket address in the paradigm(h1, h2, h3, h4, p1, p2)
    addr: place to store the translated ip address and port
    return -1 if it cannot be translated,
    return -2 if if the port number is less than 1024
    return 0 if succeed.
 */
int parseAddress(char *parameter, struct sockaddr_in *addr) {
    int len = strlen(parameter);
    int i, j;
    char ipAddr[16] = {0};
    char str[6][4] = {{'\0'}};
    unsigned short port = 0;
    int comma = 0;
    for (i = 0, j = 0; i < len; ++i) {
        if (comma >= 6) {
            return -1;
        }
        if (parameter[i] >= '0' && parameter[i] <='9') {
            str[comma][j++] = parameter[i];
        }
        else if (parameter[i] == ',') {
            str[comma][j] = '\0';
            j = 0;
            ++comma;
        }
        else {
            return -1;
        }
    }
    for (i = 0; i < 6; ++i) {
        int temp = 0;
        int strl = strlen(str[i]);
        if (strl == 0) {
            return -1;
        }
        for (j = 0; j < strl; ++j) {
            temp = temp * 10 + str[i][j] - '0';
        }
        if (temp >= 256) {
            return -1;
        }
    }
    for (i = 0; i < 4; ++i) {
        strcat(ipAddr, str[i]);
        strcat(ipAddr, ".");
    }
    ipAddr[strlen(ipAddr) - 1] = '\0';
    for (i = 4; i < 6; ++i) {
        int temp = 0;
        int strl = strlen(str[i]);
        for (j = 0; j < strl; ++j) {
            temp = temp * 10 + str[i][j] - '0';
        }
        port = port * 256 + temp;
    }
    if (port < 1024) {
        return -2;
    }
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    inet_pton(AF_INET, ipAddr, &addr->sin_addr);
    return 0;
}

/* replace the "." in the ip address with ","
    ip: the ip address
 */
void convertIP(char *ip) {
    int len = strlen(ip);
    int i;
    for (i = 0; i < len; ++i) {
        if (ip[i] == '.') {
            ip[i] = ',';
        }
    }
}

/* convert an integer to string
    buf: place to store the derived string
    i: the number to be converted
 */
void myItoa(char *buf, int i) {
    char buf2[11];
    int j = 0, k = 0;
    do {
        buf2[j++] = i % 10 + '0';
        i /= 10;
    } while (i != 0);
    --j;
    while (j >= 0) {
        buf[k++] = buf2[j--];
    }
    buf[k] = '\0';
}
/* convert a string to an Integer
    str: string to be converted
    return the integer
 */
int myAtoi(char *str) {
    int result = 0;
    int len = strlen(str);
    int i;
    for(i = 0; i < len; ++i) {
        result = result * 10 + str[i] - '0';
    }
    return result;
}

/* check if a string only has digits
    str: sthe string to be checked
    return -1 if not, 0 if it does.
 */
int checkDigit(char *str) {
    int len = strlen(str);
    int i;
    for (i = 0; i < len; ++i) {
        if (str[i] <'0' || str[i] > '9') {
            return -1;
        }
    }
    return 0;
}

/* get a file's name from the path
    parameter: filename including path
    filename: filename to be stored
 */
void getFilename(char *parameter, char *filename) {
    int len = strlen(parameter);
    int i;
    for (i = len - 1; i >= 0; --i) {
        if (parameter[i] == '/') {
            strcpy(filename, &parameter[i + 1]);
            return;
        }
    }
    strcpy(filename, parameter);
}

/* get code number of the server's message
    message: the message from the server
    return the code
 */
int getMessageCode(char *message) {
    int code = 0;
    char buffer[8192] = {'\0'};
    strcpy(buffer, message);
    int len = strlen(buffer);
    int i;
    for (i = 0; i < len; ++i) {
        if (buffer[i] == ' ') {
            buffer[i] = '\0';
        }
    }
    code = myAtoi(buffer);
    return code;
}
/* get your host's own ip address
    return the string of ip address
 */
char* getPcIp() {
    struct ifaddrs * ifAddrStruct = NULL, * ifa = NULL;
    void * tmpAddrPtr = NULL;
    getifaddrs(&ifAddrStruct);
    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa ->ifa_addr->sa_family == AF_INET) { // check it is IP4
            char mask[INET_ADDRSTRLEN];
            void* mask_ptr = &((struct sockaddr_in*) ifa->ifa_netmask)->sin_addr;
            inet_ntop(AF_INET, mask_ptr, mask, INET_ADDRSTRLEN);
            if (strcmp(mask, "255.0.0.0") != 0) {
                // is a valid IP4 Address
                tmpAddrPtr = &((struct sockaddr_in *) ifa->ifa_addr)->sin_addr;
                char *addressBuffer = (char*)malloc(20);
                memset(addressBuffer,0,20);
                inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
                return addressBuffer;
            }
        }
    }
    return NULL;
}
#endif