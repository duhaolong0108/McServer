#ifndef in
#define in

#include <stdio.h>
#include <stdint.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <time.h>

typedef struct client_info
{
    SOCKET clnt_sock;
    struct sockaddr_in clnt_addr;
    int clnt_addr_len;
} client_info;


typedef struct player
{
    char name;
    char uuid;
    double x;
    double y;
    double z;
    double f;
    short have[36][2];
    short use;
};


#define MAX_CONNECTIONS 10

#include "Files.c"
#include "Logger.c"
#include "Until.c"
#include "World.c"
#include "Entity.c"
#include "Socket.c"

#endif