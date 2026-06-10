#include "LineArtGenerator.h"

namespace SketchMaster {

LineArtGenerator::LineArtGenerator() {}

LineArtGenerator::~LineArtGenerator() {}

bool LineArtGenerator::loadImage(const std::string& path) {
    return imageProcessor_.loadImage(path);
}

std::vector<Polyline> LineArtGenerator::generateLineArt(double contrast,
                                                        int blurSize,
                                                        double mergeDistance,
                                                        double simplifyEpsilon) {
    if (!imageProcessor_.isLoaded()) {
        return std::vector<Polyline>();
    }
    
    // 自适应阈值边缘检测
    edgeImage_ = imageProcessor_.detectEdgesAdaptive(contrast, blurSize);
    
    // 设置路径简化参数
    pathSimplifier_.setEpsilon(simplifyEpsilon);
    
    // 从边缘提取路径 - 使用高精度模式保留更多细节
    bool highPrecision = (simplifyEpsilon < 1.0);
    auto polylines = pathSimplifier_.extractFromEdges(edgeImage_, highPrecision);
    
    // 合并相近线段
    if (mergeDistance > 0 && polylines.size() > 1) {
        polylines = pathSimplifier_.mergeNearbyLines(polylines, mergeDistance);
    }
    
    return polylines;
}

std::vector<Polyline> LineArtGenerator::generateLineArtCanny(double threshold1,
                                                             double threshold2,
                                                             double mergeDistance,
                                                             double simplifyEpsilon) {
    if (!imageProcessor_.isLoaded()) {
        return std::vector<Polyline>();
    }
    
    // Canny边缘检测
    edgeImage_ = imageProcessor_.detectEdges(threshold1, threshold2);
    
    // 设置路径简化参数
    pathSimplifier_.setEpsilon(simplifyEpsilon);
    
    // 从边缘提取路径 - 使用高精度模式保留更多细节
    bool highPrecision = (simplifyEpsilon < 1.0);
    auto polylines = pathSimplifier_.extractFromEdges(edgeImage_, highPrecision);
    
    // 合并相近线段
    if (mergeDistance > 0 && polylines.size() > 1) {
        polylines = pathSimplifier_.mergeNearbyLines(polylines, mergeDistance);
    }
    
    return polylines;
}

cv::Mat LineArtGenerator::getOriginalImage() const {
    return imageProcessor_.getOriginalImage();
}

cv::Mat LineArtGenerator::getOriginalImageCV() const {
    return imageProcessor_.getOriginalImage();
}

int LineArtGenerator::getWidth() const {
    return imageProcessor_.getWidth();
}

int LineArtGenerator::getHeight() const {
    return imageProcessor_.getHeight();
}

} // namespace SketchMaster
