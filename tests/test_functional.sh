#!/bin/bash
# Functional dev-board test: publish an image and require all semantic outputs.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SHARED_TESTS="${SCRIPT_DIR}/../../yolo_general/tests"
# shellcheck disable=SC1091
source "${SHARED_TESTS}/common_setup.sh"

PKG_CONFIG="$(pkg_share_config semantic_seg config/pp_liteseg.yaml)"
IMAGE_TOPIC=/semantic_seg_test/image_raw
MASK_TOPIC=/semantic_seg_test/mask
CLASSES_TOPIC=/semantic_seg_test/classes
DEBUG_TOPIC=/semantic_seg_test/debug
NODE_PID=""; PUB_PID=""; MASK_PID=""; CLASSES_PID=""; DEBUG_PID=""
MASK_FILE=""; CLASSES_FILE=""; DEBUG_FILE=""
cleanup() {
  rm -f "${MASK_FILE}" "${CLASSES_FILE}" "${DEBUG_FILE}"
  stop_node_gracefully "${MASK_PID}"; stop_node_gracefully "${CLASSES_PID}"
  stop_node_gracefully "${DEBUG_PID}"; stop_node_gracefully "${PUB_PID}"
  stop_node_gracefully "${NODE_PID}"
}
trap cleanup EXIT INT TERM

ros2 run semantic_seg semantic_seg_node --ros-args \
  -p "config_path:=${PKG_CONFIG}" -p lazy_load:=false -p use_camera:=false \
  -p "image_topic:=${IMAGE_TOPIC}" -p "mask_topic:=${MASK_TOPIC}" \
  -p "classes_topic:=${CLASSES_TOPIC}" -p "debug_image_topic:=${DEBUG_TOPIC}" &
NODE_PID=$!
assert_node_starts semantic_seg_node
assert_topic_exists "${MASK_TOPIC}"; assert_topic_exists "${CLASSES_TOPIC}"; assert_topic_exists "${DEBUG_TOPIC}"

MASK_FILE="$(mktemp)"; CLASSES_FILE="$(mktemp)"; DEBUG_FILE="$(mktemp)"
timeout 45s ros2 topic echo "${MASK_TOPIC}" --once >"${MASK_FILE}" 2>/dev/null & MASK_PID=$!
timeout 45s ros2 topic echo "${CLASSES_TOPIC}" --once >"${CLASSES_FILE}" 2>/dev/null & CLASSES_PID=$!
timeout 45s ros2 topic echo "${DEBUG_TOPIC}" --once >"${DEBUG_FILE}" 2>/dev/null & DEBUG_PID=$!
sleep 1

TEST_IMAGE="$(grep '^test_image:' "${PKG_CONFIG}" | head -1 | sed 's/^test_image:[[:space:]]*//' | sed "s#^~#${HOME}#")"
PUBLISH_ARGS=(--topic "${IMAGE_TOPIC}" --count 40)
if [ -f "${TEST_IMAGE}" ]; then PUBLISH_ARGS+=(--image "${TEST_IMAGE}" --strict-image); fi
python3 "${SHARED_TESTS}/publish_test_image.py" "${PUBLISH_ARGS[@]}" & PUB_PID=$!
wait "${MASK_PID}"; wait "${CLASSES_PID}"; wait "${DEBUG_PID}"
wait "${PUB_PID}"
grep -q 'encoding: mono8' "${MASK_FILE}"
grep -q 'data:' "${CLASSES_FILE}"
grep -q 'encoding: bgr8' "${DEBUG_FILE}"
echo "PASS: inference produced mask, class counts, and debug image"
