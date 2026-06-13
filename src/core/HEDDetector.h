#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <string>

namespace SketchMaster {

// HED (Holistically-Nested Edge Detection) 深度边缘检测器
// 使用 OpenCV DNN 模块加载 Caffe 预训练模型
class HEDDetector {
public:
    HEDDetector();
    ~HEDDetector();

    // 加载模型（从指定目录，目录下需要 deploy.prototxt 和 hed_pretrained_bsds.caffemodel）
    bool loadModel(const std::string& modelDir);

    // 检测边缘，返回单通道 8U 边缘图（白色=边缘，黑色=背景）
    cv::Mat detectEdges(const cv::Mat& inputImage);

    // 是否已加载模型
    bool isLoaded() const { return netLoaded_; }

    // 获取最后的错误信息
    std::string getLastError() const { return lastError_; }

private:
    cv::dnn::Net net_;
    bool netLoaded_;
    std::string lastError_;

    // 在多个候选路径中查找模型文件
    std::string findModelFile(const std::string& modelDir, const std::string& filename);
};

} // namespace SketchMaster
