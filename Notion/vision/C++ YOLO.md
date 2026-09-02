---
notion-id: 3868cdcb-81ea-8029-97db-daff77c29343
tags:
  - YOLO
  - 深度学习
  - OpenCV
  - C++
  - 部署
  - onnx
  - dnn
  - NMS
  - blobFromImage
  - 姿态检测
  - onnxRuntime
  - NCHW
  - 目标检测
---

# C++ YOLO 部署：深度学习 → OpenCV → C++ 全链路

> 跨域导航：[[Notion/机器学习/深度学习/深度学习]] · [[C++OpenCV图像处理]] · [[C++OpenCV基础总结]] · [[C++类和对象]]
>
> 目标：使用 Python 训练，C++ 识别部署

## 0. 工具链总览

| 任务 | C++ 侧对应模块 |
|---|---|
| 加载深度学习模型（PyTorch 训练导出 ONNX） | `cv::dnn` 模块 |
| 加载机器学习模型 | `onnxruntime_cxx_api` 模块 |
| 图像预处理（缩放 / BGR→RGB / 归一化） | `blobFromImage` |
| 前向推理 | `net.forward()` |
| 后处理（NMS 去重、画框） | `cv::dnn::NMSBoxes` |

## 1. 前置概念：ONNX 与 dnn

- **ONNX**：开源的、用于表示机器学习模型的标准格式——不同深度学习框架之间的桥梁
- **dnn（深度神经网络）**：一个宏大的系统统称；**YOLO** 是 dnn 中专门为目标检测设计的明星级网络架构

```cpp
cv::dnn::Net net = cv::dnn::readNetFromONNX(模型路径);
```
从文件或内存读取 ONNX 格式模型，转换成 OpenCV 可执行的网络对象（`cv::dnn::Net`）。

### NCHW 四维布局
深度学习框架默认使用 NCHW，该内存布局更适合 GPU 卷积、并行计算效率更高。

- **N**：一次性处理的图数
- **C**：通道数
- **H**：图像竖直方向的像素数
- **W**：图像水平方向的像素数

存储顺序：先存 N 个样本，每个样本存 C 个通道，每个通道存 H 行，每行存 W 个像素。

## 2. 图像 → 张量：blobFromImage

BGR 转 blob 的过程：① 缩放尺寸（如转成 640×640）② BGR 转 RGB ③ 归一化（整数转浮点）④ 用三层 for 循环重排成一维连续序列。

### 单张图片
```cpp
cv::Mat blob = cv::dnn::blobFromImage(img, scale, inputSize, mean, swapRB, crop);
```
- `scale = 1.0 / 255.0`：将像素值归一化
- `inputSize(416, 416)`：调整模型输入大小
- `mean(0,0,0)`：均值，去除平均背景色，让数据更标准
- `swapRB = true`：BGR 转 RGB（很多模型如 YOLO 用 RGB）
- `crop`：
  - `false`：不裁剪，直接拉伸
  - `true`：先按比例缩放，再裁剪中间部分

### 多张图片
```cpp
cv::dnn::blobFromImages(img, scalefactor = 1.0, size = Size(), mean = Scalar(),
                        swapRB = false, crop = false, ddepth = CV_32F);
```
- `img` 为 `vector<cv::Mat>`；转换前最好将图片尺寸统一为 640×640

### 访问 blob
`blob.size`（类型 MatSize）按顺序依次为 NCHW：
```cpp
int N = blob.size[0];
int C = blob.size[1];
int H = blob.size[2];
int W = blob.size[3];
```
访问元素（blob 中像素索引数据类型为 float）：
```cpp
// 方式一：四维索引
int params[4] = {N, C, H, W};
float val = blob.at<float>(params);

// 方式二：data 指针 + 手工偏移
float* data = (float*)blob.data;
int index = n * (C * H * W) + c * (H * W) + h * W + w;
```

## 3. 前向推理

```cpp
net.setInput(blob);              // 把图像张量喂给网络，不做计算
cv::Mat output = net.forward();  // 前向推理，得到预测结果
```
- 目的：从输入特征提取信息，在输出层得出预测结果；用于部署/推理阶段
- 输出通常为 `[N, M]` 二维矩阵：N 为图像数，M 为该 M 类别的概率值
- 经过 YOLO 前向推理，每张图输出 **8400 个预测框**，每框 **84 个数据**（4 个坐标 x,y,w,h + 80 类置信度）
- 注意：YOLO 给出的是物体**中心点坐标**

### 输出重排与筛选
```cpp
cv::Mat data(cv::Size(8400, 84), CV_32F, (float*)output.data); // 去多余维度，借老内存不新申请
data = data.t();        // 转置
cv::Mat dst = img(cv::Range::all(), cv::Range(0, n)); // 提取图片，先行后列
```
- 根据置信度筛选（筛掉小于阈值的框），记录框位置、类别、置信度
- `Size(8400, 84)` 是读 84 行、8400 列

### NMS 去重
```cpp
cv::dnn::NMSBoxes(boxes, confidences, score_threshold, nms_threshold, indices);
```
- `boxes`：矩形框容器
- `score_threshold`：置信度阈值，通常 0.4 / 0.5
- `nms_threshold`：重叠阈值，通常 0.4 / 0.45；重叠占比超过它即认定为同一物体
- `indices`：输出结果，装存活下来的框的索引

> 大脑推理（训练时反向传播）OpenCV 的 dnn 模块不具备，只用于训练模型。

## 4. 姿态检测（YOLOv8n-pose）

姿态检测模型输出的矩阵为 `[8400, 56]`：
- 0-3：人的矩形坐标
- 4：这是一个人的置信度
- 5-55：人体骨架数据，17 个关键点，每点含 3 个数据（x, y, 该点置信度）

骨架连线：
```cpp
const vector<pair<int, int>> skeleton = {
    {0,1}, {0,2}, {1,3}, {2,4}, {5,6}, {5,7}, {7,9}, {6,8}, {8,10},
    {5,11}, {6,12}, {11,12}, {11,13}, {13,15}, {12,14}, {14,16}
}; // 每两个数字代表把哪两个关键点连起来
```

由于 YOLO 返回 640×640 图像坐标，需做比例操作反向映射回摄像头原始分辨率。若只需上半身，就不给下半身的值（即使是 0 也不行，会影响机器判断）。

节点被遮挡的处理：
- **返回上一帧滞留值**：当前帧某点置信度低，直接拷贝上一帧该点坐标
- **均值平滑**：连续几帧丢失，取前面正常值的平均，或直接丢掉该帧

长时间节点消失：
- 将该点置信度权重设为 0（数据传 `[0,0,0]`）
- 用身体的对称来映射

## 5. onnxRuntime 替代方案

适用于加载机器学习模型；需引入 `#include <onnxruntime_cxx_api.h>`。ORT 不自带好用的 NMS，要 NMS 直接使用 `dnn::NMSBoxes`（注意其时间复杂度为 O(N²)）。

```cpp
Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "FallDetectionTest");
// 全局环境 = 后台服务，初始化线程池、内存分配器、日志信息
// WARNING：出现严重警告或错误才打印；第二个参数是代号，日志会显示

Ort::SessionOptions session_options;   // 管理用什么推理，默认 CPU
const wchar_t* model_path = L"模型地址"; // Windows 上用宽字符，路径前加 L
Ort::Session session(env, model_path, session_options);
```
- `wchar_t` 是 C++ 宽字符，Windows 导入模型路径最好用宽字符

### 输入张量组装
```cpp
vector<float> input_features;   // 传入 YOLO 识别到的关键点信息，必须是浮点型
vector<int64_t> input_shape = {n, m}; // 数据形状：n 一次传入几组，m 每组几个特征

Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
    memory_info, input_features.data(), input_features.size(),
    input_shape.data(), input_shape.size());

const char* input_names[]  = {"float_input"};
const char* output_names[] = {"output_label"};
// 输入输出名称固定，可用 https://netron.app 可视化查看具体名称

auto output_tensors = session.Run(
    Ort::RunOptions{nullptr}, input_names, &input_tensor, 1, output_names, 1);
```
- `Ort::RunOptions{nullptr}` 表示无限制
- `output_tensors` 装的是点名请求的输出，元素与请求一一对应

### 读取结果
```cpp
// 直接读标签：把内存当 64 位整数读
int64_t* result_label = output_tensors[0].GetTensorMutableData<int64_t>();
// result_label[0] 为数字，代表标签

// 读概率：Value 是「万能盲盒」，可装 Tensor/Sequence/Map
Ort::AllocatorWithDefaultOptions allocator;   // 提供内存空间
Ort::Value map_values  = output_tensors[0].GetValue(0, allocator); // 0=keys
Ort::Value prob_values = map_values.GetValue(1, allocator);        // 1=values
float* probs = prob_values.GetTensorMutableData<float>();
cout << probs[0] << endl;
```

### 用 runtime 做识别与关键点提取的步骤
1. 配置 session（导入 env、model_path、session_options）
2. 将图片转成 Tensor_value（用 vector 容器装）
3. 获取 input_tensor：
   `input_shape` 格式为 `{张量数, 通道数, 行数, 列数}`
4. 跑模型获取 output_tensors：`auto output_tensors = yolo_session.Run(...)`
5. 获取模型数据和形状：
```cpp
float* float_ptr = output_tensors[0].GetTensorMutableData<float>();
auto output_shape = output_tensors[0].GetTensorTypeAndShapeInfo().GetShape();
// GetTensorTypeAndShapeInfo() 返回含数据类型与尺寸的元数据
// GetShape() 返回形状（NHW）
```

### 跨平台 / 平台加速
```cpp
// 树莓派可用 CPU，但效率低，通常开启针对 ARM 指令集优化的加速包
Ort::SessionOptions session_options;
session_options.AppendExecutionProvider_XNNPACK();

// k230 不能用 ONNX Runtime，改用 KPU 运行

#ifdef _WIN32  // 编译期生效的宏条件编译，非运行期
std::wstring w_model_path(model_path.begin(), model_path.end());
Ort::Session session(env, w_model_path.c_str(), session_options);
#else
Ort::Session session(env, model_path.c_str(), session_options);
#endif
```
- 进程是正在运行的程序；线程是网络能调度的最小单位
- `.c_str()` 将 string 转成 `const char*`

## 6. 输出解析：按任务类型

### 目标识别
`[1, 6, 8400]`：
- `1`：输入一张图片
- `8400`：预测框总数
- `6 = 4 + 2`：4 个 x,y,w,H 坐标 + 2 个类别概率

### 图形/实例分割
`output0` 为 `[1, 38, 8400]`：
- `38 = 4 + 2 + 32`：4 坐标 + 2 类别置信度（如黑白棋）+ 32 掩码系数

### 用指针切片读区
```cpp
cv::Mat scores(1, 2, CV_32F, row_ptr + 4);
// 传现成指针就不新申请内存，借用旧内存
// 1 行、2 列、32 位浮点
// row_ptr + 4：指针后移，跳过前 4 个坐标，从「类别得分」开始读 2 个数据
```

### 类别偏移法（防框互相覆盖）
识别到的两种物体重叠时可能一个覆盖另一个。遍历每框概率，超过阈值就提出；但 NMS 也可能吞框，可给不同物体不同偏移量，让 NMS 认为它们不在同一处——**类别偏移法**。

## 7. 模型内部结构（YOLOv8）

```
输入图片 → 主干网络 → 特征融合 → 检测头 → 最终预测框
```
1. **主干网络**：看图提取特征（边缘、颜色、纹理）
2. **特征融合**：融合大图特征和小图特征（既看清小目标，又把握全局）
3. **检测头**：网络的最后几层，将融合特征图「翻译」成数值结果
   - 分类分支：判断类别
   - 回归分支：判断精确位置，返回坐标

### DFL（概率分布代替单一数值）
1. 对每个边界输出 16 个概率对数
2. 概率对数经 Softmax 得到概率分布
3. 计算概率分布与各点权重矩阵的点积，得数学期望，最终得到边界框偏移量
   - 训练阶段知道点的准确值时，可对该点相邻两点设置权重（防止双峰分布），为后面的 DFL 卷积层铺垫

### HBB vs OBB
- YOLO 系列属于单阶段目标检测器
- **HBB**：认为世界横平竖直；**YOLO-OBB**：承认世界带角度、自由旋转
- OBB 主干+特征融合出特征图后进入解耦头，分为两个平行并行的卷积分支：
  - 分类分支：判断是什么，输出每类置信度
  - 回归分支：判断长什么样，旋转角 θ + 中心点 + 宽高挤在同一回归分支
- HBB 与 OBB 每张图吐出的框数量相同：OBB 每张 640×640 图吐 8400 框，每框 = 4 坐标 + 各物体置信度 + 角度
- 选置信度最高的预测框，顺带读取预测的 θ
- HBB 用标准 NMS；OBB 用 **Rotated NMS**

### 亚像素 → 像素转换
1. 还原真实分辨率
2. 浮点级 NMS / IoU 过滤
3. 四舍五入取整
