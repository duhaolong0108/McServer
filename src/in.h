#ifndef in
#define in

/* Socket 用 无符号char */
/* 其他都用 char */

#include "stdio.h"
#include <time.h>
#include <stdlib.h>
#include <winsock2.h>
#include <ws2tcpip.h>

char *cAnd(char *A, char *B)
{
    int length = strlen(A) + strlen(B) + 1;
    char *r = malloc(length);
    if (r)
    { // 检查malloc是否成功
        // fprintf(stderr, "Memory allocation failed!\n");
        exit(1);
    }
    strcpy(r, A);
    strcat(r, B);
    return r;
}


typedef struct client_info
{
    SOCKET clnt_sock;
    struct sockaddr_in clnt_addr;
    int clnt_addr_len;
} client_info;

#define MAX_CONNECTIONS 10


#include "Files.h"
#include "Logger.h"
#include "Socket.c"
#endif