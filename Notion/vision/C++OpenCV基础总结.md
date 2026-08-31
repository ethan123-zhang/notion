---
notion-id: 37f8cdcb-81ea-80af-ae24-e7d396838002
tags:
  - OpenCV
  - 图像处理
  - C++
  - 基础总结
  - Mat
  - 像素
  - 色彩空间
  - 直方图
  - 图像IO
  - HSV
  - Scalar
  - ROI
---

# C++ OpenCV 基础总结

## 基本概念

- OpenCV 画布的坐标原点在左上角。
- C++ 中不可以重复定义，同一个变量可以多次使用。
- 图像空间：看到图像中某个像素的具体位置 (x, y)。
- 特征空间：不直接看"位置"，而是看像素的"特征"，然后把每个像素映射到一个特征坐标系中 (x, y, R, G, B) 或 (x, y, H, S, V)。
- `img.size`：返回的是图像总像素个数（高度 * 宽度 * 通道数）。
- `img.cols`、`img.rows`、`img.channels`：表示物理尺寸。
- `img.at<int>(row, col)`：访问像素。

## Mat 与 Scalar

- `cv::Size` 对象是 [列数 * 行数]。
- `cv::Mat` 对象是 [行数 * 列数]。
- Mat："动态类型"的矩阵容器，很灵活，但在编译时不知道自己存储的是什么数据类型。
- Mat_："静态类型"的模板类，在定义时就锁定了数据类型，使用起来很方便，更安全。
- Mat 负责创建矩阵，Scalar 负责给矩阵的颜色统一值。
- `Mat mat(行数, 列数, type, 颜色(Scalar(1,2,3)))`。
- 将矩阵赋值为同一颜色：
  ```cpp
  cv::Mat img = cv::Mat(img1.size(), CV_8UC3, cv::Vec3b(255,255,0));
  img.setTo(cv::Vec3b(255,255,0));
  ```
  注意：如果 img 为单通道就直接填数就行。
- 饱和度：图像颜色的鲜艳程度。饱和度越高，色彩越鲜艳（红的更红，绿的更绿）；饱和度越低，颜色显得越暗淡。
- 对比度：图片中最暗处和最亮处的亮度差异。

## Point 类型

- `cv::Point` 本身只能存储 int 类型。
- `cv::Point2f` 能放置 float。
- `cv::Point2d` 能放置 double。
- `cv::Rect` 里面存的是整数，`cv::Rect2d` 里面存的是小数。

## 类型转换

- 隐式类型转换：普通变量赋值（`int a = 10; float b = a`）、函数传参。
- 不能进行隐式类型转换：模板访问 `Mat::at<T>`。
- `img.convertTo(data, CV_32F)`：将 img 的值赋值给 data，并改变数据类型。
- `img.convertTo(ouputImg, 数据类型, alpha, belta)`：目标像素值 = 原像素值 * alpha + belta。
- 在数字后面加 f（例如 `3.153f`）表明这个数是单精度，否则是双精度。
- `cvtColor(inputImg, outputImg, 转换方式)`：将图片类型进行转换。注意：Gray 可以转换成 BGR，但无法恢复原始 BGR 图的彩色信息。

## 生成随机数

```cpp
RNG rng(12345);              // 输入随机种子
double x1 = rng.uniform(0,512);  // 生成一个 [0,512) 的 double 类型的随机数
int x1 = (int)rng.uniform(0,512); // 将 x1 的类型转换至 int
```

## Mat 数组的切片

- `Rect` 类型的 roi 只能是矩形，如果要提取非矩形区域，则用 mask 实现。
- ```cpp
  Rect rect;            // 先创建对象
  rect.x; rect.y; rect.height; rect.width;  // 对成员变量进行赋值
  ```
- `img(roi).clone()`：深拷贝，保证原图像不会受到干扰。
- ```cpp
  Mat robot = (Mat_<int>(2,2) << 1,0,0,-1);  // 通过 Mat 的模板子类快速创建一个 2*2 的矩阵
  ```

## 抠图与贴图

```cpp
// 抠图
Rect rect(0,0,200,200);
Mat img2 = img1(rect);

// 贴图
Rect rect(0,0,200,200);
Mat img2 = img1(rect);
img3.copyTo(img2);
```

## 头文件与命名空间

- `#include<opencv2/opencv.hpp>`：导入头文件。
- `using namespace cv;`：C++ 命名空间使用指令，让 cv 命名空间内所有标识符在当前作用域中"直接可见"。

## 图像文件的加载、显示与保存

```cpp
imread(图片地址, 读取图片的颜色格式(默认是彩色的))
```

- 读取模式：
  - `IMREAD_UNCHANGED` 加载透明通道
  - `IMREAD_GRAYSCALE` 加载灰色图像
  - `IMREAD_COLOR` 加载 BGR
  - `IMREAD_ANYCOLOR` 加载各种颜色的
- `namedWindow(窗口名称, 窗口属性)`：
  - `WINDOW_AUTOSIZE` 自动将窗口适配至图像大小，窗口不可修改
  - `WINDOW_FREERATIO` 可手动修改图像大小
  - `WINDOW_NORMAL` 大图，可手动修改
- `imshow(显示图片的窗口名, img)`：无法保存透明图像。imshow() 也可以自动创建图片，如果仅仅查看图片，就没必要使用 namedWindow()。
- `imwrite(address, img)`。
- `waitKey(0)`：
  ```cpp
  char mark = cv::waitKey(20);
  if (mark == 'q')   // 等号两边均为 char 类型
  ```
- `destroyAllWindows()`：删除所有窗口。
- `destroyWindow(要删除的窗口名)`：删除指定窗口。

### 图像类型

| img.type() | 对应的像素类型 |
| --- | --- |
| CV_8UC3（8 位无符号 3 通道） | Vec3b |
| CV_8UC1（8 位无符号 1 通道） | uchar |
| CV_32FC1（32 位浮点数 1 通道） | float |
| CV_32FC3（32 位浮点数 3 通道） | Vec3f |

OpenCV 常量，属于宏定义。

## Mat：图像文件的内存数据对象（底层是数据，应用是矩阵）

- Mat 分为头部和数据部分，头部存储的是图像的宽度、高度等信息；数据部分存储的是数据。
- ```cpp
  int width = img.cols;   // 获取列数
  int height = img.rows;  // 获取行数
  int dim = img.channels; // 获取块数
  int d = img.depth();    // 返回图像的深度（例如 CV_8U）
  int t = img.type();     // 返回深度 + 通道数
  img.size();             // 返回的是 (宽度, 高度)
  ```

### 创建 Mat 对象

```cpp
Mat m = Mat::zeros(img.size(), img.type());
Mat m = Mat::ones(img.size(), img.type());
// Mat::zeros() 和 Mat::ones 均属于静态函数
```

## 访问和修改像素值

### 1. 通过数组去访问或修改（速度慢，安全）

```cpp
Vec3b v = img.at<Vec3b>(row, col);  // 无符号 3 通道
uchar u = img.at<uchar>(row, col);  // 无符号 1 通道

// 修改
img.at<Vec3b>(row, col) = 10;
img.at<uchar>(row, col) = 10;
```

### 2. 使用指针去访问或修改（速度快，安全性低）

从内存上来看，数据在内存中的排序是先是第一行第一列蓝色、绿色、红色，第一行第二列蓝色绿色红色，以此类推，所以可以用指针的方式去访问和修改。

```cpp
for (int row = 0; row < height; row++) {
    uchar *p = img.ptr<uchar>(row);
    for (int col = 0; col < width; col++) {
        if (channel == 3) {
            int blue = *p++;
            int green = *p++;
            int red = *p++;
        }
    }
}
```

## 图像算术处理

### 1. 图像之间的加减乘除（Mat 也具有广播机制）

- 图像的 size 和 type 必须相同。
- 加法：`cv::add(img1, img2, res_img)` 和 `+`：自动做饱和处理，超过 255 的部分会自动截断为 255。
- 图像融合：`addWeighted(img1, α, img2, β, add_num, res_img)`，`res_img = img1 * α + img2 * β + add_num`。
- 减法：`cv::subtract(img1, img2, res_img)` 和 `-`：自动做饱和处理，低于 0 的部分会自动截断为 0。
- 乘法：`cv::multiply(img1, img2, res_img)` 和 `*`：默认情况下为逐元素相乘；`gemm()` 为矩阵相乘。注意：如果第一个矩阵的列数 = 第二个矩阵的行数，`*` 就会变为矩阵乘法。
- 除法：`cv::divide(img1, img2, res_img)`：自动饱和。

### 2. 赋值

- 在 OpenCV 中，把 Scalar 直接赋值给 Mat 对象时，创建新的矩阵，尺寸和被赋值的矩阵尺寸、类型完全一样，所有像素的通道值都被设置成 Scalar 里面的参数：`black = Scalar(40,40,40)`。
- `black.setTo(Scalar(40,40,40))`：将值直接赋给 black，并未创建新的矩阵。

## 图像位操作

- 相同图像做 AND/OR = 原图；做 XOR = 全黑；做 NOT = 反色。
- ```cpp
  cv::bitwise_not(input_img1, output_img, mask);       // 对图像进行取反
  cv::bitwise_and(input_img1, input_img2, output_img, mask); // 与运算
  cv::bitwise_or(input_img1, input_img2, output_img, mask);  // 或运算
  cv::bitwise_xor(input_img1, input_img2, output_img, mask); // 异或运算
  ```
- 注意：
  - 如果 mask 区域取的是 Mat()，表明创建一个空矩阵，告诉函数不限值操作区域，对图像/数组进行操作；如果不指明 mask 的值，默认也是整个图片。
  - mask 和输入的图片 size 必须相同（mask 的 type 必须为 CV_8UC1）。
  - mask 区域中的值为 255，原图会被处理给输出的图像。
  - 与/或/非/异或支持单通道和多通道图像，多通道时逐通道独立运算。
  - 双输入位运算要求两图通道数相同（无需都是单通道）。
  - 在使用 bitwise_and 和 bitwise_or 时，非 mask 区域的值会保持不变。

## 像素信息统计

### 最大值、最小值

- min_val 和 max_val 都是 double 类型的；minloc 和 maxloc 都是 Point 类型的（Point 是专门显示二维坐标的数据类型，可以通过实例化对象引出 x 坐标和 y 坐标）。
- `minMaxLoc(img, &min_val, &max_val, &minloc, &maxloc)`：要传入数据和点的地址。注意：minMaxLoc 求的是单通道的最大值最小值，如果要求多通道，则先将其分为单通道即可。

### 平均值

- `Scalar s = mean(img)`。注意：mean() 是对图片的四个通道求平均值，然后存储在 s 中；Scalar 是专门存储颜色的容器。

### 方差

- `meanStdDev(img, 均值, 方差)`。注意：均值和方差为 Mat 类型的 N 行 1 列的单通道（N = 通道数）矩阵，type 为 double。

## 图形绘制与填充

对于非空的图形，如果线宽为 -1，则变成实心的图形。

- 直线：`line(img, Point(x1,y1), Point(x2,y2), Scalar(0,0,255), 线宽(数字), 渲染方式(LINE_8 简称 8))`
- 矩形：需要先初始化关于矩形信息的实例化对象
  ```cpp
  Rect rect(x1, y1, 宽, 高);
  rectangle(img, rect, Scalar(0,255,0), 线宽, 渲染方式(8));
  ```
- 圆：`circle(img, Point(x1,y1), 半径, Scalar(255,0,0), 线宽, 渲染方式)`
- 椭圆：`ellipse(img, 椭圆中心: Point(x1,y1), 轴长: Size(长轴,短轴), 旋转角度, 椭圆起始角度: 一般为0, 椭圆终止角度: 一般为360, 颜色, 线宽)`
- 写文字：`putText(image, 文字, 初始点, 字体, 大小(一般为0), 颜色, 线条粗细, 渲染方式)`

## 图像通道的合并与分离

- 图像的分离：`split(img, mv)`：将 img 三个通道拆开，其中 mv 为 Mat 类型的 vector 容器。
- 图像的合并：`merge(mv, img)`：将三个图像进行合并。

## 图像直方图统计

将图像中的元素的值进行统计：

`cv::calcHist(&img, 图片数, 通道(灰色默认为 {0}，彩色的 BGR 分别为 {0}{1}{2}), mask, out_hist(一维/二维矩阵), dim(加工的通道数), histSize(每个维度多少格), ranges(每个维度的取值范围))`

- 如果 channels 是多参数，第一个参数表示横轴，第二个参数表示纵轴。
- 注意：bin 是把连续的像素值范围划分为若干个"区间"。

将得到的数据归一化（将一组数值缩放到指定的范围内，保留数据间相对大小关系）：

`cv::normalize(inHist, outHist, minnum(归一化后的最小值), maxnum(归一化后的最大值), 归一化的方式(NORM_MINMAX))`

## 直方图均衡化

- 目的：拉伸稀疏灰度区间，压缩密集灰度区间，最终让灰度分布更均匀，提高图像对比度。
- `equalizeHist(inputImg, OutImg)`：将"灰色"图像进行均衡化操作。

## 直方图反向投影（根据直方图投影到图片上）

- 反向投影是反向直方图模型在目标图像中的分布情况：用直方图模型去目标图像中寻找是否有相似的对象。
- 通常用 HSV 色彩空间的 H 和 S 两个通道直方图模型。
- 步骤：
  1. 先将原图进行提取直方图，再归一化：在直方图中横轴 H，纵轴 S。
  2. 接着提取输入图像中不同颜色的占比。
  3. 生成灰度图，越像目标颜色的区域越亮，越不像的颜色区域越暗。
- `calcBackProject(&inputImage, uimages, channels, 算好的目标直方图, 反向投影图, **range, scale放大倍数)`
  - 注意：反向投影的取值上限为 255，大于 255 的数会被截断，所以要对目标直方图进行归一化。

## 直方图比较

- 核心逻辑：如果两幅图像的内容相似，它们的直方图图形也会相似；反之则差异较大。
- 方法：先计算直方图，并归一化，再同 compareHist 进行比较。
- `compareHist(直方图1, 直方图2, 比较方法)`。
- 注意：两个直方图必须满足维度相同、bin 数量相同、数据类型一致。比较方法一般采用巴士距离（HISTCMP_BHATTACHARYYA），越距离 0，图片越相似。

## 图像查找表与颜色表

- 功能：预计算映射表 + 查表替换，用少量的内存消耗换取图像处理速度的大幅提高。主要应用于像素值映射、亮度/对比度调整、伪色彩可视化等。
- 1. 自定义颜色查找表：`cv::LUT(img, 颜色表colorImg, resImg)`。注意：如果要将灰色图片进行上色，需要将单通道灰度图转换成 3 通道灰度图（3 个通道值都等于原灰度图）。
- 2. 直接使用 OpenCV 预定义的颜色映射：`cv::applyColorMap(InputImg, OutputImg, colorMap)`。注意：输入图像必须是单通道 8 位灰度图；输出图像会自动生成 3 通道彩色图；colorMap 是预定义颜色映射类型（例：COLORMAP_JET）。

## 防溢出函数

- `saturate_cast<uchar>(num1 + num2)`：是模板函数。
- 核心保护机制：如果里面的值超过 255，自动截断为 255；如果小于 0，自动截断为 0。防止像素值溢出导致的图像失真。

## 窗口滑块

- 目的：方便调试参数。
- `cv::createTrackbar("滑块名", "窗口名", &value, maxNum, 回调函数)`。注意：窗口名必须提前创建。
- 回调函数模板：
  ```cpp
  void highThreshold(int, void*) {  // int 和 void* 没用，所以只需要占个位置就行
      cv::Mat img;
      // 处理过程.....
      cv::imshow();
  }
  ```

## 读取摄像头

```cpp
VideoCapture capture(0);   // 打开电脑自带摄像头
capture.isOpened();        // 判断摄像头是否打开
while (true) {
    bool ret = capture.read(frame);  // 将摄像头捕捉到的内容给 frame
    char c = waitKey(0);
    if (c == 27) {
        break;
    }
}
capture.release();  // 释放内存
```

## 视频读写

```cpp
VideoCapture capture(视频地址或网址);   // 读取视频
int fps = capture.get(CAP_PROP_FPS);   // 获取帧率
int width = capture.get(CAP_PROP_FRAME_WIDTH);    // 获取图片的宽度
int height = capture.get(CAP_PROP_FRAME_HEIGHT);  // 获取图片的高度
int numOfFrames = capture.get(CAP_PROP_FRAME_COUNT); // 获取图片的总帧数

int fourcc = cv::VideoWriter::fourcc('X', 'V', 'I', 'D');  // 获取 .avi 编码格式
VideoWriter writer(视频放入地址, type(视频的编码格式), fps, Size, 是否为彩色);
while (true) {
    bool ret = capture.read(frame);
    writer.write(frame);  // 写入视频
    char c = waitKey(0);
    if (c == 27) {
        break;
    }
}
capture.release();  // 释放内存
writer.release();   // 释放内存
```

## 色彩空间

| 色彩空间 | 用途 |
| --- | --- |
| RGB | 图像显示，基础图像处理 |
| HSV | 颜色分割，亮度/饱和度调整 |
| Lab | 颜色矫正，跨设备颜色匹配，医学图像处理 |
| YCrCb | 视频处理，数字摄影技术，图片/视频压缩与传输 |

Scalar 是给多通道图像赋值的通用工具。

### HSV 颜色阈值化

- `inRange(inputHSVImg, lowerb, upperb, outputBinaryImg)`：专门针对 HSV 图像的颜色阈值化函数。
  - 输出的是和输入图像同尺寸的单通道二值图。
  - lowerb 和 upperb 的范围均为左闭右闭。
  - lowerb 和 upperb 均为 Scalar(H, S, V)。
- `inRange(hsvImg, Scalar(0,30,30), Scalar(180,255,255), mask)`：只保留 S>30、V>30 的像素，避免阴影/强光影响。
- 当 S=0，就没有颜色之分了，只能看到黑灰白（由 V 决定）。

## 坐标系与极坐标

- 笛卡尔坐标系包含平面直角坐标系和空间直角坐标系。
- `cartToPolar(x, y, v, angle, angleInDegrees = false)`：将笛卡尔坐标系转化为极坐标系。
  - x 表示带有 x 坐标的数组。
  - y 表示带有 y 坐标的数组。
  - v 表示带有速度的数组。
  - angle 表示带有角度的数组。
  - `angleInDegrees = false` 输出弧度制。
  - `angleInDegrees = true` 输出角度。

## 用鼠标手动框选"感兴趣区域"

先框选感兴趣的区域，再按空格键或回车键确认。

`Rect roi = selectROI(windowName, img, showCrossshair = true, fromCenter = false)`

- windowName：指定窗口名。
- showCrosshair：是否画十字准星。
- fromCenter：是否从中心开始画。
  - 默认为 false：从鼠标按下的位置开始画框，松开的位置是右下角。
  - 设为 true：从鼠标按下的位置（中心），松开的位置是边缘，框会向四周扩散。

## 通道搬运与截取

- `hue.create(img.size(), img.depth())`：创建纯黑的图片 hue，但这个黑只是"暂时的占位"，马上会被覆盖。
- `mixChannels(&img1(原图像地址), 源图像个数, 目标图像地址, 目标图像个数, channels(搬哪几个通道), 要搬几个)`：将通道进行搬运（例如只搬运 HSV 中的 H，S 和 V 中为初始的纯黑，没有值）。
- `int channels[] = {源通道索引, 目标通道索引, 源通道索引, 目标通道索引.....}`
- `Mat roiHue = hue(trackWindow)`：截取目标图像中矩形部分。

## reshape 调整矩阵形状

`img = img1.reshape(通道数, 行数)`：用于调整矩阵形状，复杂度为 O(1)。

- 当通道数为 0 时，img1 与 img 的通道数相同。
- 1. 不复制数据，只改变视图。
- 2. 总元素数必须匹配。
- 3. 必须用返回值赋值。

## TermCriteria 迭代终止条件

TermCriteria 是 OpenCV 里通用的迭代终止条件类。

`TermCriteria(type, maxCount, epsilon)`

- type：终止条件的类型（选哪种规则停止）：`TermCriteria::EPS + TermCriteria::COUNT`。
  - `TermCriteria::COUNT`：表示迭代次数。
  - `TermCriteria::EPS`：表示精度。
- maxCount：最大迭代次数，例如 10。
- epsilon：精度阈值（变化量小于这个数就停止），例如 1.0。
- 第一个参数对应后两个，如果第一个参数中只有一个类型，只需要填对应的那个参数就行，另外那个随便填。

## resize 缩放

`resize(inputImg, outputImg, dsize, fx, fy, interpolation = cv::INTER_LINEAR)`

- dsize 为目标尺寸，格式为 Size(宽, 高)。
- fx、fy 为缩放因子（目标尺寸 = 原尺寸 * fx/fy）。

## 伪彩色

`applyColorMap(inputImg, outputImg, colormap)`：给灰度图上"伪彩色"。

- inputImg 为灰度图。
- outputImg 为彩色图。
- colormap 为颜色映射类型（COLORMAP_JET：蓝 -> 青 -> 黄 -> 红）。

## 其他工具函数

- `countNonZero(inputImg)`：统计矩阵中非零像素的总数量。inputImg 只能是单通道矩阵。
- 两个矩阵进行大小比较的规则：将两个矩阵逐元素进行比较，满足条件，这个位置设为 255；如果不满足，则设为 0。

## 特征匹配相关类型

### KeyPoint 与 Point 的区别

- KeyPoint 是 Point 的加强版。
- KeyPoint 是专门为特征检测设计的类型。
- KeyPoint 包含 Point、size（关键点的大小）、angle（关键点的方向）、response（关键点的辨识度高低）、octave（关键点所在的金字塔层数）。

### DMatch

- DMatch 中的 Query 通常为第一张图，Train 通常指第二张图。
- queryIdx 为第一张图的索引。
- trainIdx 为第二张图的索引。

## 透视变换

- `PerspectiveTransform(Points1, Points2, H)`：将 Points1 中的点经过 H 进行变换，变成 Points2。
- `warpPerspective(inputImg, outputImg, H, dsize, flags = INTER_LINEAR, borderMode = BORDER_CONSTANT, borderValue = cv::Scalar())`：专门用来执行透视变换的函数。
  - flags：
    - `cv::INTER_LINEAR`：双线性插值（平衡速度和质量）。
    - `cv::INTER_NEAREST`：最近邻插值（速度快，但画质差）。
    - `cv::INTER_CUBIC`：双三次插值（画质好，但速度慢）。
  - borderMode：
    - `cv::BORDER_CONSTANT`：用固定颜色填充，默认为黑色。
    - `cv::BORDER_REPLICATE`：复制边缘填充（将图像最边缘哪一行复制出去，填充空缺）。
    - `cv::BORDER_WARP`：平铺填充（将图片重复平铺出去）。

## 相关笔记

- [[C++OpenCV图像处理]]
- [[C++OpenCV图像检测]]
- [[C++ YOLO]]
- [[C++类和对象]]
