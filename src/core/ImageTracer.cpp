#include "ImageTracer.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace SketchMaster {

ImageTracer::ImageTracer()
    : width_(0), height_(0) {}

ImageTracer::~ImageTracer() {}

bool ImageTracer::loadImage(const std::string& path) {
    sourceImage_ = cv::imread(path, cv::IMREAD_COLOR);
    if (sourceImage_.empty()) {
        error_ = "无法加载图片: " + path;
        return false;
    }
    // 转换为RGB
    cv::cvtColor(sourceImage_, sourceImage_, cv::COLOR_BGR2RGB);
    width_ = sourceImage_.cols;
    height_ = sourceImage_.rows;
    return true;
}

bool ImageTracer::loadImage(const cv::Mat& image) {
    if (image.empty()) {
        error_ = "空图片";
        return false;
    }
    if (image.channels() == 4) {
        cv::cvtColor(image, sourceImage_, cv::COLOR_BGRA2RGB);
    } else if (image.channels() == 3) {
        cv::cvtColor(image, sourceImage_, cv::COLOR_BGR2RGB);
    } else if (image.channels() == 1) {
        cv::cvtColor(image, sourceImage_, cv::COLOR_GRAY2RGB);
    } else {
        sourceImage_ = image.clone();
    }
    width_ = sourceImage_.cols;
    height_ = sourceImage_.rows;
    return true;
}

void ImageTracer::quantizeColors() {
    if (sourceImage_.empty()) return;

    cv::Mat blurred;
    if (params_.blurSize > 1) {
        cv::GaussianBlur(sourceImage_, blurred,
                         cv::Size(params_.blurSize * 2 + 1, params_.blurSize * 2 + 1), 0);
    } else {
        blurred = sourceImage_.clone();
    }

    // K-Means颜色量化
    cv::Mat samples = blurred.reshape(1, blurred.total());
    samples.convertTo(samples, CV_32F);

    int clusterCount = std::max(2, std::min(256, params_.colors));
    cv::Mat labels, centers;
    cv::TermCriteria criteria(cv::TermCriteria::MAX_ITER + cv::TermCriteria::EPS, 10, 1.0);
    cv::kmeans(samples, clusterCount, labels, criteria, 3, cv::KMEANS_PP_CENTERS, centers);

    // 重建量化图像
    quantizedImage_ = cv::Mat(blurred.size(), CV_8UC3);
    for (int i = 0; i < blurred.rows; ++i) {
        for (int j = 0; j < blurred.cols; ++j) {
            int idx = labels.at<int>(i * blurred.cols + j);
            quantizedImage_.at<cv::Vec3b>(i, j) = cv::Vec3b(
                static_cast<uchar>(centers.at<float>(idx, 2)),
                static_cast<uchar>(centers.at<float>(idx, 1)),
                static_cast<uchar>(centers.at<float>(idx, 0))
            );
        }
    }
}

void ImageTracer::extractColorRegions(std::vector<std::vector<std::vector<cv::Point>>>& regions,
                                      std::vector<cv::Vec3b>& colors) {
    if (quantizedImage_.empty()) {
        quantizeColors();
    }

    // 收集所有唯一颜色
    std::vector<cv::Vec3b> uniqueColors;
    for (int i = 0; i < quantizedImage_.rows; ++i) {
        for (int j = 0; j < quantizedImage_.cols; ++j) {
            cv::Vec3b color = quantizedImage_.at<cv::Vec3b>(i, j);
            bool found = false;
            for (const auto& c : uniqueColors) {
                if (c[0] == color[0] && c[1] == color[1] && c[2] == color[2]) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                uniqueColors.push_back(color);
            }
        }
    }

    // 为每种颜色提取轮廓
    for (const auto& color : uniqueColors) {
        cv::Mat mask(quantizedImage_.size(), CV_8UC1);
        for (int i = 0; i < quantizedImage_.rows; ++i) {
            for (int j = 0; j < quantizedImage_.cols; ++j) {
                cv::Vec3b px = quantizedImage_.at<cv::Vec3b>(i, j);
                mask.at<uchar>(i, j) = (px[0] == color[0] && px[1] == color[1] && px[2] == color[2]) ? 255 : 0;
            }
        }

        // 形态学操作：填充孔洞
        if (params_.fillHoles) {
            cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
            cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernel);
        }

        // 提取轮廓
        std::vector<std::vector<cv::Point>> contours;
        std::vector<cv::Vec4i> hierarchy;
        cv::findContours(mask, contours, hierarchy, cv::RETR_CCOMP, cv::CHAIN_APPROX_SIMPLE);

        // 过滤小区域并简化轮廓
        std::vector<std::vector<cv::Point>> validContours;
        for (size_t i = 0; i < contours.size(); ++i) {
            double area = cv::contourArea(contours[i]);
            if (area >= params_.minArea) {
                if (params_.smoothPaths) {
                    validContours.push_back(simplifyContour(contours[i]));
                } else {
                    validContours.push_back(contours[i]);
                }
            }
        }

        if (!validContours.empty()) {
            regions.push_back(validContours);
            colors.push_back(color);
        }
    }
}

std::vector<cv::Point> ImageTracer::simplifyContour(const std::vector<cv::Point>& contour) {
    std::vector<cv::Point> simplified;
    double epsilon = params_.curveThreshold;
    cv::approxPolyDP(contour, simplified, epsilon, true);

    // 如果简化后点数太少，回退到原始轮廓
    if (simplified.size() < 3 && contour.size() >= 3) {
        return contour;
    }

    return simplified;
}

std::string ImageTracer::pointsToPathData(const std::vector<cv::Point>& points, bool closed) {
    if (points.empty()) return "";

    std::ostringstream pathData;
    pathData << std::fixed << std::setprecision(1);

    // 移动到第一个点
    pathData << "M " << points[0].x << " " << points[0].y;

    // 使用直线段连接（简化后的轮廓点已经够少，用直线即可）
    for (size_t i = 1; i < points.size(); ++i) {
        pathData << " L " << points[i].x << " " << points[i].y;
    }

    if (closed) {
        pathData << " Z";
    }

    return pathData.str();
}

std::string ImageTracer::contourToPath(const std::vector<cv::Point>& contour) {
    auto simplified = simplifyContour(contour);
    return pointsToPathData(simplified, true);
}

std::string ImageTracer::colorToHex(const cv::Vec3b& color) {
    std::ostringstream hex;
    hex << "#" << std::hex << std::setfill('0') << std::setw(2) << (int)color[0]
        << std::setw(2) << (int)color[1]
        << std::setw(2) << (int)color[2];
    return hex.str();
}

std::string ImageTracer::generateSVG(const std::vector<std::vector<std::vector<cv::Point>>>& regions,
                                     const std::vector<cv::Vec3b>& colors) {
    std::ostringstream svg;

    svg << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n";
    svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" ";
    svg << "width=\"" << width_ << "\" height=\"" << height_ << "\" ";
    svg << "viewBox=\"0 0 " << width_ << " " << height_ << "\">\n";

    // 背景（第一种颜色通常是背景色，按面积排序后最大的放最底层）
    // 这里按原有顺序绘制，后面的覆盖前面的
    for (size_t r = 0; r < regions.size(); ++r) {
        std::string fillColor = colorToHex(colors[r]);

        for (const auto& contour : regions[r]) {
            std::string pathData = pointsToPathData(contour, true);
            if (pathData.empty()) continue;

            svg << "  <path d=\"" << pathData << "\" ";
            svg << "fill=\"" << fillColor << "\" ";
            svg << "stroke=\"none\"/>\n";
        }
    }

    svg << "</svg>\n";
    return svg.str();
}

bool ImageTracer::traceToSVG(const std::string& outputPath) {
    if (sourceImage_.empty()) {
        error_ = "未加载图片";
        return false;
    }

    std::vector<std::vector<std::vector<cv::Point>>> regions;
    std::vector<cv::Vec3b> colors;

    extractColorRegions(regions, colors);

    if (regions.empty()) {
        error_ = "未能提取任何颜色区域";
        return false;
    }

    std::string svgContent = generateSVG(regions, colors);

    std::ofstream file(outputPath);
    if (!file.is_open()) {
        error_ = "无法创建文件: " + outputPath;
        return false;
    }

    file << svgContent;
    file.close();

    return true;
}

std::string ImageTracer::getSVGString() {
    if (sourceImage_.empty()) {
        error_ = "未加载图片";
        return "";
    }

    std::vector<std::vector<std::vector<cv::Point>>> regions;
    std::vector<cv::Vec3b> colors;

    extractColorRegions(regions, colors);

    if (regions.empty()) {
        error_ = "未能提取任何颜色区域";
        return "";
    }

    return generateSVG(regions, colors);
}

} // namespace SketchMaster
