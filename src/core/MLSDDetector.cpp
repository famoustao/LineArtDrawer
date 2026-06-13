#include "MLSDDetector.h"
#include <iostream>
#include <filesystem>
#include <cmath>

namespace fs = std::filesystem;

namespace SketchMaster {

// ============================================================
// MLSDDetector 实现
// ============================================================

MLSDDetector::MLSDDetector()
    : netLoaded_(false)
    , scoreThreshold_(0.1f)
    , distThreshold_(10.0f) {
}

MLSDDetector::~MLSDDetector() {
}

std::string MLSDDetector::findModelFile(const std::string& modelDir, const std::string& filename) {
    // 候选搜索路径列表
    std::vector<std::string> searchPaths = {
        modelDir + "/" + filename,
        "models/mlsd/" + filename,
        "../models/mlsd/" + filename,
    };

    for (const auto& path : searchPaths) {
        if (fs::exists(path)) {
            return path;
        }
    }

    return "";
}

bool MLSDDetector::loadModel(const std::string& modelDir) {
    lastError_.clear();

    // MLSD 模型文件名候选（优先使用 large 版本，回退到 tiny 版本）
    std::vector<std::string> modelFilenames = {
        "mlsd_large.onnx",
        "mlsd_tiny.onnx",
        "mlsd.onnx"
    };

    std::string modelPath;
    for (const auto& filename : modelFilenames) {
        modelPath = findModelFile(modelDir, filename);
        if (!modelPath.empty()) {
            break;
        }
    }

    if (modelPath.empty()) {
        lastError_ = "未找到 MLSD 模型文件 (mlsd_large.onnx / mlsd_tiny.onnx)。\n"
                     "请在 models/mlsd/ 目录下放置模型文件。\n"
                     "下载方式请参考 models/mlsd/README.md";
        std::cerr << "[MLSDDetector] " << lastError_ << std::endl;
        return false;
    }

    try {
        // 加载 ONNX 模型
        net_ = cv::dnn::readNetFromONNX(modelPath);
        netLoaded_ = true;
        std::cout << "[MLSDDetector] 模型加载成功: " << modelPath << std::endl;
        return true;
    } catch (const cv::Exception& e) {
        lastError_ = std::string("加载 MLSD 模型失败: ") + e.what();
        std::cerr << "[MLSDDetector] " << lastError_ << std::endl;
        netLoaded_ = false;
        return false;
    } catch (const std::exception& e) {
        lastError_ = std::string("加载 MLSD 模型时发生异常: ") + e.what();
        std::cerr << "[MLSDDetector] " << lastError_ << std::endl;
        netLoaded_ = false;
        return false;
    }
}

cv::Mat MLSDDetector::detectEdges(const cv::Mat& inputImage) {
    if (!netLoaded_ || inputImage.empty()) {
        return cv::Mat();
    }

    try {
        // 1. MLSD 模型输入尺寸通常为 512x512
        const int inputSize = 512;
        cv::Mat resized;
        cv::resize(inputImage, resized, cv::Size(inputSize, inputSize));

        // 2. 创建 blob
        // MLSD 使用 RGB 通道顺序，需要 swapRB=true
        // 归一化到 [0, 1]
        cv::Mat blob = cv::dnn::blobFromImage(resized, 1.0 / 255.0,
            cv::Size(inputSize, inputSize),
            cv::Scalar(0, 0, 0),  // 不减均值
            true,                 // swapRB: BGR -> RGB
            false);               // 不裁剪

        // 3. 前向传播
        net_.setInput(blob);
        std::vector<cv::Mat> outputs;
        std::vector<std::string> outNames = net_.getUnconnectedOutLayersNames();
        net_.forward(outputs, outNames);

        // 4. 后处理：从输出中解码线段并绘制到边缘图
        if (outputs.empty()) {
            std::cerr << "[MLSDDetector] 模型无输出" << std::endl;
            return cv::Mat();
        }

        cv::Mat edgeMap = processOutput(outputs[0], inputImage.cols, inputImage.rows);

        return edgeMap;

    } catch (const cv::Exception& e) {
        lastError_ = std::string("MLSD 直线检测失败: ") + e.what();
        std::cerr << "[MLSDDetector] " << lastError_ << std::endl;
        return cv::Mat();
    } catch (const std::exception& e) {
        lastError_ = std::string("MLSD 直线检测时发生异常: ") + e.what();
        std::cerr << "[MLSDDetector] " << lastError_ << std::endl;
        return cv::Mat();
    }
}

cv::Mat MLSDDetector::processOutput(const cv::Mat& output, int width, int height) {
    // MLSD 模型输出形状: [1, 1, H/8, W/8, 2] 或 [1, numLines, 5]
    // 其中每个线段由 (x1, y1, x2, y2, score) 描述
    //
    // 实际上 MLSD 的 ONNX 导出格式输出为 [1, K, 5]，
    // K 为最大线段数，5 为 (x1, y1, x2, y2, score)
    // 但不同导出方式可能产生不同格式，这里做兼容处理

    // 创建黑色背景的边缘图
    cv::Mat edgeMap = cv::Mat::zeros(height, width, CV_8U);

    if (output.dims >= 2) {
        int numDims = output.dims;

        if (numDims == 5) {
            // 格式: [1, 1, H/8, W/8, 2] - 热力图格式
            // 第5维: [cos(theta), sin(theta)] 用于计算线段方向
            // 这里简化处理：将热力图作为边缘概率图
            cv::Mat heatmap = output.reshape(1, {output.size[2], output.size[3]});
            cv::normalize(heatmap, heatmap, 0, 255, cv::NORM_MINMAX, CV_8U);
            cv::resize(heatmap, edgeMap, cv::Size(width, height));
            return edgeMap;
        }

        if (numDims == 4 && output.size[1] == 1) {
            // 格式: [1, 1, H, W] - 简单边缘图
            cv::Mat heatmap = output.reshape(1, output.size[2]);
            cv::normalize(heatmap, heatmap, 0, 255, cv::NORM_MINMAX, CV_8U);
            cv::resize(heatmap, edgeMap, cv::Size(width, height));
            return edgeMap;
        }

        if (numDims >= 2 && output.size[output.dims - 1] == 5) {
            // 格式: [1, K, 5] - 线段列表格式
            // 每行: (x1, y1, x2, y2, score)
            int numLines = output.size[1];
            float scaleX = static_cast<float>(width) / 512.0f;
            float scaleY = static_cast<float>(height) / 512.0f;

            // 获取输出数据的指针
            cv::Mat output2D = output.reshape(1, numLines);

            for (int i = 0; i < numLines; i++) {
                float* row = output2D.ptr<float>(i);
                float x1 = row[0] * scaleX;
                float y1 = row[1] * scaleY;
                float x2 = row[2] * scaleX;
                float y2 = row[3] * scaleY;
                float score = row[4];

                // 过滤低置信度的线段
                if (score < scoreThreshold_) {
                    continue;
                }

                // 过滤过短的线段
                float dist = std::sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
                if (dist < distThreshold_) {
                    continue;
                }

                // 确保坐标在图像范围内
                x1 = std::max(0.0f, std::min(static_cast<float>(width - 1), x1));
                y1 = std::max(0.0f, std::min(static_cast<float>(height - 1), y1));
                x2 = std::max(0.0f, std::min(static_cast<float>(width - 1), x2));
                y2 = std::max(0.0f, std::min(static_cast<float>(height - 1), y2));

                // 在边缘图上绘制线段
                cv::line(edgeMap,
                         cv::Point(static_cast<int>(x1), static_cast<int>(y1)),
                         cv::Point(static_cast<int>(x2), static_cast<int>(y2)),
                         cv::Scalar(255), 1, cv::LINE_AA);
            }

            return edgeMap;
        }
    }

    // 兜底：尝试将输出作为普通矩阵处理
    cv::Mat flat = output.reshape(1, output.size[2]);
    cv::normalize(flat, flat, 0, 255, cv::NORM_MINMAX, CV_8U);
    cv::resize(flat, edgeMap, cv::Size(width, height));

    return edgeMap;
}

} // namespace SketchMaster
