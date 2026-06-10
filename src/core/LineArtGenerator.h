#pragma once

#include "ImageProcessor.h"
#include "PathSimplifier.h"
#include <vector>

namespace SketchMaster {

// 线稿生成器：整合图像处理和路径提取
class LineArtGenerator {
public:
    LineArtGenerator();
    ~LineArtGenerator();

    // 加载图片
    bool loadImage(const std::string& path);
    
    // 生成线稿
    std::vector<Polyline> generateLineArt(double contrast = 1.0,
                                          int blurSize = 5,
                                          double mergeDistance = 5.0,
                                          double simplifyEpsilon = 2.0);
    
    // 使用Canny边缘检测生成线稿
    std::vector<Polyline> generateLineArtCanny(double threshold1 = 50,
                                               double threshold2 = 150,
                                               double mergeDistance = 5.0,
                                               double simplifyEpsilon = 2.0);
    
    // 获取处理后的边缘图像
    cv::Mat getEdgeImage() const { return edgeImage_; }
    
    // 获取原始图像
    cv::Mat getOriginalImage() const;
    
    // 获取原始图像 (OpenCV格式，用于图像追踪)
    cv::Mat getOriginalImageCV() const;
    
    // 获取图像尺寸
    int getWidth() const;
    int getHeight() const;

private:
    ImageProcessor imageProcessor_;
    PathSimplifier pathSimplifier_;
    cv::Mat edgeImage_;
};

} // namespace SketchMaster
