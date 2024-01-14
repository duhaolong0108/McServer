import socket

def run_client():
    # 创建套接字
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    # 设置超时时间
    client_socket.settimeout(5.0)

    try:
        # 连接到服务器
        server_address = ('localhost', 1234)
        client_socket.connect(server_address)

        print(f'Connected to {server_address[0]}:{server_address[1]}')

        # 接收服务器消息
        message = client_socket.recv(1024).decode('utf-8')
        print(f'Received: {message}')

        # 关闭套接字
        client_socket.close()

    except (socket.timeout, ConnectionRefusedError):
        print('Connection refused or timed out.')

if __name__ == '__main__':
    run_client()