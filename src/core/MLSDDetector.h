#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <string>
#include <vector>

namespace SketchMaster {

// MLSD (Mobile Line Segment Detection) 直线检测器
// 使用 OpenCV DNN 模块加载 ONNX 预训练模型
class MLSDDetector {
public:
    MLSDDetector();
    ~MLSDDetector();

    // 加载模型（从指定目录，目录下需要 mlsd_large.onnx 或 mlsd_tiny.onnx）
    bool loadModel(const std::string& modelDir);

    // 检测直线并绘制到边缘图上，返回单通道 8U 边缘图
    cv::Mat detectEdges(const cv::Mat& inputImage);

    // 是否已加载模型
    bool isLoaded() const { return netLoaded_; }

    // 获取最后的错误信息
    std::string getLastError() const { return lastError_; }

    // 设置检测参数
    void setScoreThreshold(float threshold) { scoreThreshold_ = threshold; }
    void setDistThreshold(float threshold) { distThreshold_ = threshold; }

private:
    cv::dnn::Net net_;
    bool netLoaded_;
    std::string lastError_;

    // 检测参数
    float scoreThreshold_;  // 置信度阈值（默认 0.1）
    float distThreshold_;    // 线段最短距离阈值（默认 10.0）

    // 后处理：从模型输出中提取直线并绘制到边缘图上
    cv::Mat processOutput(const cv::Mat& output, int width, int height);

    // 在多个候选路径中查找模型文件
    std::string findModelFile(const std::string& modelDir, const std::string& filename);
};

} // namespace SketchMaster
