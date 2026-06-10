#pragma once

#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

namespace SketchMaster {

// 图像处理类：负责图片加载、预处理和边缘检测
class ImageProcessor {
public:
    ImageProcessor();
    ~ImageProcessor();

    // 加载图片
    bool loadImage(const std::string& path);
    
    // 获取原始图片
    cv::Mat getOriginalImage() const { return originalImage_; }
    
    // 获取灰度图
    cv::Mat getGrayImage() const { return grayImage_; }
    
    // 边缘检测（Canny）
    cv::Mat detectEdges(double threshold1 = 50, double threshold2 = 150);
    
    // 自适应阈值边缘检测
    cv::Mat detectEdgesAdaptive(double contrast = 1.0, int blurSize = 5);
    
    // 图像预处理：去噪、增强对比度
    cv::Mat preprocess(double contrast = 1.0, int blurSize = 5);
    
    // 二值化
    cv::Mat binarize(int threshold = 128);
    
    // 获取图片尺寸
    int getWidth() const { return originalImage_.cols; }
    int getHeight() const { return originalImage_.rows; }
    
    // 是否已加载图片
    bool isLoaded() const { return !originalImage_.empty(); }

private:
    cv::Mat originalImage_;
    cv::Mat grayImage_;
    cv::Mat processedImage_;
};

} // namespace SketchMaster
