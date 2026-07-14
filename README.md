# semantic_seg

## 项目简介

ROS2 语义分割节点，基于 `vision_service.h` 调用 `model_zoo/vision` 的 PP-LiteSeg 模型，实现实时语义分割。

## 功能特性

- 支持 PP-LiteSeg 语义分割
- 输出 `mono8` 类别标签图及前景类别像素统计
- 提供可视化调试图像
- 支持直连摄像头或订阅图像话题
- 不支持：实例级 ID、目标跟踪

## 快速开始

### 环境准备

- ROS2 Humble 或更高版本
- 已编译的 `components/model_zoo/vision` 组件
- PP-LiteSeg 模型文件

### 构建编译

```bash
colcon build --packages-select semantic_seg
source install/setup.bash
```

### 运行示例

```bash
ros2 launch semantic_seg semantic_seg.launch.py
```

## 详细使用

### 依赖

- `components/model_zoo/vision`：提供 `libvision.so` 与 `vision_service.h`
- PP-LiteSeg 模型及对应 yaml 配置

### 话题

| 类型 | 话题（默认） | 说明 |
|------|--------------|------|
| 订阅/发布 | `/camera/image_raw` | `use_camera=true` 时发布，否则订阅 |
| 发布 | `/perception/semantic_segmentation` | `sensor_msgs/Image`，`mono8`；像素值为类别 ID，0 为背景 |
| 发布 | `/semantic_seg/classes` | `Float32MultiArray`，每类别 2 个数：label,pixel_count |
| 发布 | `/semantic_seg/debug_image` | 语义分割可视化图 |

### 配置

- 节点参数：`config/semantic_seg.yaml`
- 默认模型配置：`config/pp_liteseg.yaml`，Cityscapes 19 类
- 常用参数：`use_camera`、`camera_id`、`camera_fps`、`image_topic`
- 当 `use_camera=false` 时，从 `image_topic` 订阅图像；`camera_fps` 必须大于 0。

## 常见问题

- 若没有分割结果，先确认模型路径和输入图像话题。
- 空、非 `CV_8UC1`、尺寸不符或类别 ID 不在 1..255 的掩码会被忽略。
- 若从外部话题送图，请确认 `use_camera:=false`。

## 版本与发布

当前版本：1.0.0

变更记录：
- 初始版本发布

## 贡献方式

欢迎提交 Issue 和 Pull Request。

贡献者与维护者名单见：`CONTRIBUTORS.md`（如有）。

## License

本组件源码文件头声明为 Apache-2.0，最终以本目录 `LICENSE` 文件为准。
