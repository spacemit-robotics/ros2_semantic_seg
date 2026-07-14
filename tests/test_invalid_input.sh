#!/bin/bash
# Adapted invalid-config smoke test; run on a ROS 2 development board.
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck disable=SC1091
source "${SCRIPT_DIR}/../../yolo_general/tests/common_setup.sh"
assert_node_fails_with_error \
  "ros2 run semantic_seg semantic_seg_node --ros-args -p config_path:=${SCRIPT_DIR}/test_config_invalid.yaml -p use_camera:=false -p lazy_load:=false" \
  "YAML|parse|Create failed|Exception"
