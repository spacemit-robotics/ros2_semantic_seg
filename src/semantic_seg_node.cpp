/**
 * @file semantic_seg_node.cpp
 * @brief PP-LiteSeg ROS 2 node using the current VisionService API.
 */
#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include "ament_index_cpp/get_package_share_directory.hpp"
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/image.hpp"
#include "std_msgs/msg/float32_multi_array.hpp"
#include "vision_service.h"
#include "semantic_seg/image_utils.hpp"
#include "semantic_seg/segmentation_utils.hpp"

class SemanticSegNode final : public rclcpp::Node {
public:
    SemanticSegNode() : Node("semantic_seg_node") {
        std::string config_path = declare_parameter<std::string>("config_path", "");
        const bool lazy_load = declare_parameter<bool>("lazy_load", true);
        image_topic_ = declare_parameter<std::string>("image_topic", "/camera/image_raw");
        mask_topic_ = declare_parameter<std::string>("mask_topic", "/perception/semantic_segmentation");
        classes_topic_ = declare_parameter<std::string>("classes_topic", "/semantic_seg/classes");
        debug_image_topic_ = declare_parameter<std::string>("debug_image_topic", "/semantic_seg/debug_image");
        use_camera_ = declare_parameter<bool>("use_camera", true);
        camera_id_ = declare_parameter<int>("camera_id", 0);
        camera_fps_ = declare_parameter<double>("camera_fps", 30.0);
        if (!std::isfinite(camera_fps_) || camera_fps_ <= 0.0) {
            throw std::runtime_error("camera_fps must be finite and greater than zero");
        }
        if (config_path.empty()) config_path = DefaultConfigPath();
        if (config_path.empty()) throw std::runtime_error("config_path is empty");
        service_ = VisionService::Create(config_path, "", lazy_load);
        if (!service_) throw std::runtime_error("VisionService::Create failed: " + VisionService::LastCreateError());

        mask_pub_ = create_publisher<sensor_msgs::msg::Image>(mask_topic_, rclcpp::SensorDataQoS());
        classes_pub_ = create_publisher<std_msgs::msg::Float32MultiArray>(classes_topic_, 10);
        debug_pub_ = create_publisher<sensor_msgs::msg::Image>(debug_image_topic_, rclcpp::SensorDataQoS());
        if (use_camera_) StartCamera();
        else image_sub_ = create_subscription<sensor_msgs::msg::Image>(
            image_topic_, rclcpp::SensorDataQoS(),
            std::bind(&SemanticSegNode::OnImage, this, std::placeholders::_1));
    }

private:
    static std::string DefaultConfigPath() {
        try { return ament_index_cpp::get_package_share_directory("semantic_seg") + "/config/pp_liteseg.yaml"; }
        catch (...) { return ""; }
    }
    void StartCamera() {
        image_pub_ = create_publisher<sensor_msgs::msg::Image>(image_topic_, rclcpp::SensorDataQoS());
        cap_.open(camera_id_);
        if (!cap_.isOpened()) throw std::runtime_error("semantic_seg: cannot open camera id=" + std::to_string(camera_id_));
        const auto period = std::chrono::milliseconds(static_cast<int>(1000.0 / camera_fps_));
        camera_timer_ = create_wall_timer(
            std::max(period, std::chrono::milliseconds(1)),
            std::bind(&SemanticSegNode::OnCameraTimer, this));
    }
    void OnCameraTimer() {
        std_msgs::msg::Header header;
        header.stamp = now(); header.frame_id = "camera";
        sensor_msgs::msg::Image image;
        if (!semantic_seg::CaptureCameraFrame(cap_, image, header)) {
            RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 3000, "camera read empty frame"); return;
        }
        image_pub_->publish(image);
        ProcessFrame(header, semantic_seg::ImageMsgToBgr(image));
    }
    void OnImage(const sensor_msgs::msg::Image::SharedPtr image) {
        cv::Mat bgr = semantic_seg::ImageMsgToBgr(*image);
        if (bgr.empty()) {
            RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 3000, "invalid image encoding=%s", image->encoding.c_str()); return;
        }
        ProcessFrame(image->header, bgr);
    }
    void ProcessFrame(const std_msgs::msg::Header& header, const cv::Mat& bgr) {
        if (bgr.empty()) return;
        VisionServiceResponse response;
        if (service_->Infer(bgr, &response) != VISION_SERVICE_OK || !response.ok) {
            RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 2000, "infer failed: %s", service_->LastError().c_str()); return;
        }
        std::vector<const vision::Segmentation*> segments;
        for (const auto& result : response.results) {
            const auto* segment = std::get_if<vision::Segmentation>(&result);
            if (!segment) continue;
            if (!segment->mask || segment->mask->empty() || segment->mask->type() != CV_8UC1 ||
                segment->mask->size() != bgr.size() || segment->label <= 0 || segment->label > 255) {
                RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 2000,
                    "skipping invalid segmentation mask (label=%d)", segment->label);
                continue;
            }
            segments.push_back(segment);
        }
        const cv::Mat class_mask = semantic_seg::CombineSegmentationMasks(bgr.size(), segments);
        mask_pub_->publish(semantic_seg::BgrToImageMsg(class_mask, header, "mono8"));
        PublishClassCounts(class_mask);
        PublishDebug(header, bgr, response);
    }
    void PublishClassCounts(const cv::Mat& class_mask) {
        std_msgs::msg::Float32MultiArray message;
        const auto counts = semantic_seg::CountForegroundPixels(class_mask);
        message.layout.dim.resize(2);
        message.layout.dim[0].label = "foreground_classes";
        message.layout.dim[0].size = counts.size();
        message.layout.dim[0].stride = message.layout.dim[0].size * 2;
        message.layout.dim[1].label = "fields(label,pixel_count)";
        message.layout.dim[1].size = 2; message.layout.dim[1].stride = 2;
        for (const auto& [label, pixels] : counts) {
            message.data.push_back(static_cast<float>(label));
            message.data.push_back(static_cast<float>(pixels));
        }
        classes_pub_->publish(message);
    }
    void PublishDebug(
        const std_msgs::msg::Header& header,
        const cv::Mat& bgr,
        const VisionServiceResponse& response) {
        cv::Mat out_image;
        if (service_->Draw(bgr, response, &out_image) == VISION_SERVICE_OK && !out_image.empty() &&
            out_image.size() == bgr.size() && out_image.type() == CV_8UC3)
            debug_pub_->publish(semantic_seg::BgrToImageMsg(out_image, header, "bgr8"));
    }
    std::unique_ptr<VisionService> service_;
    std::string image_topic_, mask_topic_, classes_topic_, debug_image_topic_;
    bool use_camera_; int camera_id_; double camera_fps_;
    cv::VideoCapture cap_;
    rclcpp::TimerBase::SharedPtr camera_timer_;
    rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr image_pub_, mask_pub_, debug_pub_;
    rclcpp::Publisher<std_msgs::msg::Float32MultiArray>::SharedPtr classes_pub_;
    rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr image_sub_;
};

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    try { rclcpp::spin(std::make_shared<SemanticSegNode>()); }
    catch (const std::exception& error) {
        RCLCPP_ERROR(rclcpp::get_logger("semantic_seg_node"), "Exception: %s", error.what());
        rclcpp::shutdown(); return 1;
    }
    rclcpp::shutdown(); return 0;
}
