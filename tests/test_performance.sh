#!/bin/bash
# Scheduled dev-board measurement, adapted from yolo_general's vision benchmark.
set -euo pipefail
export PERCEPTION_MODULE=semantic_seg
export PERCEPTION_NODE=semantic_seg_node
export PERCEPTION_PACKAGES=semantic_seg
export PERCEPTION_CONFIG_REL=config/pp_liteseg.yaml
export PERCEPTION_TOPIC=/perception/semantic_segmentation
export PERCEPTION_IMAGE_TOPIC=/semantic_seg_perf/image_raw
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec "${SCRIPT_DIR}/../../yolo_general/tests/vision_performance.sh"
