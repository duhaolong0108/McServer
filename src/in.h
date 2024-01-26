#ifndef In
#define In

#include <stdio.h>
#include "time.h"
#include "math.h"
#include <winsock2.h>

#pragma comment(lib,"ws2_32.lib")

typedef struct client_info
{
    SOCKET clnt_sock;
    struct sockaddr_in clnt_addr;
    int clnt_addr_len;
} client_info;

#include "Tool.h"
#include "Logger.h"
#include "Socket.h"
#endif