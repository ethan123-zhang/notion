---
notion-id: 37f8cdcb-81ea-8083-9a9b-f3f25471b5ed
tags:
  - OpenCV
  - 图像处理
  - C++
  - 经验总结
  - mask
  - 滤波选型
  - CLAHE
  - 洪水填充
  - 特征匹配
  - 形态学重建
  - RANSAC
  - 多边形拟合
---

# C++ OpenCV 经验总结

## 闭运算后的角点钝化问题

在使用闭运算处理噪点和黑洞的时候，可能会将要检测的物体的角磨钝、线变得不平（会影响角点检测），可以使用**多边形拟合**解决这一问题。

## 用 mask 排除过暗、过亮、无颜色的部分

在得到图像的时候，使用 `inRange(image, Scalar(0,30,30), Scalar(180,255,255), mask)` 生成 mask，接着反向投影图 `&= mask`，实现除去过暗、过亮、没颜色的部分（不符合要求）。

## 降低计算量

面对计算量大时，降低计算量的方法：

1. BGR 进行归一化，将 BGR 三个通道的所有值均除以 255.0f（核心首选）。
2. BGR 下采样，但要保持宽高比（使用 resize() 对图像进行缩放，缩小 2/4 倍）。
3. BGR 进行轻量化降维：
   1. 将图片转为灰度图。
   2. 将 BGR 转化为 HSV，舍去 V，只保留 H 和 S 进行计算。

## K-Means 与 GMM 失效的处理办法

K-Means 和 GMM 只看颜色，对于很多场景会失效，处理办法有：

1. 交互式前景提取（GrabCut）。
2. 边缘检测 + 形态学操作：适合"边缘清晰"的物体。
   1. 先用 Canny 进行边缘检测。
   2. 再用形态学闭运算：将轮廓连接起来，形成一个完整的前景区域。
   3. 最后用轮廓查找最大轮廓，就是前景。
3. HSV 颜色范围选择 + 形态学操作：适合"颜色有特点，但 K-Means/GMM 太敏感"的物体。
   1. 把图像转成 HSV 颜色空间。
   2. 手动指定一个 HSV 的下限和上限，再用 inRange() 保留这个区间。
   3. 最后用形态学操作去除小噪点。

## 边缘检测前的滤波选择

在进行边缘检测前，如果图像需要滤波：

1. 如果图像噪声不大，边缘比较清晰：选**高斯滤波**。
2. 如果图像噪声较大，且需要保留精细边缘：选**双边滤波**（保边优势明显）。

## 去除内部空洞

去除内部空洞，防止其影响后续图像处理的方法：

### 1. 洪水填充法

- 原理：类似于往封闭区域倒油漆，从点的种子点开始倒油漆，油漆只会在和起点颜色一致、连在一起的区域流动，如果遇到颜色差异大的边界，油漆就不会流出去。
- 适用条件：硬币、药片、圆形零件等封闭前景的内部纹路/空洞填充，分水岭预处理的首选方案。
- `floodFill(inputImg, mask, seedPoint, newVal, rect = 0, loDiff = Scalar(), upDiff = Scalar(), flag = 4)`。
  - mask：掩码，必须比原图宽、高各多 2 像素（上下左右各一个），防止填充溢出。
  - seedPoint：填充的起始点，基于源图像（并非 mask）。
  - newVal：填充区域要染成的颜色。
  - loDiff、upDiff：分别为和种子点的灰度差下限和上限。
  - flag：默认为 4 连通。

### 2. 形态学重建法

- 原理：将橡皮泥放进模具里，不断把橡皮泥往外推（膨胀），但模具会挡住它，永远不让它超出原始边缘；直到橡皮泥不再发生变化，就得到内部填满、边缘和模具完全一样的结果。
- 优点：边缘和原始图 100% 一致。
- 缺点：慢 + 对于超大空洞的填充效果不好。
- 示例：
  ```cpp
  Mat mask = thresh.clone();  // 原始二值图，边缘完美
  Mat seed = closed.clone();  // 种子图：内部的纹路均填充

  Mat kernel = getStructuringElement();
  while (true) {
      Mat temp;
      dilate(seed, temp, kernel);
      bitwise_and(temp, mask, temp);
      if (countNonZero(temp != seed) == 0) break;  // 直到不再变化就意味着重建完成
      seed = temp.clone();
  }
  ```

## 提高特征点匹配的准确率

距离阈值判断和距离比筛选都是筛选最适合的匹配点，二选一即可。

1. BFMatcher 的交叉验证：
   ```cpp
   Ptr<BFMatcher> matcher = BFMatcher::create(NORM_L2, crossCheck = true);
   // crossCheck 表示的是是否交叉认证（若为 true，则为交叉认证，提高两点为最近距离的准确性）
   vector<DMatch> matches;   // 这里的容器为一维
   matcher->match(descriptor1, descriptor2, matches);
   ```
2. RANSAC：用于判断两点的空间关系是否合理。
   - 条件：
     1. 在平面图形进行视角变化。
     2. 对于平面或立体图形，视角不变。
   - ```cpp
     vector<uchar> mask;
     Mat H = findHomography(pts1, pts2, method = 0, ransacProjThreshold = 3.0, mask = noArray());
     // pts1 = H * pts2
     // pts1 和 pts2 均为 vector<Point2f> 类型
     ```
     - method：
       - RANSAC：首选。
       - LMEDS：在 RANSAC 效果不好时使用。
       - RHO：在点多的时候使用，效果和 RANSAC 一样，但更快。
     - ransacProjThreshold 为 RANSAC 容忍度，默认为 3.0（误差 > 3 则舍去这个点，误差 < 3 则保留这个点）。
       - 需要的点少则改为 1.0/2.0。
       - 需要的点多则改为 10.0。
     - mask 为输出的名单：`mask[i] = 1` 则为好点，`mask[i] = 0` 则为坏点。
3. RHO 算法：适用于立体图形 + 小幅度的视角变化。
   ```cpp
   findHomography(Points1, Points2, cv::RHO, 3.0, mask, 迭代次数: 10000, 0.995);
   // 重投影阈值：小视角用 1.0 - 2.0，大视角用 3.0 - 5.0
   ```
4. 距离阈值判断：先找最小距离的两个点 dis，如果两点之间的距离 < dis * 2，则两个点是符合条件的。

### 综合方式

1. 交叉验证 + 距离阈值判断 + RANSAC。
2. knn 匹配 + 距离比筛选 + RANSAC。

## 工程上角点检测

- 一般是 ORB + 亚像素角点检测。
- 如果追求高精度：Shi-Tomasi + 亚像素角点检测。

## 工程上特征点提取

- 1. 首选：ORB：速度快，实时性高。
- 2. 追求高精度但不追求速度：SIFT。

## CLAHE 增强

CLAHE 用于解决光照不均、局部过亮/过暗、对比度不足。

```cpp
cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(clipLimit = 40.0, tileGridSize = cv::Size(8,8));
// clipLimit：对比度限制阈值。值越大，对比度越强，但噪声越大
// tileGridSize：小方块的大小
clahe->apply(grayImg, enhanceImg);
```

- 注意：必须传入灰度图像。
- 如果本身光照均匀、对比度充足、纯色图片，使用 CLAHE 反而会适得其反。

## 相关笔记

- [[C++OpenCV基础总结]]
- [[C++OpenCV图像形态学操作]]
- [[C++OpenCV图像处理]]
- [[C++OpenCV图像检测]]
- [[C++ YOLO]]
