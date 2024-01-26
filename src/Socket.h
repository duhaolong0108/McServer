#ifndef Socket
#define Socket

#include "in.h"

#define MSB 0x80
#define REST 0x7F

#define Close(A) (closesocket(A->clnt_sock), free(A))
#define S_Close(A) (closesocket(A), WSACleanup())
#define Send(client, D) send(client->clnt_sock, (char *)D, strlen((char *)D), 0)
#define Recv(client, l, CHAR) recv(client->clnt_sock, (char *)CHAR, l, 0)
#define Len(D) strlen((char *)D)
#define G_l(D) D[Len(D) - 1]

unsigned char *encode(unsigned int num, client_info *client)
{
    short offset = 0;
    unsigned char *out = malloc(sizeof(unsigned char));
    while (num >= 0x80)
    {
        out[offset++] = (char)((num & 0xff) | 0x80);
        num >>= 7;
    }

    out[offset++] = (char)(num);
    return out;
}

DWORD WINAPI process_client(LPVOID arg)
{
    client_info *client = (client_info *)arg;
    unsigned char r[1024],
        *wd = (unsigned char *)malloc(sizeof(unsigned char));
    short postion = 0;
    int res = 0, t;
    while (Recv(client, 1, r))
    {
        wd[postion++] = r[0];
        if (res && !(--res))
        {
            postion = 0;

            if(wd[0] == 0x01 && G_l(wd) == 0x01){
                // 两个Varint 9;
                int Data_len = 0;
            }

            continue;
        }
        if (r[0] < 128 && !res)
        {
            int offset = 0;
            int shift = 0;
            int counter = offset;
            unsigned char b;
            postion = 0;
            do
            {
                if (counter >= 3)
                {
                    Error("Server", "Could not decode varint\n");
                    return 0;
                }
                b = wd[counter++];
                res += shift < 21
                           ? (b & REST) << shift
                           : (b & REST) * pow(2, shift);
                shift += 7;
            } while (b >= MSB);
            t = res;
            continue;
        }
    }
    Close(client);
    return 0;
}

void Server(short port, short MAX_CONNECTIONS)
{

    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data))
    {
        Error("Server", "WSAStartup failed");
        return;
    }

    SOCKET serv_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serv_sock == INVALID_SOCKET)
    {
        Error("Server", "socket failed");
        WSACleanup();
        return;
    }

    int optval = 1;
    setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&optval, sizeof(optval));

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    if (bind(serv_sock, (SOCKADDR *)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR)
    {
        Error("Server", "bind failed");
        S_Close(serv_sock);
        return;
    }

    if (listen(serv_sock, MAX_CONNECTIONS) == SOCKET_ERROR)
    {
        Error("Server", "listen failed");
        S_Close(serv_sock);
        return;
    }

    char t[37];
    snprintf(t, 37, "Server is listening on port %d", port);
    Info("Server", t);

    HANDLE client_threads[MAX_CONNECTIONS];
    DWORD thread_ids[MAX_CONNECTIONS];
    int i = 0;

    while (1)
    {
        // 接收客户端请求
        struct sockaddr_in clnt_addr;
        int clnt_addr_len = sizeof(clnt_addr);

        SOCKET clnt_sock = accept(serv_sock, (SOCKADDR *)&clnt_addr, &clnt_addr_len);
        if (clnt_sock == INVALID_SOCKET)
        {
            Error("Server", "accept failed");
            continue;
        }

        // 创建线程来处理客户端请求
        client_info *client = (client_info *)malloc(sizeof(client_info));
        client->clnt_sock = clnt_sock;
        client->clnt_addr = clnt_addr;
        client->clnt_addr_len = clnt_addr_len;

        client_threads[i] = CreateThread(NULL, 0, process_client, client, 0, &thread_ids[i]);
        if (!client_threads[i])
        {
            Error("Server", "CreateThread failed");
            closesocket(clnt_sock);
            free(client);
            continue;
        }

        i++;
    }

    // 关闭服务器套接字
    S_Close(serv_sock);
}
#endif