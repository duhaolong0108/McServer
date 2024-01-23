import socket

def run_client():
    # 创建套接字
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    # 设置超时时间
    client_socket.settimeout(5.0)

    try:
        server_address = ('localhost', 25565)
        client_socket.connect(server_address)

        print(f'Connected to {server_address[0]}:{server_address[1]}')
        while True:
            client_socket.send(b'\x7f')
            # 接收服务器消息
            import time
            time.sleep(1)
        # 关闭套接字

    except (socket.timeout, ConnectionRefusedError):
        print('Connection refused or timed out.')

if __name__ == '__main__':
    run_client()