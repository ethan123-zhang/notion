import socket
import struct
import cv2
import numpy as np

def start_server():
    # 电脑端作为 Server，绑定 0.0.0.0 代表监听电脑所有网卡（不管哪个IP进来的流量都收）
    LISTEN_IP = "0.0.0.0"
    PORT = 8888

    # 1. 创建 TCP Socket 并绑定端口
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # 设置端口复用，防止程序重启时报 Address already in use 错误
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.bind((LISTEN_IP, PORT))
    
    # 2. 开启监听
    server_socket.listen(1)
    print(f"[Server] 监控服务端已启动，正在监听端口 {PORT}，等待树莓派连接...")

    while True:
        try:
            # 3. 阻塞等待树莓派呼叫
            client_socket, client_address = server_socket.accept()
            print(f"[Server] 树莓派已成功上线！设备地址: {client_address}")

            while True:
                # 4. 读取 4 字节的包头（图片长度）
                header = client_socket.recv(4)
                if not header:
                    break # 没读到数据，说明树莓派断开了

                # struct.unpack('>I', ...) 的意思是：
                # 严格按照网络字节序（> 代表大端模式）将 4 字节二进制还原为无符号整数(I)
                data_len = struct.unpack('>I', header)[0]

                # 5. 判定是否为心跳包
                if data_len == 0:
                    print("[Heartbeat] 收到树莓派的心跳包，设备在线。")
                    continue # 略过后续图片解析，继续等下一个包

                # 6. 接收真实的 JPEG 图像二进制数据（处理网络分包问题）
                img_data = b""
                while len(img_data) < data_len:
                    # 每次最多读 4096 字节，直到读满 data_len 为止
                    packet = client_socket.recv(min(data_len - len(img_data), 4096))
                    if not packet:
                        break
                    img_data += packet

                if len(img_data) < data_len:
                    print("[Warning] 接收数据不完整，丢弃该帧")
                    break

                # 7. 解码 JPEG 并显示在屏幕上
                # 将二进制字节流转为内存 numpy 数组
                nparr = np.frombuffer(img_data, np.uint8)
                # 将 JPEG 编码的内存数据解码为 OpenCV 的 BGR 图片矩阵
                frame = cv2.imdecode(nparr, cv2.IMREAD_COLOR)

                if frame is not None:
                    # 弹出大屏监控窗口
                    cv2.imshow("Raspberry Pi Edge AI Monitor", frame)
                    
                    # 必须加 waitKey，否则 OpenCV 窗口会卡死变白。按 'q' 键手动退出
                    if cv2.waitKey(1) & 0xFF == ord('q'):
                        print("[Server] 用户手动关闭监控。")
                        return

        except Exception as e:
            print(f"[Error] 通信发生异常: {e}")
        finally:
            client_socket.close()
            print("[Server] 树莓派下线，等待重新连接...\n")

    # 释放所有 OpenCV 窗口
    cv2.destroyAllWindows()
    server_socket.close()

if __name__ == "__main__":
    start_server()