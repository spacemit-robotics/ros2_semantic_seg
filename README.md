# semantic_seg

ROS 2 semantic-segmentation package for the `VisionService` PP-LiteSeg backend.
It consumes either a ROS image stream or a local camera, calls
`VisionService::Infer(cv::Mat, VisionServiceResponse*)`, and uses
`VisionService::Draw(original, response, output)` for the debug image.

## Output

| Topic | Type | Meaning |
| --- | --- | --- |
| `/perception/semantic_segmentation` | `sensor_msgs/Image` (`mono8`) | Per-pixel class label; 0 is background. |
| `/semantic_seg/classes` | `std_msgs/Float32MultiArray` | Repeated pairs `[label, pixel_count]`, one per foreground class. |
| `/semantic_seg/debug_image` | `sensor_msgs/Image` (`bgr8`) | Service-rendered debug overlay. |

Null, empty, non-`CV_8UC1`, out-of-image-size masks, and labels outside the
`mono8` foreground range 1–255 are skipped with a throttled warning. Where
valid masks overlap, the later service result owns the output pixels.

## Parameters

`config_path` defaults to the installed `config/pp_liteseg.yaml`; `lazy_load`
defaults to `true`. Input parameters are `image_topic` (`/camera/image_raw`),
`use_camera` (`true`), `camera_id` (`0`), and `camera_fps` (`30.0`). Output
topics are `mask_topic`, `classes_topic`, and `debug_image_topic` with the
defaults listed above.

## Build and run (development board)

The vision staging library and PP-LiteSeg ONNX model must be prepared first.

```bash
cd middleware/ros2/perception
colcon build --packages-select semantic_seg
source install/setup.bash
ros2 launch semantic_seg semantic_seg.launch.py
```

For an external camera publisher rather than the local camera:

```bash
ros2 run semantic_seg semantic_seg_node --ros-args -p use_camera:=false \
  -p image_topic:=/camera/image_raw
```

## Tests (development board)

```bash
python3 middleware/ros2/perception/semantic_seg/tests/test_source_contract.py
bash middleware/ros2/perception/semantic_seg/tests/test_functional.sh
bash middleware/ros2/perception/semantic_seg/tests/test_invalid_input.sh
```
