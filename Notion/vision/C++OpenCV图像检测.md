---
notion-id: 37f8cdcb-81ea-8002-af7c-f8eea7dc6e15
tags:
  - OpenCV
  - C++
  - 图像检测
  - 特征提取
  - 特征匹配
  - 图像分割
  - 目标跟踪
  - 霍夫变换
  - 角点检测
  - SIFT
  - ORB
  - 光流
  - RANSAC
  - Harris
  - 分水岭
  - GrabCut
  - GMM
  - K-Means
  - 距离变换
---

# C++ OpenCV 图像检测

## 1. 霍夫变换

### 1.1 霍夫直线检测

```cpp
HoughLines(binaryImg, lines, rho, theta, threshold)
```

- `lines`：输出向量，每个元素是 `Vec2f`，包含 `(ρ, θ)`
- `rho`：参数空间中 ρ 的分辨率，通常为 1
- `theta`：参数空间中 θ 的分辨率，通常为 `CV_PI / 180`
- `threshold`：累加器阈值，表示至少需要多少个点在一条直线上才能被检测为直线

```cpp
HoughLinesP(BinaryImg, lines, rho, theta, threshold, minLineLength, maxLineGap)
```

- `lines` 中元素的数据类型为 `Vec4i` -> `[x1, y1, x2, y2]`
- `minLineLength`：最小线段长度
- `maxLineGap`：最大线段间隔，小于该间隔的线段均会被连接

### 1.2 霍夫圆检测

```cpp
HoughCircles(binaryImage, circles, method, dp, minDist, param1 = 100, param2 = 100, minRadius = 0, maxRadius = 0)
```

- `method`：霍夫变换的方法，一般为 `HOUGH_GRADIENT`
- `dp`：累加器分辨率和图像分辨率的比值，一般为 1
- `minDist`：检测到的圆心之间的最小距离
- `param1`：Canny 边缘检测的高阈值，需要知道 Canny 边缘信息才能工作
- `param2`：累加器阈值
- `minRadius` / `maxRadius`：圆的最小/最大半径

## 2. 图像分割

图像分割：将图像切分为不同区域，目的是将感兴趣的部分进行提取。

### 2.1 K-Means 方法

K-Means 将颜色相似的像素归为同一区域。

```cpp
kmeans(data, K, bestLabels, criteria, attempts, flags, centers = noArray())
```

- `data`：所有像素的 RGB 值集合，必须是 `CV_32F`，格式为（宽 × 高）行、3 列，每列对应 R/G/B 一个颜色通道
- `K`：要分的"颜色堆"数量。二值分割（前景/背景）设 2，多区域分割常用 4/8
- `bestLabels`：每个像素的"堆标签"，输出整型（`CV_32S`）矩阵，大小为（宽 × 高）行、1 列，值为 0~k-1，表示该像素属于第几个颜色堆
- `centers`：每个颜色堆的"代表色"，输出浮点矩阵，大小为 K 行、3 列，每行对应一个堆的 BGR 分别的平均值
- `criteria = TermCriteria(TermCriteria::EPS + TermCriteria::COUNT, 10, 1.0)`：迭代终止条件，迭代十次或质心移动距离小于 1.0
- `attempts`：随机初始化质心的尝试次数，次数越多结果越稳定，但计算慢 -> 3~10
- `flags`：质心初始化方式
  - `KMEANS_RANDOM_CENTERS`：随机初始化 -> 最常用
  - `KMEANS_PP_CENTERS`：KMeans++ 初始化（更优但稍慢）

### 2.2 GMM（高斯混合模型）方法

GMM 也是根据颜色进行划分，与 K-Means 有所不同。GMM 模型就是根据 EM 类中的算法反复磨练得出不同的聚类（和神经网络不是一回事）。EM 是 ml 模块中封装了 EM 算法的工具类，GMM 模型是要训练的目标（空模型），通过 EM 类来创建。

创建对象：

```cpp
cv::Ptr<cv::ml::EM> gmm = cv::ml::EM::create();
```

- `Ptr` 是 OpenCV 封装的智能指针，能够自动管理内存，无需手动释放
- `create()` 是 EM 中的静态函数，用来返回一个 `Ptr<EM>` 类型的智能指针

设置参数：

```cpp
gmm->setClustersNumber(k)         // 设置聚类数量：分离背景和前景设为2，复杂场景设为3~5
gmm->setCovarianceMatrixType(type) // 设置协方差矩阵类型：EM::COV_MAT_DIAGOBAL -> 对角协方差，最常用，计算快
gmm->setTermCriteria(TermCriteria tc) // 设置迭代终止条件：迭代次数100，精度1e-6
```

核心训练/预测 API：

```cpp
trainEM(sample, logLikelihoods = noArray(), labels = noArray(), prob = noArray()) // 将所有元素进行分类
predict2(sample, probs) // 给定一个新像素，判断靠近哪个聚类
```

- `trainEM`：`sample` 为像素数据，数据类型 `CV_32F`，每一行样本有 BGR 三列；`labels` 为每个样本聚类标签，类型 int；`probs` 为每个样本属于各聚类的概率，类型 float
- `predict2`：`sample` 为单个像素，`probs` 为属于各聚类的概率

### 2.3 分水岭分割方法

- **优势**：能够精准分割"颜色相似、空间上粘连/重叠"的物体
- **局限性**：只能分割同质（硬币等）、粘连、规则形状、背景简单的物体
- **原理**：根据梯度图中梯度的大小变化确定边缘（颜色相似的粘连物体，边缘的梯度变化一定大）
- **步骤**：
  1. 先对图像进行去噪操作
  2. 将内部纹路进行填充
  3. 生成分水岭 markers（将原二值图进行膨胀 - 已经确定的前景图像 -> 白色区域就是未确定的区域）
  4. 放入 `watershed` 中再对未区分区域进行划分
  5. 对边缘进行显示

```cpp
watershed(inputImg, markers)
```

- `inputImg` 必须为 8 位 3 通道彩色图像（BGR 格式）
- `markers` 必须为 float 类型的单通道矩阵
  - 输入时：`0` 背景；`1,2,3...` 确定前景（每个物体一个唯一正整数）；`-1` 未知区域（需要算法分割的边界）
  - 输出时：将边界标记为 -1，其他区域保持原来的标记

### 2.4 GrabCut 方法

计算量大，建议将图像进行缩小化。

- **简介**：基于图像分割的交互式分割算法，核心是"用户画一个矩形将目标框住，算法自动把框里的目标和背景分开"
- 适用于复杂形状、非规则、异质物体（人、手机、动物）
- **核心原理**：高斯混合模型 + 图割

```cpp
grabCut(inputImg, mask, rect, bgdModel, fgdModel, iterCount, mode = cv::GC_EVAL)
```

- `inputImg`：BGR 彩色图像
- `mask`：输入输出 mask 矩阵
  - `GC_BGD(0)`：肯定是背景
  - `GC_FGD(1)`：肯定是前景
  - `GC_PR_BGD(2)`：可能是背景
  - `GC_PR_FGD(3)`：可能是前景
- `bgdModel` / `fgdModel`：OpenCV 内部临时存储。要看背景和前景时，要根据 mask 手动分割图像
- `iterCount`：迭代次数
- `mode`：
  - `cv::GC_INIT_WITH_RECT(0)`：用矩形框初始化（第一次用 GrabCut 必备）。初始化时框外所有像素强制设为 `GC_BGD`，框内所有像素强制设为 `GC_PR_FGD`；mask 结果会有 0,1,2,3，我们将 1 和 3 视为前景
  - `cv::GC_INIT_WITH_MASK(1)`：用自定义 mask 初始化（手动标记修正结果时用）。适合对第一次结果不满意，手动标记修正（一般是给定 mode=1 输出的 mask，GMM 会重新初始化）
  - `cv::GC_EVAL(2)`：仅迭代优化（已经初始化，想再跑几次让结果更准）。不再重新初始化，直接用当前 mask、bgdModel、fgdModel 继续迭代优化
  - 技巧：如果 mode=1 跑了 5 次觉得效果不错，想要更加精细就适合 mode=2

### 2.5 K-Means 与 GMM 的区别与相似

| 对比项 | K-Means | GMM |
| --- | --- | --- |
| 划分方式 | 硬划分，每个像素只能在一个聚类中 | 软划分，统计每个像素在不同聚类的概率 |
| 聚类形状 | 圆（形状较为固定） | 椭圆（形状更加灵活） |
| 时间 | 所需时间少 | 所需时间多 |

相似处：都只看颜色是否相似而忽视距离；均为无监督学习。

### 2.6 分水岭与轮廓分析的对比

轮廓分析只能分"不粘连的连通区域"，分水岭能分"粘连在一起、但有梯度边界的相似物体"。

## 3. 距离变换

```cpp
distanceTransform(inputImg, outputImg, distanceType, maskSize)
```

计算二值图中每个白色像素到最近黑色像素的距离。

- `inputImg`：数据类型为 `CV_8UC1`，是原图片经过二值化 + 开运算之后的图像
- `outputImg`：数据类型为 `CV_32FC1`，不能直接显示，需要归一化到 0~255 才能看（矩阵中数据的含义是二值图中每个白色像素到最近黑色像素的距离）
- `distanceType`：距离计算方法
  - `DIST_L2`：欧式距离（最常用，精度高，符合视觉）
  - `DIST_L1`：曼哈顿距离（快，但精度低）
- `maskSize`：3（快，但精度低）；5（精度高、速度还行，工程首选）

## 4. 特征点提取

特征点是经过算法分析出来的、含有丰富局部信息的点，经常出现在图像中拐角、纹理剧烈变化等地方。特征点不仅是一个点，还可能包含一系列局部信息，甚至很多情况下是具有面积的一小块区域。

特征点拥有的性质：旋转或尺度不变性，或小的仿射变换的不变性。其中"尺度不变性"指其在不同图片中能够被识别出来并具有统一性质。

用途：

1. 识别算法判别两个物体是否属于一类
2. 帮助跟踪算法进行跟踪计算
3. 帮助定位算法进行定位处理
4. 支持图像拼接算法进行拼接构造

`cv::Feature2D` 是 OpenCV 中所有 2D 图像特征检测器和描述符提取器的统一抽象类。它本身是抽象类，不能实例化，需使用其具体的子类，如 SIFT、ORB、BRISK、AKAZE 等。

```cpp
#include <opencv2/Feature2D.hpp>
```

通用函数：

```cpp
Ptr<算法类型> 指针名 = 算法类型::create(....);
detect(inputImg, keyPoints, mask)                        // 仅检测关键点（keyPoint 为输出的关键点）
compute(inputImg, keyPoint, descriptors)                 // 仅计算描述子
detectAndCompute(inputImg, mask, keyPoints, descriptors, useProvidedKeypoints = false) // 检测 + 计算
```

- `compute`：`keyPoint` 为输入的已知关键点，`descriptors` 为输出的描述子矩阵（描述子是一个 128 维的向量；同一个像素点不管图像怎么旋转、缩放、变亮变暗，其描述子几乎一模一样）
- `detectAndCompute`：`useProvidedKeypoints` 若为 true，则使用传入的 keyPoints，不需要重新检测
- 注意：`KeyPoint` 为 `vector<KeyPoint>` 的对象

```cpp
drawKeyPoint(inputImg, KeyPoint, outputImg, color = Scalar::all(-1), flags = DrawMatchesFlags::DEFAULT)
```

- `Scalar::all(-1)` 给每个关键点随机分配不同颜色
- `flags`：
  - `DrawMatchesFlags::DEFAULT`：只画一个简单的小圆圈
  - `DrawMatchesFlags::DRAW_RICH_KEYPOINTS`：画一个带大小的圆，并画一条线表示方向

### 4.1 角点检测

在连续的图像移动或图像拼接中，都要求检测角点作为特征点。

#### Harris 角点检测

原理：在角点处，随便移动一个小窗口，窗口里的灰度都会明显变化。

```cpp
cornerHarris(inputGrayImg, outputImg(CV_32FC1), blockSize(通常为3，角点密集可以选5), ksize(常用3，和blockSize), k(0.04 ~ 0.06，先尝试0.04), borderType = BORDER_DEFAULT)
```

- 返回的 `CV_32FC1` 图，每个像素存 R 值：
  - 正的且大 -> 角点
  - 接近 0 -> 平坦
  - 负的且绝对值大 -> 边缘
- 将得到的值归一化 -> `normalize()`，再转换数据类型 -> `convertScaleAbs()`
- 对角度、亮度变化不敏感，但对尺度变化敏感，精度低
- 补充：可能会出现多个点扎堆的情况，所以需要用到 NMS（Non-Maximum suppression）-> 不是最大的都抑制掉，只留最好的、把旁边重复的都删掉

#### Shi-Tomasi 角点检测（Harris 的进阶版，更常用）

思想：两个特征值都大，才是真的角点。

```cpp
goodFeaturesToTrack(inputGrayImg, corner, maxCorners, qualityLevel, minDistance, mask = noArray(), blockSize = 3, useHarrisDetector = false, k = 0.04)
```

- `corners`：检测到的角点，存成 `vector<Point2f>`
- `maxCorners`：最多检测到的角点数量
- `qualityLevel`：只保留响应值大于"最大响应值 × qualityLevel"的角点，一般为 0.01~0.1
- `minDistance`：角点之间的最小距离
- 优点：不需要调 k，对钝角更敏感，自带 NMS，对旋转、亮度变化不敏感
- 缺点：对尺度变化敏感，不精确（达不到亚像素级别）

#### 亚像素角点检测

在像素级角点检测的基础上，通过数学建模与优化算法，将角点坐标精度从整数像素级别提升至亚像素级别的数字图像处理技术。只是提升已有角点的定位精度，不会发掘更多的点。

```cpp
cornerSubPix(inputGrayImg, corners, winSize, zeroZone, criteria)
```

- `corners`：
  - 输入：像素级初始角点（通常来自 `goodFeaturesToTrack` / `findChessboardCorners`）
  - 输出：优化后的亚像素角点
- `winSize`：搜索窗口的半边长，通常为 `Size(5,5)` / `Size(11,11)`
  - 窗口越大，抗噪性越强，但计算量增大、易受邻域边缘干扰
  - 窗口越小，抗噪性越弱，计算快
  - 角点距离近时要缩小 winSize，避免多个角点邻域窗口重叠相互干扰
- `zeroZone`：中心死区的半边长，通常为 `Size(-1,-1)`
- `criteria`：最大迭代次数 30、精确度 0.001；或最大迭代次数 20、精确度 0.01
- 注意：容易受到噪声影响，不自带 NMS

如果用到亚像素级别角点检测，corners 为 `vector<Point2f>` 的对象。

#### FAST

一种专门用于快速检测图像角点的算法，无法生成特征描述子。FAST 在 OpenCV 主模块 features2d 里。

```cpp
cv::Ptr<cv::FastFeatureDetector> fast = cv::FastFeatureDetector::create(threshold = 10, nonmaxSuppression = true, type = cv::FastFeatureDetector::TYPE_9_16)
```

- `threshold`：亮度差阈值，值越大检测的点越少
- `nonmaxSuppression`：是否开启非极大值抑制
- `type`：检测模式。`TYPE_9_16`：用 16 个像素、连续 9 个满足条件（最常用）

```cpp
fast->detect(img, keypoints); // 检测角点，用 KeyPoint 装角点
```

### 4.2 ORB 算法

ORB 是 FAST 和 BRIEF 的结合升级版（具有尺度不变性和旋转不变性）。

```cpp
cv::Ptr<cv::ORB> orb = cv::ORB::create(nfeatures = 500, scaleFactor = 1.2f, nlevels = 8, edgeThreshold = 31, firstLevel = 0, WTA_K = 2, scoreType = cv::ORB::HARRIS_SCORE, patchSize = 31, fastThreshold = 20);
```

- `nfeatures`：最多保留的特征点数量
- `scaleFactor`：控制不同大小的特征。默认 1.2；1.1 每层图片缩小得慢；1.5 每层缩小得快，但会漏掉细节
- `nlevels`：金字塔层数，默认 8。层数越多（16）检查的尺度范围越大但计算慢；层数越少（4）只能检测某几种大小的特征
- `edgeThreshold`：避开边缘上的假特征点，默认 31。设得越大（50）排除的点越少、保留的越多；设得越小（15）排除的点越多、保留的越少
- `patchSize`：计算描述子时用的邻域大小，一般和 edgeThreshold 匹配
- `WTA_K`：生成描述子时用几对像素。默认 2：每两对像素生成 1 位二进制数；设为 3 或 4 生成的描述子区分度更高，但计算量大
- `scoreType`：判断特征点好坏的方式。`HARRIS_SCORE`：用 Harris 算法给角点评分，更稳定
- `fastThreshold`：FAST 角点检测的亮度差阈值，值越大检测到的点越少

在进行距离匹配时，通过 `NORM_HAMMING` 检测进行像素点匹配并计算距离。

### 4.3 SIFT 算法（尺度不变特征变换）

- **核心本质**：从图像中提取出不受尺度缩放、旋转、光照变换、视角变换、局部遮挡、噪声干扰的稳定关键点
- **目的**：让计算机在两张有变化的图里能够精准找到同一物理点
- **核心步骤**：
  1. 尺度空间极值检测 -> 实现尺度不变（为了让计算机不管物体放大还是缩小都能找到同一个点）
  2. 关键点精修与过滤（为了剔除不稳定点，提升抗干扰能力）
  3. 关键点方向赋值 -> 实现旋转不变性（关键点的主方向由关键点邻域像素本身的灰度特征决定，和图像的整体方向无关）
  4. 特征描述子生成（为了将关键点的特性唯一表示出来）

```cpp
Ptr<SIFT> sift = SIFT::create(nfeatures = 0, nOctaveLayers = 3, contrastThreshold = 0.04, edgeThreshold = 10, sigma = 1.6);
```

- `nOctaveLayers`：SIFT 图像金字塔中每组的层数，默认 3，要精度更高可设为 4
- `contrastThreshold`：过滤对比度低的"弱特征点"，默认 0.04。设得越大（0.1）过滤越狠，越小（0.01）过滤越松
- `edgeThreshold`：过滤图像边缘上的"不稳定特征点"，默认 10。设得越大（20）边缘上的特征点越多，越小（5）越少
- `sigma`：初始高斯模糊 sigma 值，默认 1.6。设得越大（2.0）原图模糊得狠、细节保留少；越小（1.0）原图模糊得轻、细节保留多

### 4.4 SURF 算法

一种快速的图像特征点检测与描绘算法（需要装 OpenCV_contrib 才能实现）。

```cpp
cv::Ptr<cv::SURF> surf = cv::SURF::create(HessianThreshold, nOctaves, nOctaveLayers, extended, upright)
```

- `HessianThreshold`：Hessian 矩阵阈值，通常 100~1000（阈值越大检测到的点越少），刚开始为 400~800
- `nOctaves`：金字塔组数，通常为 4
- `nOctaveLayers`：每组金字塔的层数，通常为 3
- `extended`：扩展描述符。false 时特征描述符 64 维，true 时 128 维
- `upright`：是否为直立模式。true 时不具备旋转不变性，false 时具备

### 4.5 BRIEF / BRISK / FREAK

- **BRIEF**：一种二进制特征描述子（用 0 和 1 组成的二进制码描述图像中某个关键点周围的局部特征）
  - 优点：速度快、内存小
  - 缺点：不具备尺度不变性和旋转不变性，对噪声敏感
- **BRISK**：同时具备尺度不变性和旋转不变性的二进制特征描述子（BRIEF 的加强版）
- **FREAK**：快速进行特征提取并转换成二进制特征描述子（计算速度快），需要 OpenCV_contrib

### 4.6 SURF 与 SIFT 对比

- SURF 的速度是 SIFT 的 3~10 倍
- SIFT 的精度略高于 SURF
- SIFT 更倾向于找角点、边缘交叉处；SURF 更倾向于找斑点
- 两者在特征匹配的代码一样

### 4.7 SIFT 与 ORB 的优缺点

- ORB：速度更快，占用资源少
- SIFT：精度高，尺度/旋转不变性强，对视角/光照变化大的适配性强一些

## 5. 特征匹配

### 5.1 暴力匹配（BFMatcher）

原理：将描述子进行挨个匹配。

```cpp
Ptr<BFMatcher> matcher = BFMatcher::create(NORM_L2);
vector<vector<DMatch>> knn_matches;
matcher->knnMatch(descriptors1, descriptors2, knn_matches, k);
```

- `BFMatcher`（暴力匹配器）是 cv 的子类，用于匹配描述子的欧氏距离（`NORM_L2`）。BFMatcher 即使没有一样的特征点也会硬找
- `DMatch` 为专门记录匹配结果的类型（图一的哪个点和图二的哪个点的距离）
- `knnMatch`：将距离图一的像素最近的 k 个点提取出来
- 进行最近邻距离比筛选，过滤掉错误匹配、保证这个点匹配的唯一性：只有当第一名距离 < 0.7 × 第二名距离时，才认为这个匹配靠谱（如果最近的点为 10，第二近的为 100，则会将第二个点舍去）

注意：

- `matcher->match()`："一对一"匹配，计算两点距离，快速但筛选效果差
- `matcher->knnMatch()`："K 近邻"匹配，找最相似的 K 个描述子，稍慢但筛选效果好

在进行特征匹配时：

1. 如果描述子为二进制，就使用 `NORM_HAMMING`（汉明检测）
2. 如果描述子为非二进制，就使用 `NORM_L2`（欧式距离）

### 5.2 FLANN

FLANN：快速找两个描述子集合之间最相似的匹配对。

过程：

1. 先将一侧的所有描述子建成一个智能索引树：
   - 浮点型描述子（SIFT/SURF）-> KD_Tree 或 Hierarchical K-Means Tree
   - 二进制描述子（ORB/BRIEF）-> LSH
2. 快速搜索

浮点型描述子直接用：

```cpp
cv::FlannBasedMatcher matcher;
```

二进制描述子必须专门设置 LSH 参数：

```cpp
cv::Ptr<cv::flann::IndexParams> indexParams = cv::makePtr<cv::flann::LshIndexParams>(table_number = 6, key_size = 12, multi_probe_level = 1); // 告诉FLANN怎么建立智能索引树
cv::Ptr<cv::flann::SearchParams> searchParams = cv::makePtr<cv::flann::SearchParams>(50); // 控制搜索的精度和速度
cv::FlannBasedMatcher matcher(indexParams, searchParams);
```

- `table_number`：建立多少个哈希表，一般为 6-12。哈希表越多，找到正确匹配的概率越高，内存占用越大
- `key_size`：每个哈希表的键长，一般为 10-20
- `multi_probe_level`：搜索时是否要看隔壁树杈，设 1/2 会看几个近邻，正确率提高但稍微变慢
- `SearchParams`：数值越大（100）搜索越精准但速度变慢；数值越小（30）搜索越快但精度变低

注意：使用 knnMatch 匹配相同点时，每个点对应的匹配点数量可能达不到预期，以至于越过边界导致报错。

### 5.3 基于特征值检测定位物体

将小图中的物体定位到大图。

```cpp
cv::Mat H = cv::findHomography(Points1, Points2, cv::RANSAC, 1.0, mask); // 获取单应性矩阵H，记录小图任意点对应到大图的位置
cv::PerspectiveTransform(resPoints1, resPoints2, H/HInv);
```

- 默认第一个容器中的点为小图的点，第二个容器中的点为大图的点；如果反过来，就需求 H 的逆矩阵（`HInv = H.inv()`）
- 获取小图中想要定位到大图的点组成的容器 `resPoints1`
- 注意：传入的参数的数据类型必须是 `Point2f`

### 5.4 绘制匹配点

```cpp
drawMatches(inputImg1, KeyPoints1, inputImg2, KeyPoints2, matches1to2, outputImg, matchColor = cv::Scalar::all(-1), singlePointColor = Scalar::all(-1), matchesMask = vector<char>(), flags = DrawMatchesFlags::DEFAULT)
```

- 将两张图片拼在一起，并用线把匹配上的关键点连起来
- `matches1to2`：匹配结果
- `matchColor`：匹配线和匹配点的颜色
- `matchesMask`：指定画哪些匹配，和 good_matches 一样长的 char 数组，1 表示画这对匹配，0 表示不画
- 注意：函数本身不会对 `matches1to2` 中的匹配点进行决策，需要我们处理好（一维容器中最好只有一组匹配点）

## 6. 目标跟踪

### 6.1 三帧差法

原理：检测视频序列中哪些像素发生了变化，变化的地方就是"运动的物体"。主要使用 `absdiff(grayImg1, grayImg2, resImg)` 找变化的像素。

- 优点：速度快，对光照不敏感
- 缺陷：容易产生孔洞；对物体的运动速度敏感（动得太快会检测出两个物体，动得太慢会检测不出物体）

### 6.2 背景减除法

在固定摄像头场景下做运动目标检测 & 跟踪（最经典、最实用的算法）。同样适用于背景分析：在图像处理/视频分析中，把场景里的"固定不变的部分（背景）"和"变化/感兴趣部分（前景）"分开的技术。

#### MOG2

- **核心原理**：给每个像素点建立多个高斯分布，区分"长期不变的背景分布"和"偶尔出现的前景分布"，自带阴影检测能力
- **特征**：对光照渐变、轻微背景晃动鲁棒性更好，参数少；能检测阴影，默认把阴影标记为 127，前景为 255，背景为 0

```cpp
cv::Ptr<BackgroundSubtractorMOG2> bgSubtractor = createBackgroundSubtractorMOG2(history = 500, varThreshold = 16, detectShadows = true);
bgSubtractor->apply(frame, fgMask); // 生成前景掩码
```

- `history`：历史帧数。场景稳定（1000），场景变化快（200）
- `varThreshold`：方差阈值。画面噪点多（25-30），小目标检测不到（8-10）

#### KNN

使用 K 近邻算法，判断每个像素是和历史背景更像，还是和新出现的内容更像。

- 优点：比 MOG2 对复杂背景、光照突变的鲁棒性更强，能检测阴影，对前景目标的边缘检测更精准，噪点更少
- 缺点：对密集动态背景处理差，计算量大

```cpp
cv::Ptr<cv::BackgroundSubtractorKNN> bgSubtractor = cv::createBackgroundSubtractorKNN(history = 500, dist2Threshold = 400, detectShadows = true);
bgSubtractor->apply(frame, mask);           // 获取前景掩码
bgSubtractor->getBackgroundImage(image);    // 获取当前背景模型图像
```

- `history`：历史帧数
- `dist2Threshold`：距离阈值，判断是前景还是背景
- `detectShadows`：是否检测阴影

如果出现画面不稳定的情况，可以让前 n 帧只进行学习、不进行展示。

### 6.3 光流分析

可以检测背景和前景，但核心是计算像素运动（运动目标跟踪、动作视频、视频稳定）。

光流：视频里每个像素的"运动轨迹"，即运动方向和速度（光流为 0 即为不动）。

光流法三大假设：

1. **亮度恒定假设**：同一个物体的像素在不同帧里亮度差不多，不会突然变暗变亮
2. **小运动假设**：像素移动得很慢，不会从这一帧的左上角直接跳到右下角
3. **空间一致**：相邻的像素运动方式相似（因为它们属于同一物体表面）

- **稀疏光流**：只给"人为选中的少数关键特征点"算光流，完全不碰图里的其他像素
- **稠密光流**：给图里的每一个像素都算光流，输出全图的运动矢量图

#### 稀疏光流（KLT）

KLT 仅跟踪图像中指定的特征点（如角点）。

```cpp
calcOpticalFlowPyrLK(prevGrayImg, nextGrayImg, prevPts, nextPts, status, err, winSize = Size(21,21), maxLevel = 3, TermCriteria criteria = TermCriteria(TermCriteria::COUNT + TermCriteria::EPS, 30, 0.01), flags = 0, minEigThreshold = 1e-4)
```

- `prevImg` -> 前一帧图像，`nextImg` -> 后一帧图像
- `prevPts` -> 前一帧特征点，`nextPts` -> 后一帧特征点，均为 `vector<cv::Point2f>`
- `status` -> 跟踪状态，`vector<uchar>`，1 = 成功，0 = 失败
- `err` -> 跟踪误差，`vector<float>`
- `winSize` -> 搜索窗口大小（越大越能跟踪大运动，但计算量增加）。运动快就调大（31/41），运动慢就调小（11/15）
- `maxLevel` -> 金字塔层数，0 为不使用金字塔，3 为常用值。金字塔光流先将图像逐层缩小（金字塔顶层）在小图上跟踪大运动，再逐层放大（金字塔底层）细化跟踪位置，实现"从粗到细"的稳定跟踪。高速运动调 4/5，极慢就用 0/1
- `criteria` -> 迭代终止条件（最多 30 次，或精度达 0.01）
- `flags` -> 操作标志（默认 0 即可）
- `minEigThreshold` -> 最小特征值阈值（过滤不稳定的点）

#### 稠密光流

算法会构建一个"能量函数"，包含：

1. **数据项**：惩罚不满足"亮度恒定"假设的像素
2. **平滑项**：惩罚那些和周围邻居运动方向不一样的像素（保证空间一致性）

```cpp
calcOpticalFlowFarneback(prevGrayImg, nextGrayImg, flow, pyr_scale, levels, winsize, iterations, poly_n, poly_sigma, flags)
```

- `flow`：输出光流图，双通道 32 位浮点数
  - `flow.at<Vec2f>(y,x)[0]` 是 x 方向位移，`flow.at<Vec2f>(y,x)[1]` 是 y 方向位移
- `pyr_scale`：图像金字塔缩放比例，通常为 0.5，即每次缩放的倍数（决定分辨率缩放的"激进程度"）。设得越小金字塔层数越多，能处理更快的运动，但计算量也越大
- `levels`：金字塔层数，设为 1 表示只使用原始图像，通常设为 3~5（缩放的次数，决定在几种不同大小的尺度上计算光流）
- `winsize`：平均窗口大小（推荐 15）。设得大（21 或 31）会看到更大区域但光流细节少；设得小（5 或 7）能捕捉细节但容易产生乱流
- `iterations`：每层金字塔上的迭代次数，通常 3~10，一般 3
- `poly_n`：用于多项式展开的邻域大小，通常为 5
- `poly_sigma`：高斯平滑标准差，通常为 1.1
- `flags`：操作标志，通常为 0

### 6.4 均值迁移（MeanShift）

基于密度梯度的非参数迭代算法，目标是找到数据分布中密度最大的区域（局部峰值）。

1. 给一个固定大小的搜索窗口
2. 计算窗口内所有像素的"特征均值"
3. 把窗口移动到这个均值位置
4. 重复直到收敛，窗口位置不变

应用场景：图像分割/平滑、目标跟踪（仅适用于目标大小变化不大的情况）。

```cpp
MeanShift(probImage, window, cv::TermCriteria(cv::TermCriteria::EPS | cv::TermCriteria::COUNT, 10, 1.0))
```

- `probImage`：反向投影图
- `window`：搜索窗口，函数运行完会自动修改窗口的属性

### 6.5 自适应迁移（CamShift）

适用于目标跟踪。

```cpp
CamShift(probImage, window, cv::TermCriteria(cv::TermCriteria::EPS | cv::TermCriteria::COUNT, 10, 1.0))
```

- `probImage`：反向投影图
- `window`：搜索窗口，函数运行完会自动修改窗口的属性
- 不适用场景：
  1. 目标颜色和背景颜色过于相似
  2. 目标被完全遮挡，CamShift 会失效
  3. 光照变化特别剧烈

### 6.6 跟踪器

使用前需导入头文件 `#include <opencv2/tracking.hpp>`。

#### CSRT

特点：精度高，抗光照变化能力强，速度稍慢（能做到实时，但只有 20 帧）；自适应多尺寸估计，框会跟着目标自动缩放；空间可靠性图，自动忽视被遮挡的部分。

```cpp
TrackerCSRT::Params params;
params.use_hog = true;          // 是否使用HOG特征（对边缘和形状非常敏感，是追踪行人、车辆等明显物体主力）
params.use_color_names = true;  // 是否使用颜色名称特征（目标颜色鲜艳且背景不乱时非常有效）
params.template_size = 200;     // 模板的最大尺寸校址（可调成150-100用于提速）
params.filter_lr = 0.02f;       // 函数学习率（目标改变快时可调成0.05）
params.padding = 3.0f;          // 区域搜索的扩展倍数（目标运动快可调成4.0-5.0，运动慢可调成2.0）

cv::Ptr<cv::TrackerCSRT> tracker = TrackerCSRT::create(params); // 创建跟踪器
tracker->init(frame, targetBox);   // 初始化跟踪器，传入第一帧和目标框
tracker->update(frame, targetBox); // 传入之后的图片并返回跟踪框
```

#### MOSSE

需要包含头文件 `#include <opencv2/tracking/tracking_legacy.hpp>`。

特点：速度最快但精度低、抗干扰性差（适合嵌入式设备、对精度要求低的场景）；追踪框大小不会变化，只看灰度图，抗遮挡能力差。

```cpp
cv::Ptr<legacy::TrackerMOSSE> mosse = legacy::TrackerMOSSE::create(); // 创建追踪器
mosse->init(frame, boundingBox);  // 初始化目标（MOSSE没有适应能力，这是唯一能训练它的机会）
mosse->update(frame, boundingBox);
```

## 相关笔记

- [[C++OpenCV基础总结]]
- [[C++OpenCV经验总结]]
- [[C++OpenCV图像处理]]
- [[C++ YOLO]]
- [[C++类和对象]]
