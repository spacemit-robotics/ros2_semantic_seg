#include "semantic_seg/image_utils.hpp"

#include <climits>
#include <cstring>
#include <opencv2/imgproc.hpp>

namespace semantic_seg {
cv::Mat ImageMsgToBgr(const sensor_msgs::msg::Image& msg) {
    if (msg.width == 0 || msg.height == 0 || msg.width > INT_MAX || msg.height > INT_MAX ||
        msg.data.empty()) return cv::Mat();
    constexpr size_t kBytesPerPixel = 3;
    const size_t minimum_step = static_cast<size_t>(msg.width) * kBytesPerPixel;
    const size_t required_bytes = static_cast<size_t>(msg.step) * msg.height;
    if (msg.step < minimum_step || msg.data.size() < required_bytes) return cv::Mat();
    if (msg.encoding == "bgr8") {
        return cv::Mat(
            msg.height, msg.width, CV_8UC3,
            const_cast<unsigned char*>(msg.data.data()), msg.step).clone();
    }
    if (msg.encoding == "rgb8") {
        cv::Mat rgb(
            msg.height, msg.width, CV_8UC3,
            const_cast<unsigned char*>(msg.data.data()), msg.step);
        cv::Mat bgr;
        cv::cvtColor(rgb, bgr, cv::COLOR_RGB2BGR);
        return bgr;
    }
    return cv::Mat();
}

sensor_msgs::msg::Image BgrToImageMsg(
    const cv::Mat& image,
    const std_msgs::msg::Header& header,
    const std::string& encoding) {
    sensor_msgs::msg::Image msg;
    msg.header = header;
    msg.height = static_cast<uint32_t>(image.rows);
    msg.width = static_cast<uint32_t>(image.cols);
    msg.encoding = encoding;
    msg.step = static_cast<uint32_t>(image.step);
    msg.data.resize(static_cast<size_t>(image.rows) * image.step);
    if (image.isContinuous()) {
        std::memcpy(msg.data.data(), image.ptr(), msg.data.size());
    } else {
        for (int row = 0; row < image.rows; ++row) {
            std::memcpy(
                msg.data.data() + static_cast<size_t>(row) * image.step,
                image.ptr(row),
                image.step);
        }
    }
    return msg;
}

bool CaptureCameraFrame(
    cv::VideoCapture& cap,
    sensor_msgs::msg::Image& out_msg,
    const std_msgs::msg::Header& header) {
    cv::Mat bgr;
    if (!cap.read(bgr) || bgr.empty()) return false;
    out_msg = BgrToImageMsg(bgr, header, "bgr8");
    return true;
}
}  // namespace semantic_seg
