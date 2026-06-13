#include "HEDDetector.h"
#include <opencv2/dnn/layer.hpp>
#include <opencv2/dnn/layer_reg.hpp>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

namespace SketchMaster {

// ============================================================
// CropLayer 自定义层实现
// ============================================================

cv::Ptr<cv::dnn::Layer> CropLayer::create(cv::dnn::LayerParams& /*params*/) {
    return cv::Ptr<cv::dnn::Layer>(new CropLayer());
}

bool CropLayer::getMemoryShapes(const std::vector<std::vector<int>>& inputs,
                                 const int /*requiredOutputs*/,
                                 std::vector<std::vector<int>>& outputs) const {
    // inputs[0] = 输入特征图 [batch, channels, height, width]
    // inputs[1] = 目标尺寸参考 [batch, channels, targetHeight, targetWidth]
    // 输出 = 从 inputs[0] 中心裁剪到 inputs[1] 的空间尺寸

    if (inputs.size() < 2) {
        CV_Error(cv::Error::StsBadArg, "CropLayer 需要两个输入");
    }

    const auto& inputShape = inputs[0];
    const auto& targetShape = inputs[1];

    int batchSize = inputShape[0];
    int numChannels = inputShape[1];
    int targetHeight = targetShape[2];
    int targetWidth = targetShape[3];

    outputs.resize(1);
    outputs[0] = {batchSize, numChannels, targetHeight, targetWidth};

    return true;
}

void CropLayer::forward(const std::vector<cv::Mat*>& inputs,
                         std::vector<cv::Mat>& outputs,
                         std::vector<cv::Mat>& /*internals*/) {
    // 从 inputs[0] 的中心裁剪到 inputs[1] 的空间尺寸
    cv::Mat& input = *inputs[0];
    cv::Mat& target = *inputs[1];

    int inputHeight = input.size[2];
    int inputWidth = input.size[3];
    int targetHeight = target.size[2];
    int targetWidth = target.size[3];

    // 计算中心裁剪的起始偏移
    int ystart = (inputHeight - targetHeight) / 2;
    int xstart = (inputWidth - targetWidth) / 2;
    int yend = ystart + targetHeight;
    int xend = xstart + targetWidth;

    // 确保偏移量合法
    ystart = std::max(0, ystart);
    xstart = std::max(0, xstart);
    yend = std::min(inputHeight, yend);
    xend = std::min(inputWidth, xend);

    // 提取裁剪区域
    // 对于 4D blob [N, C, H, W]，使用 ranges 提取
    cv::Range ranges[4] = {
        cv::Range::all(),          // batch
        cv::Range::all(),          // channels
        cv::Range(ystart, yend),   // height
        cv::Range(xstart, xend)    // width
    };

    outputs[0] = input(ranges).clone();
}

// ============================================================
// HEDDetector 实现
// ============================================================

// 注册 CropLayer 自定义层（全局，程序启动时执行一次）
// 注意：必须在 readNet 之前注册
static bool g_cropLayerRegistered = []() {
    cv::dnn::LayerFactory::registerLayer("Crop", CropLayer::create);
    return true;
}();

HEDDetector::HEDDetector()
    : netLoaded_(false) {
}

HEDDetector::~HEDDetector() {
}

std::string HEDDetector::findModelFile(const std::string& modelDir, const std::string& filename) {
    // 候选搜索路径列表
    std::vector<std::string> searchPaths = {
        modelDir + "/" + filename,                                          // 指定目录
        "models/hed/" + filename,                                           // 当前工作目录下的 models/hed/
        "models/hed/" + filename,                                           // 相对路径
        "../models/hed/" + filename,                                         // 上级目录
    };

    // 尝试从可执行文件所在目录查找
    // 注意：这里使用简单的方式，在 loadModel 中会进一步处理

    for (const auto& path : searchPaths) {
        if (fs::exists(path)) {
            return path;
        }
    }

    return "";
}

bool HEDDetector::loadModel(const std::string& modelDir) {
    lastError_.clear();

    // 确定模型文件路径
    std::string prototxtPath = findModelFile(modelDir, "deploy.prototxt");
    std::string caffemodelPath = findModelFile(modelDir, "hed_pretrained_bsds.caffemodel");

    if (prototxtPath.empty()) {
        lastError_ = "未找到 HED 模型文件 deploy.prototxt。\n"
                     "请在 models/hed/ 目录下放置以下文件：\n"
                     "  1. deploy.prototxt\n"
                     "  2. hed_pretrained_bsds.caffemodel\n"
                     "下载方式请参考 models/hed/README.md";
        std::cerr << "[HEDDetector] " << lastError_ << std::endl;
        return false;
    }

    if (caffemodelPath.empty()) {
        lastError_ = "未找到 HED 模型权重文件 hed_pretrained_bsds.caffemodel。\n"
                     "请在 models/hed/ 目录下放置该文件。\n"
                     "下载方式请参考 models/hed/README.md";
        std::cerr << "[HEDDetector] " << lastError_ << std::endl;
        return false;
    }

    try {
        // 加载 Caffe 模型
        net_ = cv::dnn::readNetFromCaffe(prototxtPath, caffemodelPath);
        netLoaded_ = true;
        std::cout << "[HEDDetector] 模型加载成功: " << prototxtPath << std::endl;
        std::cout << "[HEDDetector] 权重加载成功: " << caffemodelPath << std::endl;
        return true;
    } catch (const cv::Exception& e) {
        lastError_ = std::string("加载 HED 模型失败: ") + e.what();
        std::cerr << "[HEDDetector] " << lastError_ << std::endl;
        netLoaded_ = false;
        return false;
    } catch (const std::exception& e) {
        lastError_ = std::string("加载 HED 模型时发生异常: ") + e.what();
        std::cerr << "[HEDDetector] " << lastError_ << std::endl;
        netLoaded_ = false;
        return false;
    }
}

cv::Mat HEDDetector::detectEdges(const cv::Mat& inputImage) {
    if (!netLoaded_ || inputImage.empty()) {
        return cv::Mat();
    }

    try {
        // 1. 缩放图片到 500x500（HED 标准输入尺寸）
        cv::Mat resized;
        cv::resize(inputImage, resized, cv::Size(500, 500));

        // 2. 创建 blob
        // scalefactor=1.0, size=(500,500)
        // mean=(104.00698793, 116.66876762, 122.67891434) - ImageNet BGR 均值
        // swapRB=false（Caffe 模型使用 BGR 通道顺序）
        // crop=false
        cv::Mat blob = cv::dnn::blobFromImage(resized, 1.0, cv::Size(500, 500),
            cv::Scalar(104.00698793, 116.66876762, 122.67891434),
            false, false);

        // 3. 前向传播
        net_.setInput(blob);
        cv::Mat out = net_.forward();

        // 4. 处理输出
        // 输出形状: [1, 1, H, W]，取 out[0, 0] 作为边缘概率图
        cv::Mat edges = out.reshape(1, out.size[2]);

        // 归一化到 0-255
        cv::normalize(edges, edges, 0, 255, cv::NORM_MINMAX, CV_8U);

        // 5. 恢复到原始尺寸
        cv::Mat result;
        cv::resize(edges, result, cv::Size(inputImage.cols, inputImage.rows));

        return result;

    } catch (const cv::Exception& e) {
        lastError_ = std::string("HED 边缘检测失败: ") + e.what();
        std::cerr << "[HEDDetector] " << lastError_ << std::endl;
        return cv::Mat();
    } catch (const std::exception& e) {
        lastError_ = std::string("HED 边缘检测时发生异常: ") + e.what();
        std::cerr << "[HEDDetector] " << lastError_ << std::endl;
        return cv::Mat();
    }
}

} // namespace SketchMaster
