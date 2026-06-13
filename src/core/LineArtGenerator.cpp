#include "LineArtGenerator.h"
#include <opencv2/opencv.hpp>

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

// ============================
// 内置算法：DoG (Difference of Gaussians)
// ============================
std::vector<Polyline> LineArtGenerator::generateLineArtDoG(double sigma1,
                                                           double sigma2,
                                                           double mergeDistance,
                                                           double simplifyEpsilon) {
    if (!imageProcessor_.isLoaded()) {
        return std::vector<Polyline>();
    }

    // 获取灰度图
    cv::Mat gray = imageProcessor_.getGrayImage();
    if (gray.empty()) {
        return std::vector<Polyline>();
    }

    // 转换为浮点型
    cv::Mat grayFloat;
    gray.convertTo(grayFloat, CV_64F);

    // 两个不同sigma的高斯模糊
    cv::Mat blur1, blur2;
    cv::GaussianBlur(grayFloat, blur1, cv::Size(0, 0), sigma1);
    cv::GaussianBlur(grayFloat, blur2, cv::Size(0, 0), sigma2);

    // 高斯差分
    cv::Mat dog;
    cv::subtract(blur1, blur2, dog);

    // 归一化到0-255范围
    double minVal, maxVal;
    cv::minMaxLoc(dog, &minVal, &maxVal);
    cv::Mat dogNorm;
    if (maxVal - minVal > 1e-6) {
        dog = (dog - minVal) / (maxVal - minVal) * 255.0;
    }
    dog.convertTo(dogNorm, CV_8U);

    // 反转：边缘为白色，背景为黑色
    cv::Mat dogInverted;
    cv::bitwise_not(dogNorm, dogInverted);

    // 二值化提取边缘
    cv::Mat binary;
    cv::threshold(dogInverted, binary, 30, 255, cv::THRESH_BINARY);

    edgeImage_ = binary;

    // 设置路径简化参数
    pathSimplifier_.setEpsilon(simplifyEpsilon);

    // 从边缘提取路径
    bool highPrecision = (simplifyEpsilon < 1.0);
    auto polylines = pathSimplifier_.extractFromEdges(edgeImage_, highPrecision);

    // 合并相近线段
    if (mergeDistance > 0 && polylines.size() > 1) {
        polylines = pathSimplifier_.mergeNearbyLines(polylines, mergeDistance);
    }

    return polylines;
}

// ============================
// 内置算法：Sobel 算子
// ============================
std::vector<Polyline> LineArtGenerator::generateLineArtSobel(double mergeDistance,
                                                             double simplifyEpsilon) {
    if (!imageProcessor_.isLoaded()) {
        return std::vector<Polyline>();
    }

    cv::Mat gray = imageProcessor_.getGrayImage();
    if (gray.empty()) {
        return std::vector<Polyline>();
    }

    // 计算Sobel梯度
    cv::Mat gradX, gradY;
    cv::Sobel(gray, gradX, CV_16S, 1, 0, 3);
    cv::Sobel(gray, gradY, CV_16S, 0, 1, 3);

    // 转换为绝对值并合并
    cv::Mat absGradX, absGradY;
    cv::convertScaleAbs(gradX, absGradX);
    cv::convertScaleAbs(gradY, absGradY);

    cv::Mat grad;
    cv::addWeighted(absGradX, 0.5, absGradY, 0.5, 0, grad);

    // 二值化
    cv::Mat binary;
    cv::threshold(grad, binary, 50, 255, cv::THRESH_BINARY);

    edgeImage_ = binary;

    pathSimplifier_.setEpsilon(simplifyEpsilon);
    bool highPrecision = (simplifyEpsilon < 1.0);
    auto polylines = pathSimplifier_.extractFromEdges(edgeImage_, highPrecision);

    if (mergeDistance > 0 && polylines.size() > 1) {
        polylines = pathSimplifier_.mergeNearbyLines(polylines, mergeDistance);
    }

    return polylines;
}

// ============================
// 内置算法：Laplacian 拉普拉斯
// ============================
std::vector<Polyline> LineArtGenerator::generateLineArtLaplacian(int kernelSize,
                                                                  double mergeDistance,
                                                                  double simplifyEpsilon) {
    if (!imageProcessor_.isLoaded()) {
        return std::vector<Polyline>();
    }

    cv::Mat gray = imageProcessor_.getGrayImage();
    if (gray.empty()) {
        return std::vector<Polyline>();
    }

    // 确保kernelSize为奇数
    if (kernelSize % 2 == 0) kernelSize++;

    // 高斯模糊降噪
    cv::Mat blurred;
    cv::GaussianBlur(gray, blurred, cv::Size(3, 3), 0);

    // Laplacian算子
    cv::Mat laplacian;
    cv::Laplacian(blurred, laplacian, CV_16S, kernelSize);

    // 取绝对值并转换为8位
    cv::Mat absLaplacian;
    cv::convertScaleAbs(laplacian, absLaplacian);

    // 二值化
    cv::Mat binary;
    cv::threshold(absLaplacian, binary, 30, 255, cv::THRESH_BINARY);

    edgeImage_ = binary;

    pathSimplifier_.setEpsilon(simplifyEpsilon);
    bool highPrecision = (simplifyEpsilon < 1.0);
    auto polylines = pathSimplifier_.extractFromEdges(edgeImage_, highPrecision);

    if (mergeDistance > 0 && polylines.size() > 1) {
        polylines = pathSimplifier_.mergeNearbyLines(polylines, mergeDistance);
    }

    return polylines;
}

// ============================
// 内置算法：LSD (Line Segment Detector)
// ============================
std::vector<Polyline> LineArtGenerator::generateLineArtLSD(double mergeDistance,
                                                             double simplifyEpsilon) {
    if (!imageProcessor_.isLoaded()) {
        return std::vector<Polyline>();
    }

    cv::Mat gray = imageProcessor_.getGrayImage();
    if (gray.empty()) {
        return std::vector<Polyline>();
    }

    // 创建空白边缘图像
    edgeImage_ = cv::Mat::zeros(gray.size(), CV_8U);

    try {
        // 使用OpenCV的LSD检测器
        cv::Ptr<cv::LineSegmentDetector> lsd = cv::createLineSegmentDetector(
            cv::LSD_REFINE_STD);

        std::vector<cv::Vec4f> linesStd;
        lsd->detect(gray, linesStd);

        // 将检测到的直线段转换为Polyline
        std::vector<Polyline> polylines;
        for (const auto& line : linesStd) {
            Polyline poly;
            poly.points.push_back(Point2D(line[0], line[1]));
            poly.points.push_back(Point2D(line[2], line[3]));
            poly.thickness = 1.0;
            polylines.push_back(poly);

            // 在边缘图像上绘制线段（用于可视化）
            cv::line(edgeImage_,
                     cv::Point(static_cast<int>(line[0]), static_cast<int>(line[1])),
                     cv::Point(static_cast<int>(line[2]), static_cast<int>(line[3])),
                     cv::Scalar(255), 1, cv::LINE_AA);
        }

        // 合并相近线段
        if (mergeDistance > 0 && polylines.size() > 1) {
            polylines = pathSimplifier_.mergeNearbyLines(polylines, mergeDistance);
        }

        return polylines;
    } catch (const cv::Exception& e) {
        // LSD不可用时，回退到Canny
        return generateLineArtCanny(50, 150, mergeDistance, simplifyEpsilon);
    }
}

// ============================
// 内置算法：Scharr 滤波器
// ============================
std::vector<Polyline> LineArtGenerator::generateLineArtScharr(double mergeDistance,
                                                               double simplifyEpsilon) {
    if (!imageProcessor_.isLoaded()) {
        return std::vector<Polyline>();
    }

    cv::Mat gray = imageProcessor_.getGrayImage();
    if (gray.empty()) {
        return std::vector<Polyline>();
    }

    // 计算Scharr梯度（比Sobel更精确）
    cv::Mat gradX, gradY;
    cv::Scharr(gray, gradX, CV_16S, 1, 0);
    cv::Scharr(gray, gradY, CV_16S, 0, 1);

    // 转换为绝对值并合并
    cv::Mat absGradX, absGradY;
    cv::convertScaleAbs(gradX, absGradX);
    cv::convertScaleAbs(gradY, absGradY);

    cv::Mat grad;
    cv::addWeighted(absGradX, 0.5, absGradY, 0.5, 0, grad);

    // 二值化
    cv::Mat binary;
    cv::threshold(grad, binary, 50, 255, cv::THRESH_BINARY);

    edgeImage_ = binary;

    pathSimplifier_.setEpsilon(simplifyEpsilon);
    bool highPrecision = (simplifyEpsilon < 1.0);
    auto polylines = pathSimplifier_.extractFromEdges(edgeImage_, highPrecision);

    if (mergeDistance > 0 && polylines.size() > 1) {
        polylines = pathSimplifier_.mergeNearbyLines(polylines, mergeDistance);
    }

    return polylines;
}

// ============================
// 内置算法：Morphological 形态学边缘检测
// ============================
std::vector<Polyline> LineArtGenerator::generateLineArtMorphological(int kernelSize,
                                                                       double mergeDistance,
                                                                       double simplifyEpsilon) {
    if (!imageProcessor_.isLoaded()) {
        return std::vector<Polyline>();
    }

    cv::Mat gray = imageProcessor_.getGrayImage();
    if (gray.empty()) {
        return std::vector<Polyline>();
    }

    // 确保kernelSize为奇数
    if (kernelSize % 2 == 0) kernelSize++;

    // 定义结构元素
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT,
                                                cv::Size(kernelSize, kernelSize));

    // 形态学边缘检测：膨胀减腐蚀
    cv::Mat dilated, eroded;
    cv::dilate(gray, dilated, kernel);
    cv::erode(gray, eroded, kernel);

    cv::Mat morphEdge;
    cv::subtract(dilated, eroded, morphEdge);

    // 归一化
    cv::Mat morphNorm;
    double minVal, maxVal;
    cv::minMaxLoc(morphEdge, &minVal, &maxVal);
    if (maxVal > 0) {
        morphEdge = morphEdge * 255.0 / maxVal;
    }
    morphEdge.convertTo(morphNorm, CV_8U);

    // 二值化
    cv::Mat binary;
    cv::threshold(morphNorm, binary, 30, 255, cv::THRESH_BINARY);

    edgeImage_ = binary;

    pathSimplifier_.setEpsilon(simplifyEpsilon);
    bool highPrecision = (simplifyEpsilon < 1.0);
    auto polylines = pathSimplifier_.extractFromEdges(edgeImage_, highPrecision);

    if (mergeDistance > 0 && polylines.size() > 1) {
        polylines = pathSimplifier_.mergeNearbyLines(polylines, mergeDistance);
    }

    return polylines;
}

// ============================
// 从外部边缘图像生成线稿（用于Python脚本输出的结果）
// ============================
std::vector<Polyline> LineArtGenerator::generateLineArtFromEdgeImage(const cv::Mat& edgeImage,
                                                                       double mergeDistance,
                                                                       double simplifyEpsilon) {
    if (edgeImage.empty()) {
        return std::vector<Polyline>();
    }

    // 确保边缘图像是单通道的
    if (edgeImage.channels() == 3) {
        cv::Mat gray;
        cv::cvtColor(edgeImage, gray, cv::COLOR_BGR2GRAY);
        edgeImage_ = gray;
    } else if (edgeImage.channels() == 4) {
        cv::Mat gray;
        cv::cvtColor(edgeImage, gray, cv::COLOR_BGRA2GRAY);
        edgeImage_ = gray;
    } else {
        edgeImage_ = edgeImage.clone();
    }

    // 确保是二值图像
    cv::Mat binary;
    cv::threshold(edgeImage_, binary, 127, 255, cv::THRESH_BINARY);
    edgeImage_ = binary;

    pathSimplifier_.setEpsilon(simplifyEpsilon);
    bool highPrecision = (simplifyEpsilon < 1.0);
    auto polylines = pathSimplifier_.extractFromEdges(edgeImage_, highPrecision);

    if (mergeDistance > 0 && polylines.size() > 1) {
        polylines = pathSimplifier_.mergeNearbyLines(polylines, mergeDistance);
    }

    return polylines;
}

// ============================
// 内置算法：HED 深度边缘检测 (OpenCV DNN)
// ============================
std::vector<Polyline> LineArtGenerator::generateLineArtHED(double mergeDistance,
                                                             double simplifyEpsilon) {
    if (!imageProcessor_.isLoaded()) {
        return std::vector<Polyline>();
    }

    // 尝试加载 HED 模型（如果还没加载）
    if (!hedDetector_.isLoaded()) {
        // 从多个候选路径尝试加载
        std::vector<std::string> modelPaths = {
            "models/hed/",
            "../models/hed/",
        };

        bool loaded = false;
        for (const auto& path : modelPaths) {
            if (hedDetector_.loadModel(path)) {
                loaded = true;
                break;
            }
        }

        if (!loaded) {
            std::cerr << "[LineArtGenerator] HED 模型加载失败，无法使用 HED 算法。" << std::endl;
            std::cerr << "[LineArtGenerator] 请参考 models/hed/README.md 下载模型文件。" << std::endl;
            return std::vector<Polyline>();
        }
    }

    // 检测边缘
    cv::Mat originalImage = imageProcessor_.getOriginalImage();
    edgeImage_ = hedDetector_.detectEdges(originalImage);

    if (edgeImage_.empty()) {
        std::cerr << "[LineArtGenerator] HED 边缘检测返回空结果。" << std::endl;
        return std::vector<Polyline>();
    }

    // 二值化处理（HED 输出是概率图，需要二值化）
    cv::Mat binary;
    cv::threshold(edgeImage_, binary, 128, 255, cv::THRESH_BINARY);
    edgeImage_ = binary;

    // 提取路径
    pathSimplifier_.setEpsilon(simplifyEpsilon);
    bool highPrecision = (simplifyEpsilon < 1.0);
    auto polylines = pathSimplifier_.extractFromEdges(edgeImage_, highPrecision);

    // 合并相近线段
    if (mergeDistance > 0 && polylines.size() > 1) {
        polylines = pathSimplifier_.mergeNearbyLines(polylines, mergeDistance);
    }

    return polylines;
}

// ============================
// 内置算法：MLSD 直线检测 (OpenCV DNN)
// ============================
std::vector<Polyline> LineArtGenerator::generateLineArtMLSD(double mergeDistance,
                                                              double simplifyEpsilon) {
    if (!imageProcessor_.isLoaded()) {
        return std::vector<Polyline>();
    }

    // 尝试加载 MLSD 模型（如果还没加载）
    if (!mlsdDetector_.isLoaded()) {
        // 从多个候选路径尝试加载
        std::vector<std::string> modelPaths = {
            "models/mlsd/",
            "../models/mlsd/",
        };

        bool loaded = false;
        for (const auto& path : modelPaths) {
            if (mlsdDetector_.loadModel(path)) {
                loaded = true;
                break;
            }
        }

        if (!loaded) {
            std::cerr << "[LineArtGenerator] MLSD 模型加载失败，无法使用 MLSD 算法。" << std::endl;
            std::cerr << "[LineArtGenerator] 请参考 models/mlsd/README.md 下载模型文件。" << std::endl;
            return std::vector<Polyline>();
        }
    }

    // 检测边缘
    cv::Mat originalImage = imageProcessor_.getOriginalImage();
    edgeImage_ = mlsdDetector_.detectEdges(originalImage);

    if (edgeImage_.empty()) {
        std::cerr << "[LineArtGenerator] MLSD 直线检测返回空结果。" << std::endl;
        return std::vector<Polyline>();
    }

    // 确保是二值图像
    cv::Mat binary;
    cv::threshold(edgeImage_, binary, 127, 255, cv::THRESH_BINARY);
    edgeImage_ = binary;

    // 提取路径
    pathSimplifier_.setEpsilon(simplifyEpsilon);
    bool highPrecision = (simplifyEpsilon < 1.0);
    auto polylines = pathSimplifier_.extractFromEdges(edgeImage_, highPrecision);

    // 合并相近线段
    if (mergeDistance > 0 && polylines.size() > 1) {
        polylines = pathSimplifier_.mergeNearbyLines(polylines, mergeDistance);
    }

    return polylines;
}

} // namespace SketchMaster
