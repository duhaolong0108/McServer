#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <errno.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")

#define MAX_CONNECTIONS 10

int emit = 1;

// 数据结构来存储客户端信息
typedef struct client_info {
    SOCKET clnt_sock;
    struct sockaddr_in clnt_addr;
    int clnt_addr_len;
} client_info;

// 线程回调函数
DWORD WINAPI process_client(LPVOID arg) {
    client_info *client = (client_info *)arg;
	while (emit){
		char buffer[1024];
		int bytes_received = recv(client->clnt_sock, buffer, sizeof(buffer), 0);
		if (bytes_received > 0) {
			Send(client,"114514");
		}
		bzero(buffer,1024);
	}
	Close(client);
    return 0;
}

void Close(client_info *client){
	closesocket(client->clnt_sock);
    free(client);
}

void Send(client_info *client, const char *response){ // 格式： client / Recv
	send(client->clnt_sock, response, strlen(response), 0);
}

int Server(int host) {
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        perror("WSAStartup failed");
        exit(EXIT_FAILURE);
    }

    // 创建套接字
    SOCKET serv_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serv_sock == INVALID_SOCKET) {
        perror("socket failed");
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    // 设置套接字选项以允许重复绑定
    int optval = 1;
    setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&optval, sizeof(optval));

    // 将套接字和 IP、端口绑定
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));  // 每个字节都用0填充
    serv_addr.sin_family = AF_INET;            // 使用 IPv4 地址
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 允许任何 IP 地址连接
    serv_addr.sin_port = htons(host);          // 端口

    if (bind(serv_sock, (SOCKADDR *)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR) {
        perror("bind failed");
        closesocket(serv_sock);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    // 进入监听状态，等待用户发起请求
    if (listen(serv_sock, MAX_CONNECTIONS) == SOCKET_ERROR) {
        perror("listen failed");
        closesocket(serv_sock);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port 1234...");

    HANDLE client_threads[MAX_CONNECTIONS];
    DWORD thread_ids[MAX_CONNECTIONS];
    int i=0;

    while (emit) {
        // 接收客户端请求
        struct sockaddr_in clnt_addr;
        int clnt_addr_len = sizeof(clnt_addr);

        SOCKET clnt_sock = accept(serv_sock, (SOCKADDR *)&clnt_addr, &clnt_addr_len);
        if (clnt_sock == INVALID_SOCKET) {
            perror("accept failed");
            continue;
        }
		
        // 创建线程来处理客户端请求
        client_info *client = malloc(sizeof(client_info));
        client->clnt_sock = clnt_sock;
        client->clnt_addr = clnt_addr;
        client->clnt_addr_len = clnt_addr_len;

        client_threads[i] = CreateThread(NULL, 0, process_client, client, 0, &thread_ids[i]);
        if (!client_threads[i]) {
            perror("CreateThread failed");
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

void Stop(){
    emit = 0;
}