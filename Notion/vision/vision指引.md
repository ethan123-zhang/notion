---
tags:
  - MOC
  - 导航
  - OpenCV
  - 视觉
  - 算法索引
---

# vision 指引

> OpenCV / 机器视觉的主题级索引，点击直达文件内小节。
> 相关：[[机器学习/机器学习指引|机器学习指引]] · [[日常学习总结/视觉与相机]] · [[日常学习总结/深度学习与边缘部署]]

## 内容文件

- [[vision/C++OpenCV基础总结|OpenCV 基础总结]] — Mat、像素、色彩空间、直方图
- [[vision/C++OpenCV图像处理|图像处理]] — 卷积、滤波、边缘、阈值、轮廓
- [[vision/C++OpenCV图像检测|图像检测]] — 霍夫、分割、特征点、匹配、跟踪
- [[vision/C++OpenCV图像形态学操作|形态学操作]] — 腐蚀膨胀、开闭、顶帽黑帽
- [[vision/C++OpenCV经验总结|经验总结]] — 工程实战技巧
- [[vision/C++ YOLO|C++ YOLO]] — 深度学习目标检测部署

## 速查表（点击直达小节）

| 主题 | 文件内定位 |
|---|---|
| Mat / Scalar | [[C++OpenCV基础总结#Mat 与 Scalar]] |
| 图像加载显示保存 | [[C++OpenCV基础总结#图像文件的加载、显示与保存]] |
| 像素访问 / 修改 | [[C++OpenCV基础总结#访问和修改像素值]] |
| 色彩空间 / HSV | [[C++OpenCV基础总结#色彩空间]] |
| 直方图统计 / 均衡化 | [[C++OpenCV基础总结#图像直方图统计]] · [[C++OpenCV基础总结#直方图均衡化]] |
| 图像卷积 | [[C++OpenCV图像处理#1. 图像卷积]] |
| 锐化 / 高斯 / 盒子模糊 | [[C++OpenCV图像处理#3. 锐化与模糊]] |
| 图像梯度 / 边缘发现 | [[C++OpenCV图像处理#6. 图像梯度]] · [[C++OpenCV图像处理#7. 图像边缘发现]] |
| 图像去噪 | [[C++OpenCV图像处理#9. 图像去噪]] |
| 边缘保留滤波 | [[C++OpenCV图像处理#10. 边缘保留滤波]] |
| 阈值化（全局/自适应） | [[C++OpenCV图像处理#12. 灰度图像、阈值化]] |
| 连通组件扫描 | [[C++OpenCV图像处理#13. 连通组件扫描（CCL）]] |
| 轮廓发现 / 匹配 / 拟合 | [[C++OpenCV图像处理#15. 图像轮廓的发现与计算]] |
| 霍夫直线 / 圆检测 | [[C++OpenCV图像检测#1. 霍夫变换]] |
| 图像分割（K-Means/GMM/分水岭/GrabCut） | [[C++OpenCV图像检测#2. 图像分割]] |
| 距离变换 | [[C++OpenCV图像检测#3. 距离变换]] |
| 特征点提取（角点/ORB/SIFT） | [[C++OpenCV图像检测#4. 特征点提取]] |
| 特征匹配（BFMatcher/FLANN） | [[C++OpenCV图像检测#5. 特征匹配]] |
| 目标跟踪 | [[C++OpenCV图像检测#6. 目标跟踪]] |
| 腐蚀 / 膨胀 | [[C++OpenCV图像形态学操作#腐蚀]] · [[C++OpenCV图像形态学操作#膨胀]] |
| 开操作与闭操作 | [[C++OpenCV图像形态学操作#开操作与闭操作]] |
| 顶帽 / 黑帽 / 击中不击中 | [[C++OpenCV图像形态学操作#顶帽]] · [[C++OpenCV图像形态学操作#黑帽]] · [[C++OpenCV图像形态学操作#击中不击中]] |
| 降低计算量（经验） | [[C++OpenCV经验总结#降低计算量]] |
| 提高特征点匹配准确率 | [[C++OpenCV经验总结#提高特征点匹配的准确率]] |
| CLAHE 增强 | [[C++OpenCV经验总结#CLAHE 增强]] |
| YOLO：blobFromImage | [[C++ YOLO#2. 图像 → 张量：blobFromImage]] |
| YOLO：前向推理 / NMS | [[C++ YOLO#3. 前向推理]] |
| YOLO：姿态检测 | [[C++ YOLO#4. 姿态检测（YOLOv8n-pose）]] |
| YOLO：onnxRuntime | [[C++ YOLO#5. onnxRuntime 替代方案]] |
| YOLO：输出解析 / 模型结构 | [[C++ YOLO#6. 输出解析：按任务类型]] · [[C++ YOLO#7. 模型内部结构（YOLOv8）]] |

## 自动索引（Dataview）

```dataview
TABLE tags AS 标签
FROM "Notion/vision"
WHERE file.name != "vision指引"
SORT file.name ASC
```
