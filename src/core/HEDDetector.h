#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <string>
#include <vector>

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

// CropLayer 自定义层
// HED 的 Caffe prototxt 中使用了 Crop 层，OpenCV DNN 不原生支持，需要自定义实现。
// 必须在 readNet 之前通过 LayerFactory 注册。
class CropLayer : public cv::dnn::Layer {
public:
    // 工厂方法，用于 LayerFactory 注册
    static cv::Ptr<cv::dnn::Layer> create(cv::dnn::LayerParams& params);

    // 计算输出形状
    // inputs[0] = 输入特征图, inputs[1] = 目标尺寸参考
    // 输出 = 从 inputs[0] 中心裁剪到 inputs[1] 的空间尺寸
    virtual bool getMemoryShapes(const std::vector<std::vector<int>>& inputs,
                                  const int requiredOutputs,
                                  std::vector<std::vector<int>>& outputs) const override;

    // 前向传播：执行裁剪操作
    virtual void forward(const std::vector<cv::Mat*>& inputs,
                         std::vector<cv::Mat>& outputs,
                         std::vector<cv::Mat>& internals) override;
};

} // namespace SketchMaster
