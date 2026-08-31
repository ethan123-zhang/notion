---
notion-id: 37f8cdcb-81ea-80d4-89d2-d59250a09af6
tags:
  - OpenCV
  - C++
  - 图像处理
  - 卷积
  - 滤波
  - 边缘检测
  - 图像分割
  - 阈值化
  - 轮廓
  - 锐化
  - 去噪
  - 高斯滤波
  - 中值滤波
  - Canny
  - 轮廓匹配
---

# C++ OpenCV 图像处理

> 卷积归一化：使图片平滑、模糊，降低噪声；未归一化：提取边缘、增强细节，`ddepth` 一般为 `CV_32F`。
>
> 滤波的核心目标 **不等于** 模糊一切。滤波的真正目的是去除"无意义的噪声"、保留"有意义的结构"，而边缘恰恰是图像中最有价值的结构信息之一。

## 1. 图像卷积

作用：对图像进行平滑处理（模糊处理），即抹平细节、降低噪声。

```cpp
cv::blur(inputImg, OutputImg, ksize, anchor, borderType = cv::BORDER_DEFAULT)
```

- `ksize`：卷积核大小 -> 窗口大小（`cv::Size(3,3)`）
- `anchor`：锚点 -> 窗口中心（`Point(-1,-1)` 默认是中心位置）
- 注意：卷积核越大，图像越模糊

## 2. 处理边缘像素

由于卷积核无法完整覆盖图像的边缘/角落像素，导致输出图像尺寸比输入小，所以可在平滑处理（图像卷积）前对边缘像素进行填充。

```cpp
copyMakeBorder(inputImg, imgOutput, 填充层数, 填充层数, 填充层数, 填充层数, borderType)
```

- `borderType` 一般为 `BORDER_DEFAULT`
- 只有 `borderType` 等于 `cv::BORDER_CONSTANT` 时，才能显示指定的 `cv::Scalar` 值填充边界

## 3. 锐化与模糊

- **模糊**：降低局部对比度，使得边缘平滑、细节减弱、图像柔和
- **锐化**：增强局部对比度，突出轮廓、增强细节、提高清晰度（无法恢复已经模糊掉的细节）

### 3.1 高斯模糊

- 用途：模糊效果自然、过度平滑
- 原理：对其中每个像素按照"中间重、周围轻"的规矩平均分。σ 越大，找的周围的像素就越多，距离远的像素占比就重，最终变得更加模糊

```cpp
gaussianBlur(inputImg, OutputImg, ksize, sigmaX, sigmaY(默认等于sigmaX), borderType = BORDER_DEFAULT)
```

- `sigmaX` 表示模糊程度：
  - 0.5 ~ 2  磨皮
  - 2 ~ 8    背景虚化
  - 8 ~ 15   重度模糊
- 注意：如果使用 `sigmaX`，`ksize` 就要设成 `Size(0,0)`
- 高斯模糊没有锚定位置，默认为中心点

### 3.2 盒子模糊（均值模糊）

- 用途：快速进行模糊处理

```cpp
boxFliter(inputImg, outputImg, ddepth, ksize, anchor, normalize = true, borderType = BORDER_DEFAULT)
```

- `ddepth` 为输出图像的数据类型，该值为 -1 时默认和输入图像的数据类型相同
- `normalize` 是否归一化，一般为 `true`

## 4. 自定义滤波

```cpp
filter2D(inputImg, OutputImg, ddepth, 自定义的卷积核, anchor, delta(用于调整图像亮度), borderType)
```

## 5. 图像数据类型（ddepth）的转换

```cpp
convertScaleAbs(inputImg, OutputImg, alpha, belta) // 对图片做缩放、取绝对值，并转换为8位无符号整数
// img[i] * alpha + belta
```

- 并不会改变图片原有的通道数

## 6. 图像梯度

作用：专门量化像素灰度变化的剧烈程度 + 变化方向。

1. **Robot**：计算速度快，对噪声极其敏感，边缘定位精度低。注意：OpenCV 没有该函数，得手写，最好不用
2. **Sobel**（首选）：结合高斯平滑（中间行权重大），抗噪性强

```cpp
Sobel(inputImg, outputImg, ddepth, dx, dy, ksize(Sobel核尺寸，一般为1/3/5/7，默认为3), scale = 1(缩放因子), delta = 0, borderType = cv::BORDER_DEFAULT)
```

3. **Scharr**：梯度方向计算最精准

```cpp
Scharr(inputImg, outputImg, ddepth, dx, dy, scale = 1, delta = 0, borderType = cv::BORDER_DEFAULT)
```

注意：robot、Sobel、Scharr 是计算 Gx/Gy，最后求出图像梯度 `G = |Gx| + |Gy|`

## 7. 图像边缘发现

### 7.1 拉普拉斯算子

相当于对图像进行二次微分，突出细节（容易受噪点干扰）。

```cpp
laplacian(inputImg, outputImg, ddepth, ksize = 1, scale = 1, delta = 0, borderType = BORDER_DEFAULT)
```

- 如果检测精细的边缘时 `ksize = 1`；如果图像有噪点时 `ksize = 3` 会使图像更加丝滑
- 锐化图 = 原图 - k × 拉普拉斯结果（使用 `addWeighted()`，原图像权重为 1，拉普拉斯结果权重为 -2 ~ -0.2）
- 如果不想受噪点影响，可以将原图进行高斯模糊，显著降低噪点带来的影响

### 7.2 直接锐化（轻度锐化）

使用矩阵 `[0,-1,0;-1,5,-1;0,-1,0]` 配合 `filter2D()` 进行图像锐化。

### 7.3 USM 锐化（不容易受噪点影响）

锐化图 = 原图 + amount × (原图 - 高斯模糊图)。amount 受不同条件影响，一般取 0.85。

## 8. 图像噪声

噪声分类：

1. **椒盐噪声**：图片上遍布白色和黑色的小点 -> 一般使用中值滤波进行去噪
2. **高斯噪声**：图片上每个像素都有连续扰动（符合正态分布），整体呈现薄雾感和颗粒感 -> 可以使用高斯模糊去噪，但效果不是特别理想
   - `randn()` 获取正态分布的随机数
3. **其他噪声**

## 9. 图像去噪

### 9.1 中值滤波

利用滑动窗口，将里面的数从小到大排序，取中值并替换掉最大值和最小值。

- 小窗口（3×3）：去噪弱，但保留细节好，适合噪声少、细节多的图
- 大窗口（5×5）：去噪强，但会损失细节，适合噪声多的图
- 一般先试小窗口，不行再换大窗口
- 注意：窗口边长大小必须是奇数

```cpp
medianBlur(inputImg, outputImg, ksize = (3/5))
```

### 9.2 均值滤波

与中值滤波相似，但均值滤波是取窗口内的平均值，再将极值替换掉。

## 10. 边缘保留滤波

### 10.1 高斯双边滤波（保留边缘）

包含两个权重：

- **空间域核**：由像素位置的欧式距离决定。在平坦区域，中心点与周围像素亮度值接近，空间域权重起主导作用，滤波效果近似于高斯模糊，有效去除噪声
- **值域核**：由像素值的差异决定。在边缘区域，中心点与边缘另一侧的像素值差异大，值域权重会显著降低这些像素的贡献，只保留与中心像素相似的像素参与计算，从而保留边缘

```cpp
bilateralFilter(inputImg, outputImg, d(邻域直径), σColor(控制颜色值相似权重), σSpace(控制空间距离衰减), borderType)
```

- `d = 5` 速度与效果平衡；`d = 9` 更强去噪
- `σColor` 和 `σSpace` 取值要相近：
  - 75 ~ 100  常规
  - \>150 强制平滑 -> 卡通效果

### 10.2 均值迁移

（见图像检测笔记的目标跟踪部分）

### 10.3 非局部均值滤波（边缘保护更好）

利用整个图像中所有相似块进行加权平均 -> 计算量大，但保留细节能力强。

```cpp
fastNlMeansDenoising(inputImg, outputImg, h = 3, hColor = 3, templateWindowSize = 7, searchWindowSize = 21) // 处理灰度图
fastNlMeansDenoisingColored() // 处理彩色图
```

- `h`：噪声过大时 10~15，细节敏感场景减小 2~5
- `hColor` 通常为 `h/2 ~ h`
- `templateWindowSize` 块越大越能捕捉结构
- `searchWindowSize` 窗口越大找到的相似块越多

### 10.4 局部均方差

## 11. 边缘提取

图像的边缘：单位向量在该方向上图像像素强度变化最大。边缘强度跟沿法线方向的图像局部对比关系相关，对比越大越是边缘。

```cpp
Canny(inputGrayImg, outputGrayImg, lowThreshold, highThreshold, ksize = 3, G = false(当G = true时，为更加精确的计算))
```

- 最大值和最小值默认是 150、50
- 如果要显示更多细节，可以将两个阈值都调小，例如 20~60；对于人脸，阈值 30~90
- 注意：边缘部分为纯白，非边缘部分为纯黑，因此可以使用 `bitwise_and(BGRImg, BGRImg, outputImg, CannyMaskGrayImg)` 提取图片中的边缘，该边缘为彩色的

## 12. 灰度图像、阈值化

- **灰度图像**：单通道，取值范围 0~255
- **阈值化**：根据阈值将矩阵中的数进行处理
- **二值图**：单通道，取值 0（纯黑）、255（纯白），属于阈值化的一种

```cpp
cv::threshold(inputGrayImg, outputImg, threshold(阈值), maxval(最大值), type)
```

| type | 说明 |
| --- | --- |
| THRESH_BINARY | 标准二值化 |
| THRESH_BINARY_INV | 反转二值化 |
| THRESH_TRUNC | 截断高于阈值的值为阈值 |
| THRESH_TOZERO | 低于阈值归零 |
| THRESH_TOZERO_INV | 高于阈值归零 |

### 12.1 全局阈值

为整幅图像设置单一固定阈值进行分割处理，所有像素使用相同的阈值标准进行二值化判断。

- **均值法**：统计图像像素大小的均值 -> 不建议使用
- **OTSU**：优先使用

```cpp
double t = threshold(inputGrayImg, outputImg, 0(必须设置为0), maxval(最大值), type | THRESH_OTSU)
// outputImg 为修改过的图像
```

- **三角法**：用在生物学方面用途较广，对图像直方图中有一个主峰（不能在中间）且有一侧长尾的图像效果比较好

```cpp
double t = threshold(inputGrayImg, outputImg, 0(必须设置为0), maxval(最大值), type | THRESH_TRIANGLE)
// outputImg 为修改过的图像
```

### 12.2 自定义（自适应）阈值

为每个像素计算专属阈值，适用于光照不均或对比度变化大的图像。

```cpp
cv::adaptiveThreshold(inputImg, outputImg, maxValue, adaptiveMethod, thresholdType, blockSize, C)
```

- `adaptiveMethod` 为自适应方法：
  - `ADAPTIVE_THRESH_MEAN_C`：阈值是邻域的平均值减去 C -> 计算快
  - `ADAPTIVE_THRESH_GAUSSIAN_C`：阈值是邻域的加权平均值减去 C -> 更好的保留细节
- `blockSize`：邻域大小，一般取值在 11~31 中的奇数
- `C`：最后统一相减的值，一般为 2~10

## 13. 连通组件扫描（CCL）

连通组件扫描（CCL，连通组件标记算法）：一种用于检测和标记图像中连通区域的技术，主要针对二值图。

块扫描 + 决策表（BBDT）。定义：在图像中，将像素值相同且相互连接的像素点分组为同一区域。

```cpp
int num = connectedComponents(inputImg, labels, connectivity = 8, ltype = CV_32S, ccltype = 0//CCL_DEFAULT)
```

- `labels` 输出标签尺寸，背景固定为 0，`ddepth = CV_32S`
- `connectivity` 为 8 邻域
- `ltype` 表示计算连通域个数
- `ccltype` 表示算法类型
- 注意：输出的 num 要减一，因为计算出的连通域包含背景（标签 0）

```cpp
int num = connectedComponentsWithStats(inputImg, labels, stats, centroid, connectivity = 8, ltype = CV_32S)
```

- `stats` 为 N × 5 的矩阵，每行：`[left, top, width, height, area(面积)]`

```cpp
int x = stats.at<int>(i, cv::CC_STAT_LEFT == 0);
int y = stats.at<int>(i, cv::CC_STAT_TOP == 1);
int w = stats.at<int>(i, cv::CC_STAT_WIDTH == 2);
int h = stats.at<int>(i, cv::CC_STAT_HEIGHT == 3);
int area = stats.at<int>(i, cv::CC_STAT_AREA == 4);
// cv::CC_STAT_LEFT 等均为常量；注意数据类型为 CV_32S
```

- `centroid` 为 N × 2 的矩阵，每行：`[质心x, 质心y]`

```cpp
double cx = centroids.at<double>(i, 0);
double cy = centroids.at<double>(i, 1);
// 注意数据类型为 CV_64F
```

- 注意：`stats` 和 `centroid` 的第一个元素为背景的信息

## 14. 边缘与轮廓的区别

- **边缘**：灰度强度发生显著变化的位置，可能是不连续的
- **轮廓**：物体的完整边界，更强调闭合性和结构性

## 15. 图像轮廓的发现与计算

```cpp
findContours(inputImg(二值图像), contours(输出轮廓), hierarchy(轮廓层级关系), mode(检查轮廓的方式), method(轮廓逼近方式), offset = Point(0,0)(轮廓点的偏移量))
```

- `contours`：输出轮廓集合，类型是 `vector<vector<Point>>`，每个 `vector<Point>` 中存储一个轮廓及其所有坐标点
- `hierarchy`：输出轮廓的层级关系，类型是 `vector<Vec4i>`，每个 `Vec4i` 对应一个轮廓，存储四个值 `[next, prev, child, parent]`
  - `next`：同级下一个轮廓的索引
  - `prev`：同级上一个轮廓的索引
  - `child`：第一个子轮廓的索引
  - `parent`：父轮廓的索引
  - 注意：如果没有对应关系，则返回 -1
- `mode`：轮廓检索模式
  - `RETR_EXTERNAL`：检查最外层轮廓
  - `RETR_LIST`：检查所有轮廓，但不建立层级关系
  - `RETR_TREE`：检查所有轮廓，建立完整的层级关系
- `method`：轮廓逼近方法
  - `CHAIN_APPROX_NONE`：存储轮廓上所有的点（点数量密集）
  - `CHAIN_APPROX_SIMPLE`：压缩轮廓点，只保留关键拐点（点数量稀疏，最常用）
- 注意：`findContours()` 是检查非零像素的边界，如果背景为白色，`RETR_EXTERNAL` 可能会失效

```cpp
drawContours(img, contours, contourIdx, color, thickness)
// contourIdx 表示绘制的轮廓索引，-1 表示绘制所有轮廓，0/1/2... 表示绘制特定轮廓
```

```cpp
double area = contourArea(contour) // 计算轮廓面积
double len = arcLength(contour, true) // 计算轮廓周长，true 表示计算封闭轮廓（一般轮廓均封闭，所以一般该参数为 true）
```

```cpp
Rect box = boundingRect(contour) // 生成将 vector 中所有点包裹住且平行于坐标轴的矩形 -> 绘制最大矩形
RotatedRect box = minAreaRect(contour) // 获取 [center, size, angle] -> 生成旋转矩形，能包裹所有点的最小矩形；可放进 ellipse() 中，但不能直接放入 rectangle() 中
```

- `RotatedRect` 的实例化对象有 `points()` 方法，直接获取 `[左上角, 右上角, 右下角, 左下角]`

```cpp
实例化对象.points(坐标数组)
```

### 15.1 轮廓匹配

用于物体的识别与分类、工业缺陷检测。对细节敏感，对遮挡敏感，对视角敏感。

- **矩（Moments）**：描绘了形状的几何特征
  - `M00`：轮廓的面积
  - `M10`：轮廓的 x 方向矩
  - `M01`：轮廓的 y 方向矩

```cpp
Moment mm = moments(contour) // 计算矩
```

- **中心矩**：以形状的重心为参考点，解决平移问题

```cpp
x = M10 / M00 // 重心 x 的坐标
y = M01 / M00 // 重心 y 的坐标
cx = mm.m10 / mm.m00;
cy = mm.m01 / mm.m00;
```

- **Hu 矩**：解决缩放和旋转的问题

```cpp
Mat hu;
HuMoments(mm, hu) // 计算 Hu 矩
```

- 原理：进行 Hu 矩匹配

```cpp
double dist = matchShapes(hu1, hu2, method, parameter = 0);
// 0 <= dist <= 1，当 dist < 0.3 为相似
// method 匹配方法：CONTOURS_MATCH_I1 -> 速度慢，精度高
```

### 15.2 轮廓的拟合

目的：简化轮廓、提取特征、对象识别。

- **多边形拟合**：简化轮廓点数的过程，移除轮廓上不重要的点，同时尽量保持轮廓的整体形状
  - 优点：减少计算量，消除噪点，提取轮廓的关键特征

```cpp
approxPolyDP(inputCurve, outputCurve, epsilon, close)
// epsilon：逼近轮廓 -> 轮廓周长 * 0.01（如果有多个轮廓，则需为每个轮廓单独配值）
// 注意：传入和传出的轮廓均为一维轮廓
```

- **最小外接矩形（minAreaRect）**：找到能包围轮廓的最小矩形
- **边界矩形（boundingRect）**：找到能包围轮廓面积最小的轴对称矩形
- **最小包围圆**：找到能包围轮廓的半径最小的圆

```cpp
minEnclosingCircle(contour, center, radius)
```

- **椭圆拟合**：找到包围轮廓面积最小的椭圆

```cpp
fitEllipse()
```

## 相关笔记

- [[C++OpenCV基础总结]]
- [[C++OpenCV经验总结]]
- [[C++OpenCV图像检测]]
- [[C++类和对象]]
