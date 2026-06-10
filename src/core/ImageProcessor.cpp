#include "ImageProcessor.h"
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

namespace SketchMaster {

ImageProcessor::ImageProcessor() {}

ImageProcessor::~ImageProcessor() {}

bool ImageProcessor::loadImage(const std::string& path) {
    originalImage_ = cv::imread(path, cv::IMREAD_COLOR);
    if (originalImage_.empty()) {
        return false;
    }
    cv::cvtColor(originalImage_, grayImage_, cv::COLOR_BGR2GRAY);
    return true;
}

cv::Mat ImageProcessor::detectEdges(double threshold1, double threshold2) {
    if (grayImage_.empty()) return cv::Mat();
    
    cv::Mat edges;
    cv::Canny(grayImage_, edges, threshold1, threshold2);
    return edges;
}

cv::Mat ImageProcessor::detectEdgesAdaptive(double contrast, int blurSize) {
    if (grayImage_.empty()) return cv::Mat();
    
    cv::Mat processed = preprocess(contrast, blurSize);
    cv::Mat edges;
    cv::adaptiveThreshold(processed, edges, 255, 
                          cv::ADAPTIVE_THRESH_GAUSSIAN_C, 
                          cv::THRESH_BINARY_INV, 11, 2);
    return edges;
}

cv::Mat ImageProcessor::preprocess(double contrast, int blurSize) {
    if (grayImage_.empty()) return cv::Mat();
    
    cv::Mat result;
    
    // 高斯模糊去噪
    if (blurSize > 0 && blurSize % 2 == 1) {
        cv::GaussianBlur(grayImage_, result, cv::Size(blurSize, blurSize), 0);
    } else {
        result = grayImage_.clone();
    }
    
    // 对比度增强
    if (contrast != 1.0) {
        result.convertTo(result, -1, contrast, 0);
    }
    
    processedImage_ = result.clone();
    return result;
}

cv::Mat ImageProcessor::binarize(int threshold) {
    if (grayImage_.empty()) return cv::Mat();
    
    cv::Mat binary;
    cv::threshold(grayImage_, binary, threshold, 255, cv::THRESH_BINARY_INV);
    return binary;
}

} // namespace SketchMaster
