#!/usr/bin/env python3
"""Static contract checks for the semantic_seg ROS 2 node."""

from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[1]
NODE = ROOT / "src" / "semantic_seg_node.cpp"
IMAGE_UTILS = ROOT / "src" / "image_utils.cpp"
SEGMENTATION_UTILS = ROOT / "src" / "segmentation_utils.cpp"


def require(text: str, needle: str) -> None:
    if needle not in text:
        raise AssertionError(f"missing required contract: {needle}")


def main() -> int:
    text = NODE.read_text(encoding="utf-8")
    for needle in (
        "service_->Infer(bgr, &response)",
        "std::get_if<vision::Segmentation>",
        "service_->Draw(bgr, response, &out_image)",
        '"/perception/semantic_segmentation"',
        '"/semantic_seg/classes"',
        '"/semantic_seg/debug_image"',
        "CombineSegmentationMasks",
        "#include <functional>",
        "out_image.type() == CV_8UC3",
        "message.data.push_back(static_cast<float>(label))",
        "message.data.push_back(static_cast<float>(pixels))",
    ):
        require(text, needle)
    image_utils = IMAGE_UTILS.read_text(encoding="utf-8")
    for needle in (
        "#include <climits>",
        "msg.step < minimum_step",
        "msg.data.size() < required_bytes",
        "msg.width > INT_MAX",
        "msg.height > INT_MAX",
    ):
        require(image_utils, needle)
    segmentation_utils = SEGMENTATION_UTILS.read_text(encoding="utf-8")
    # Later masks overwrite overlap pixels; only non-zero labels are counted.
    for needle in (
        "class_mask.setTo(static_cast<unsigned char>(segment->label), *segment->mask)",
        "if (pixels[col] != 0) ++counts[pixels[col]]",
    ):
        require(segmentation_utils, needle)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (AssertionError, OSError) as exc:
        print(exc, file=sys.stderr)
        raise SystemExit(1)
