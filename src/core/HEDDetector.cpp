#include "HEDDetector.h"
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

namespace SketchMaster {

HEDDetector::HEDDetector()
    : netLoaded_(false) {
}

HEDDetector::~HEDDetector() {
}

std::string HEDDetector::findModelFile(const std::string& modelDir, const std::string& filename) {
    std::vector<std::string> searchPaths = {
        modelDir + "/" + filename,
        "models/hed/" + filename,
        "../models/hed/" + filename,
    };

    for (const auto& path : searchPaths) {
        if (fs::exists(path)) {
            return path;
        }
    }

    return "";
}

bool HEDDetector::loadModel(const std::string& modelDir) {
    lastError_.clear();

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
        net_ = cv::dnn::readNetFromCaffe(prototxtPath, caffemodelPath);
        net_.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
        net_.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
        netLoaded_ = true;
        std::cout << "[HEDDetector] 模型加载成功: " << prototxtPath << std::endl;
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
        cv::Mat resized;
        cv::resize(inputImage, resized, cv::Size(500, 500));

        cv::Mat blob = cv::dnn::blobFromImage(resized, 1.0, cv::Size(500, 500),
            cv::Scalar(104.00698793, 116.66876762, 122.67891434),
            false, false);

        net_.setInput(blob);
        cv::Mat out = net_.forward();

        cv::Mat edges = out.reshape(1, out.size[2]);
        cv::normalize(edges, edges, 0, 255, cv::NORM_MINMAX, CV_8U);

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
