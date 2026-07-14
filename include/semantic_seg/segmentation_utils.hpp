/* Semantic-mask composition helpers for semantic_seg. */
#ifndef SEGMENTATION_UTILS_HPP
#define SEGMENTATION_UTILS_HPP

#include <map>
#include <vector>

#include <opencv2/core.hpp>
#include "vision_service.h"

namespace semantic_seg {

// Every mask must be a non-empty CV_8UC1 image matching image_size.  The caller
// validates this prerequisite; later masks take precedence in overlap regions.
cv::Mat CombineSegmentationMasks(
    const cv::Size& image_size,
    const std::vector<const vision::Segmentation*>& segments);
std::map<int, size_t> CountForegroundPixels(const cv::Mat& class_mask);

}  // namespace semantic_seg
#endif  // SEGMENTATION_UTILS_HPP
