#include "semantic_seg/segmentation_utils.hpp"

namespace semantic_seg {
cv::Mat CombineSegmentationMasks(
    const cv::Size& image_size,
    const std::vector<const vision::Segmentation*>& segments) {
    cv::Mat class_mask(image_size, CV_8UC1, cv::Scalar(0));
    for (const vision::Segmentation* segment : segments) {
        class_mask.setTo(static_cast<unsigned char>(segment->label), *segment->mask);
    }
    return class_mask;
}

std::map<int, size_t> CountForegroundPixels(const cv::Mat& class_mask) {
    std::map<int, size_t> counts;
    for (int row = 0; row < class_mask.rows; ++row) {
        const auto* pixels = class_mask.ptr<unsigned char>(row);
        for (int col = 0; col < class_mask.cols; ++col) {
            if (pixels[col] != 0) ++counts[pixels[col]];
        }
    }
    return counts;
}
}  // namespace semantic_seg
