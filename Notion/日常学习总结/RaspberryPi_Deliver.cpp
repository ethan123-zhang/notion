#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <chrono>
#include <csignal>
#include <vector>
#include <string>

// OpenCV 图像处理与 DNN 模块
#include <opencv2/opencv.hpp>
// ONNX Runtime C++ API
#include <onnxruntime_cxx_api.h>

// Linux Socket 网络通信
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

// ==========================================
// 全局配置与状态标志
// ==========================================
std::atomic<bool> is_running{true}; // 控制所有线程优雅退出的心跳标志
                                    // 使用atomic是因为防止变量修改后其他线程仍然使用缓存的变量值
                                    // 当我们将atomic数据类型的变量进行更改，所有CPU核心上的子线程都会在同一时刻发生改变，没有任何延迟

// 捕获 Ctrl+C (SIGINT) 信号，实现优雅退出
void signal_handler(int signum)
{
    std::cout << "\n[System] 收到终止信号(Ctrl+C)，正在通知各线程安全退出..." << std::endl;
    is_running = false;
}

// ==========================================
// 核心组件：带防爆机制的线程安全队列
// ==========================================
class FrameQueue
{
private:
    std::queue<cv::Mat> q;
    std::mutex mtx;             // 互斥锁
    std::condition_variable cv; // 条件变量
    size_t max_size;

public:
    // 默认最大长度设为 2，保证下游拿到的永远是最新的 1~2 帧
    FrameQueue(size_t size = 2) : max_size(size) {} // FrameQueue(size_t size = 2):这是构造函数，size = 2是默认参数；max_size(size)是将传进来的参数赋值给max_size

    void push(const cv::Mat &frame)
    {
        std::lock_guard<std::mutex> lock(mtx);
        // 【防爆机制】：如果队列已满，主动抛弃最旧的一帧，防止内存溢出和画面严重延迟
        if (q.size() >= max_size)
        {
            q.pop();
        }
        // 【避坑】：必须 clone() 深拷贝。否则多线程下底层图像内存可能被复写污染
        q.push(frame.clone());
        cv.notify_one(); // 唤醒正在休眠等待的消费者线程
    }

    // 支持超时的 pop（timeout_ms < 0 表示死等）
    bool pop(cv::Mat &frame, int timeout_ms = -1)
    {
        std::unique_lock<std::mutex> lock(mtx);
        if (timeout_ms < 0)
        {
            // 条件变量：无数据且系统仍在运行时，释放锁并挂起线程（CPU占用降为0）
            cv.wait(lock, [this]
                    { return !q.empty() || !is_running; });
        }
        else
        {
            // 超时等待：用于 Socket 线程判定是否该发心跳包了
            if (!cv.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                             [this]
                             { return !q.empty() || !is_running; }))
            {
                return false;
            }
        }

        // 如果被唤醒是因为程序要退出了，且队列为空，直接返回 false
        if (!is_running && q.empty())
            return false;

        frame = q.front();
        q.pop();
        return true;
    }

    // 唤醒所有等待中的线程（用于程序退出时解除阻塞）
    void wake_all()
    {
        cv.notify_all();
    }
};

// 实例化两个全局流水线交接队列
FrameQueue queue_raw(2);       // 负责：线程1(采集) -> 线程2(推理)
FrameQueue queue_processed(2); // 负责：线程2(推理) -> 线程3(发送)

// ==========================================
// 线程 1：摄像头采集流水线
// ==========================================
void thread_camera()
{
    cv::VideoCapture cap(0);
    // 降低摄像头采集分辨率，减少总线带宽压力
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);

    if (!cap.isOpened())
    {
        std::cerr << "[Camera] 摄像头打开失败！请检查 /dev/video0 权限。" << std::endl;
        is_running = false;
        return;
    }

    cv::Mat frame;
    while (is_running)
    {
        cap >> frame; // 阻塞式读取硬件画面
        if (frame.empty())
            continue;

        queue_raw.push(frame); // 塞入队列 1
    }
    cap.release();
    queue_raw.wake_all(); // 确保下游不会死锁在 wait() 上
    std::cout << "[Camera] 采集线程已安全退出。" << std::endl;
}

// ==========================================
// 线程 2：YOLOv8 模型推理与后处理流水线
// ==========================================
void thread_inference()
{
    // 1. 初始化 ONNX Runtime 环境
    Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "YOLOv8_Edge");
    Ort::SessionOptions session_options;

    // 【硬件加速优化】：树莓派上使用最大图优化级别
    session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
    session_options.SetIntraOpNumThreads(4); // 绑定树莓派的 4 个物理核心

    // 加载模型 (请确保当前路径有对应的模型文件)
    Ort::Session session(env, "/home/hqf/Desktop/YOLODetect/yolov8n.onnx", session_options);
    Ort::AllocatorWithDefaultOptions allocator;

    // YOLOv8 输入配置 (1x3x640x640 Float32)
    const int INPUT_WIDTH = 640;
    const int INPUT_HEIGHT = 640;
    std::vector<int64_t> input_shape = {1, 3, INPUT_HEIGHT, INPUT_WIDTH};
    size_t input_tensor_size = 1 * 3 * INPUT_HEIGHT * INPUT_WIDTH;
    std::vector<float> input_tensor_values(input_tensor_size);
    Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

    const char *input_names[] = {"images"};
    const char *output_names[] = {"output0"};

    const int num_classes = 80;
    const int num_channels = 4 + num_classes; // 比如 23 种药，这里就是 27
    const int num_anchors = 8400;

    cv::Mat frame, blob;
    while (is_running)
    {
        // 2. 取图：如果没图，线程在这里休眠，0 CPU 消耗
        if (!queue_raw.pop(frame))
            break;

        // 记录原始尺寸，用于将预测框缩放回原图
        float x_factor = frame.cols / static_cast<float>(INPUT_WIDTH);
        float y_factor = frame.rows / static_cast<float>(INPUT_HEIGHT);

        // 3. 预处理：BGR转RGB、除以255归一化、缩放为 640x640
        cv::dnn::blobFromImage(frame, blob, 1.0 / 255.0, cv::Size(INPUT_WIDTH, INPUT_HEIGHT), cv::Scalar(), true, false);
        std::memcpy(input_tensor_values.data(), blob.ptr<float>(), input_tensor_size * sizeof(float));

        Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
            memory_info, input_tensor_values.data(), input_tensor_size, input_shape.data(), input_shape.size());

        // 4. 前向推理 (纯 CPU 计算密集型操作)
        auto output_tensors = session.Run(
            Ort::RunOptions{nullptr}, input_names, &input_tensor, 1, output_names, 1);

        // 5. YOLOv8 专属后处理：解析张量
        float *pdata = output_tensors[0].GetTensorMutableData<float>();
        // 将 1x27x8400 重组为 27x8400 的 OpenCV 矩阵，并转置为 8400x27 以便按行读取
        cv::Mat output(num_channels, num_anchors, CV_32F, pdata);
        cv::Mat output_T = output.t();

        std::vector<int> class_ids;
        std::vector<float> confidences;
        std::vector<cv::Rect> boxes;

        for (int i = 0; i < num_anchors; i++)
        {
            float *row_ptr = output_T.ptr<float>(i);
            // row_ptr[0~3] 是 cx, cy, w, h；之后的全是类别分数
            float *classes_scores = row_ptr + 4;

            cv::Mat scores(1, num_classes, CV_32F, classes_scores);
            cv::Point class_id_point;
            double max_class_score;
            // 找到概率最高的那个类别
            cv::minMaxLoc(scores, 0, &max_class_score, 0, &class_id_point);

            if (max_class_score > 0.45)
            { // 置信度阈值过滤
                float cx = row_ptr[0];
                float cy = row_ptr[1];
                float w = row_ptr[2];
                float h = row_ptr[3];

                // 还原到原始图像尺度
                int left = int((cx - 0.5 * w) * x_factor);
                int top = int((cy - 0.5 * h) * y_factor);
                int width = int(w * x_factor);
                int height = int(h * y_factor);

                boxes.push_back(cv::Rect(left, top, width, height));
                confidences.push_back((float)max_class_score);
                class_ids.push_back(class_id_point.x);
            }
        }

        // 6. NMS (非极大值抑制)，剔除重叠的框
        std::vector<int> nms_indices;
        cv::dnn::NMSBoxes(boxes, confidences, 0.45f, 0.50f, nms_indices);

        // 7. 在原图上绘制结果
        for (int idx : nms_indices)
        {
            cv::Rect box = boxes[idx];
            cv::rectangle(frame, box, cv::Scalar(0, 255, 0), 2);
            std::string label = "ID:" + std::to_string(class_ids[idx]) + " " + std::to_string(confidences[idx]).substr(0, 4);
            cv::putText(frame, label, cv::Point(box.x, box.y - 5), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 0), 2);
        }

        // 8. 将绘制好的图像塞入队列 2 准备发送
        queue_processed.push(frame);
    }
    queue_processed.wake_all();
    std::cout << "[Inference] 推理线程已安全退出。" << std::endl;
}

// ==========================================
// 线程 3：Socket 通信与状态机
// ==========================================
void thread_socket()
{
    // 目标服务器（你的电脑）的内网 IP 地址，需确保树莓派和电脑在同一个局域网/WiFi下
    const char *SERVER_IP = "192.168.137.1";
    // 目标服务器上监听的端口号（要和电脑端接收程序的端口一致）
    const int PORT = 8888;

    // 【外层死循环】：负责网络连接的整体生命周期管理（以及断线后的重连）
    while (is_running)
    {

        // ---------------------------------------------------------
        // 【状态 1：创建套接字并尝试建立 TCP 连接】
        // ---------------------------------------------------------

        // socket() 创建电话机：
        // AF_INET 代表使用 IPv4 协议；
        // SOCK_STREAM 代表使用 TCP 面向连接的可靠流传输；
        // 返回的 sock 是一个文件描述符（句柄），后续所有网络操作都认这个数字。
        int sock = socket(AF_INET, SOCK_STREAM, 0);

        // 准备一个存放服务器网络地址信息的结构体
        struct sockaddr_in server_addr;
        // 清零或初始化结构体（这里虽然没显式清零，但在赋值完整时亦可）
        server_addr.sin_family = AF_INET; // 设定地址族为 IPv4

        // htons() 主机字节序转网络字节序：
        // 树莓派 CPU 是小端模式（低位在前），而网络传输标准是大端模式（高位在前）。
        // 必须用 htons (Host to Network Short) 把 2 字节的端口号转成网络大端标准。
        server_addr.sin_port = htons(PORT);

        // inet_pton() 字符串 IP 转二进制 IP：
        // pton 代表 "Presentation to Numeric"（点分十进制字符串转二进制网络大端数值）。
        // 把 "192.168.1.100" 转换并直接写入结构体中的 sin_addr 成员中。
        inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

        std::cout << "[Socket] 尝试连接大屏服务端..." << std::endl;

        // connect() 拨号呼叫：向电脑发起 TCP 三次握手。
        // 如果返回值小于 0，说明服务器没开、IP错了或者网络超时，连接失败。
        if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        {
            // 避坑：即使连接失败，刚才创建的 sock 也会占用系统文件句柄，必须立即关闭！
            close(sock);

            // 为了防止连接失败时死循环疯狂重试把 CPU 跑满，主动让线程挂起睡觉 2 秒。
            std::this_thread::sleep_for(std::chrono::seconds(2));

            // 结束本次外层循环，直接跳回循环头部，重新执行 socket() 和 connect() 尝试重连。
            continue;
        }
        std::cout << "[Socket] 成功连接至电脑端！" << std::endl;

        // ---------------------------------------------------------
        // 【状态 2：数据发送与心跳维持】（连接成功后才能进入此区域）
        // ---------------------------------------------------------

        cv::Mat frame;                  // 准备承接从队列取出的图像矩阵
        std::vector<uchar> jpeg_buffer; // 准备存放 JPEG 压缩后的二进制字节流

        // cv::IMWRITE_JPEG_QUALITY 代表设置 JPEG 压缩质量，范围 0-100。
        // 80 是工业部署中画质与体积的绝佳平衡点，能将原始几 MB 的图片瞬间压缩到几十 KB！
        std::vector<int> encode_params = {cv::IMWRITE_JPEG_QUALITY, 80};

        // 【内层死循环】：只要网络不断开，就在这个循环里疯狂发图或者发心跳
        while (is_running)
        {

            // 【核心策略】：调用支持超时的 pop。限时等待 3000 毫秒（3秒）。
            // 情况 A：3秒内队列 2 里来图了，函数返回 true，进入 if 分支发图。
            // 情况 B：3秒到了队列里还是空空如也，说明画面静止或者上游卡了，函数返回 false，进入 else 分支发心跳。
            if (queue_processed.pop(frame, 3000))
            {

                // imencode() 将 OpenCV 的原始 Mat 矩阵数据编码压缩为 JPEG 格式，结果存入 jpeg_buffer。
                cv::imencode(".jpg", frame, jpeg_buffer, encode_params);

                // 【自定义防粘包应用层协议】：
                // TCP 是面向字节流的，如果不告诉电脑一张图有多长，电脑就会把多张图连在一起解析导致崩溃。
                // 解决方案：构建 [4字节长度头] + [真实JPEG数据] 的结构。

                // htonl() 主机转网络长整型：
                // 把 std::vector 的大小（4字节整数）从树莓派的小端转为网络传输的大端字节序。
                uint32_t data_len = htonl(jpeg_buffer.size());

                // 步骤 1：先向电脑发送 4 个字节的“长度头”。
                // MSG_NOSIGNAL 参数配合 main 函数的 SIG_IGN 形成双保险，
                // 确保对端关闭时 send 不会触发信号杀死程序，而是直接让 send 返回 -1。
                if (send(sock, &data_len, sizeof(data_len), MSG_NOSIGNAL) < 0)
                {
                    // 如果返回值小于 0，说明电脑端程序关了或者网线拔了，网络已断！
                    // 执行 break，立刻打破内层死循环！
                    break;
                }

                // 步骤 2：紧接着把真正的 JPEG 图片二进制流发过去。
                // 电脑端收到刚才的 4 字节长度后，就会在底层精准截取这个长度的字节，完美防止粘包。
                if (send(sock, jpeg_buffer.data(), jpeg_buffer.size(), MSG_NOSIGNAL) < 0)
                {
                    // 同理，发送图片实体失败也代表断网，打碎内循环。
                    break;
                }
            }
            else
            {
                // 【心跳机制触发】：
                // 代码走到这里，说明 pop 等了 3 秒都没等到新图，但只要 `is_running` 还是 true，
                // 说明程序没想退出，只是单纯没图。为了防止网络因长时间没流量而被路由器“踢下线”，
                // 必须主动发一个极小的包（心跳包）探探路，检测网络是否还健康。
                if (is_running)
                {
                    uint32_t heartbeat = 0; // 规定：发送一个长度为 0 的包头，代表心跳包。

                    // 尝试把这 4 个字节的 0 发送给电脑
                    if (send(sock, &heartbeat, sizeof(heartbeat), MSG_NOSIGNAL) < 0)
                    {
                        std::cerr << "[Socket] 心跳发送失败，网络连接异常断开。" << std::endl;
                        // 如果连 4 个字节都发不出去了，说明网真的断了，打破内循环去重连。
                        break;
                    }
                }
            }
        } // 【内层循环结束点】

        // 只要内层循环被 break 出来了（不管是发图失败还是心跳失败），都会走到这里：
        close(sock); // 彻底关闭当前这个已经死掉的套接字，释放网络资源。
        std::cout << "[Socket] 连接中断，即将准备重连..." << std::endl;

    } // 【外层循环结束点】当你按 Ctrl+C 导致 is_running 变成 false 时，外层循环破裂，走向下方退出。

    std::cout << "[Socket] 发送线程已安全退出。" << std::endl;
}

// ==========================================
// 主函数入口
// ==========================================
int main()
{
    // 极其关键：拦截 SIGPIPE 信号
    // 如果电脑端突然拔网线，树莓派继续 send 会触发操作系统的 SIGPIPE 导致直接闪退。
    // 忽略它，让 send 函数只返回 -1，交给我们的代码逻辑去重连。
    signal(SIGPIPE, SIG_IGN);

    // 注册 Ctrl+C 拦截
    signal(SIGINT, signal_handler);

    std::cout << "===========================================" << std::endl;
    std::cout << ">>> 边缘视觉处理流水线启动 (Edge AI Pipeline) <<<" << std::endl;
    std::cout << "===========================================" << std::endl;

    // 启动三大线程，各自负责自己的流水线工位
    std::thread t1(thread_camera);
    std::thread t2(thread_inference);
    std::thread t3(thread_socket);

    // 阻塞主进程，直到收到 Ctrl+C，三个线程均完成清理后结束
    t1.join();
    t2.join();
    t3.join();

    std::cout << ">>> 进程已安全关闭，内存/设备资源已释放。 <<<" << std::endl;
    return 0;
}