#include <stdio.h>
#include "time.h"
#include "math.h"
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

#define Info(T, D) cout("\033[0m", "INFO", T, D)
#define Warn(T, D) cout("\033[33m", "WARN", T, D)
#define Error(T, D) cout("\033[31m", "ERROR", T, D)

#define MSB 0x80
#define REST 0x7F

#define _c char
#define uc unsigned _c
#define it int
#define sh short
#define Close(A) (closesocket(A->clnt_sock), free(A))
#define S_Close(A) (closesocket(A), WSACleanup())
#define Send(client, D) send(client->clnt_sock, (_c *)D, strlen((_c *)D), 0)
#define Recv(client, l, CHAR) recv(client->clnt_sock, (_c *)CHAR, l, 0)
#define Len(D) strlen((_c *)D)

typedef struct client_info
{
    SOCKET clnt_sock;
    struct sockaddr_in clnt_addr;
    it clnt_addr_len;
    it num;
} client_info;

void cout(_c *C, _c *F, _c *T, _c *D)
{
    long rawtime;
    struct tm *info;
    time(&rawtime);
    info = localtime(&rawtime);
    printf("%s[%02d:%02d:%02d] [%s/%s] %s\n\033[0m", C, info->tm_hour, info->tm_min, info->tm_sec, T, F, D);
}

_c G[1024];

uc *encode(it num, client_info *client)
{
    sh offset = 0;
    uc *out = (uc *)malloc(sizeof(uc));
    while (num > REST)
    {
        out[offset++] = (uc)((num & 0xff) | MSB);
        num >>= 7;
    }

    out[offset++] = (uc)(num);
    return out;
}

DWORD WINAPI process_client(LPVOID arg)
{
    client_info *client = (client_info *)arg;
    uc r[1024],
        *wd = (uc *)malloc(sizeof(uc));
    sh postion = 0;
    it res = 0, t;
    while (Recv(client, 1, r))
    {
        wd[postion++] = r[0];
        if (res && !(--res))
        {
            postion = 0;

            if (wd[0] == 0x00 && wd[t - 1] == 0x01)
            {
                it Data_len = 0;
            }
        }
        else if (r[0] < 128 && !res)
        {
            it offset = 0;
            it shift = 0;
            it counter = offset;
            uc b;
            postion = 0;
            do
            {
                if (counter > 2)
                {
                    Error("Server", "Could not decode varit\n");
                    break;
                }
                b = wd[counter++];
                res += shift < 21
                           ? (b & REST) << shift
                           : (b & REST) * pow(2, shift);
                shift += 7;
            } while (b >= MSB);
            t = res;
        }
    }
    Close(client);
    return 0;
}

it main()
{
    sh port = 25565;
    sh MAX_CONNECTIONS = 10;
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data))
    {
        Error("Server", "WSAStartup failed");
        return 1;
    }

    SOCKET serv_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serv_sock == INVALID_SOCKET)
    {
        Error("Server", "socket failed");
        WSACleanup();
        return 1;
    }

    it optval = 1;
    setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, (const _c *)&optval, sizeof(optval));

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    if (bind(serv_sock, (SOCKADDR *)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR)
    {
        Error("Server", "bind failed");
        S_Close(serv_sock);
        return 1;
    }

    if (listen(serv_sock, MAX_CONNECTIONS) == SOCKET_ERROR)
    {
        Error("Server", "listen failed");
        S_Close(serv_sock);
        return 1;
    }

    snprintf(G, 37, "Server is listening on port %d", port);
    Info("Server", G);

    HANDLE client_threads[MAX_CONNECTIONS];
    DWORD thread_ids[MAX_CONNECTIONS];
    unsigned it i = 0;

    while (1)
    {
        struct sockaddr_in clnt_addr;
        it clnt_addr_len = sizeof(clnt_addr);

        SOCKET clnt_sock = accept(serv_sock, (SOCKADDR *)&clnt_addr, &clnt_addr_len);
        if (clnt_sock == INVALID_SOCKET)
        {
            Error("Server", "accept failed");
            continue;
        }

        client_info *client = (client_info *)malloc(sizeof(client_info));
        client->clnt_sock = clnt_sock;
        client->clnt_addr = clnt_addr;
        client->clnt_addr_len = clnt_addr_len;
        client->num = i;

        client_threads[i] = CreateThread(NULL, 0, process_client, client, 0, &thread_ids[i]);
        if (!client_threads[i++])
        {
            Error("Server", "CreateThread failed");
            closesocket(clnt_sock);
            free(client);
            continue;
        }
    }
    S_Close(serv_sock);
}