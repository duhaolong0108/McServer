#include "in.h"

typedef struct client_info
{
    SOCKET clnt_sock;
    struct sockaddr_in clnt_addr;
    int clnt_addr_len;
} client_info;

int Handle(client_info *client, short *D)
{
    // https://wiki.vg/Protocol
}

int emit = 1;

void Close(client_info *client)
{
    closesocket(client->clnt_sock);
    free(client);
}

void Send(client_info *client, const short *response)
{ // 格式： client / Recv
    send(client->clnt_sock, (char *)response, strlen((char *)response), 0);
}

// 线程回调函数
DWORD WINAPI process_client(LPVOID arg)
{
    client_info *client = (client_info *)arg;
    while (emit)
    {
        short buffer[1024];
        recv(client->clnt_sock, (char *)buffer, sizeof(buffer), 0) && Handle(client, buffer);
        memset(buffer, 0, 1024);
    }
    Close(client);
    return 0;
}

int Server(int port)
{
    Logger("Server");
    WSADATA wsa_data;
    if (!WSAStartup(MAKEWORD(2, 2), &wsa_data))
    {
        Error("WSAStartup failed");
        return 1;
    }

    // 创建套接字
    SOCKET serv_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serv_sock == INVALID_SOCKET)
    {
        Error("socket failed");
        WSACleanup();
        return 1;
    }

    // 设置套接字选项以允许重复绑定
    int optval = 1;
    setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&optval, sizeof(optval));

    // 将套接字和 IP、端口绑定
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));      // 每个字节都用0填充
    serv_addr.sin_family = AF_INET;                // 使用 IPv4 地址
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 允许任何 IP 地址连接
    serv_addr.sin_port = htons(port);              // 端口

    if (bind(serv_sock, (SOCKADDR *)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR)
    {
        Error("bind failed");
        closesocket(serv_sock);
        WSACleanup();
        return 1;
    }

    // 进入监听状态，等待用户发起请求
    if (listen(serv_sock, MAX_CONNECTIONS) == SOCKET_ERROR)
    {
        Error("listen failed");
        closesocket(serv_sock);
        WSACleanup();
        return 1;
    }

    char t[6];
    snprintf(t, 6, "%d", port);
    Info(cAnd("Server is listening on port ", t));

    HANDLE client_threads[MAX_CONNECTIONS];
    DWORD thread_ids[MAX_CONNECTIONS];
    int i = 0;

    while (emit)
    {
        // 接收客户端请求
        struct sockaddr_in clnt_addr;
        int clnt_addr_len = sizeof(clnt_addr);

        SOCKET clnt_sock = accept(serv_sock, (SOCKADDR *)&clnt_addr, &clnt_addr_len);
        if (clnt_sock == INVALID_SOCKET)
        {
            Error("accept failed");
            continue;
        }

        // 创建线程来处理客户端请求
        client_info *client = malloc(sizeof(client_info));
        client->clnt_sock = clnt_sock;
        client->clnt_addr = clnt_addr;
        client->clnt_addr_len = clnt_addr_len;

        client_threads[i] = CreateThread(NULL, 0, process_client, client, 0, &thread_ids[i]);
        if (!client_threads[i])
        {
            Error("CreateThread failed");
            closesocket(clnt_sock);
            free(client);
            continue;
        }

        i++;
    }

    // 关闭服务器套接字
    closesocket(serv_sock);
    WSACleanup();

    return 0;
}

void Stop()
{
    emit = 0;
}