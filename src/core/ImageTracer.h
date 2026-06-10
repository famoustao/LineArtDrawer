#pragma once

#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

namespace SketchMaster {

// 图像追踪参数
struct TraceParams {
    int colors = 16;           // 颜色数量（量化级别）
    int blurSize = 1;          // 模糊核大小（去噪）
    double curveThreshold = 1.0; // 曲线拟合阈值
    bool fillHoles = true;     // 填充小孔洞
    int minArea = 20;          // 最小区域面积（过滤噪点）
    bool smoothPaths = true;   // 平滑路径
};

// 图像追踪器：将位图转换为保留原图外观的SVG
class ImageTracer {
public:
    ImageTracer();
    ~ImageTracer();

    // 加载图片
    bool loadImage(const std::string& path);
    bool loadImage(const cv::Mat& image);

    // 设置参数
    void setParams(const TraceParams& params) { params_ = params; }
    TraceParams getParams() const { return params_; }

    // 执行追踪并导出SVG
    bool traceToSVG(const std::string& outputPath);

    // 获取追踪后的SVG字符串
    std::string getSVGString();

    // 获取原始图片尺寸
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }

    // 获取错误信息
    std::string getError() const { return error_; }

private:
    cv::Mat sourceImage_;
    cv::Mat quantizedImage_;
    int width_;
    int height_;
    TraceParams params_;
    std::string error_;

    // 颜色量化（减少颜色数量）
    void quantizeColors();

    // 提取每个颜色的区域
    void extractColorRegions(std::vector<std::vector<std::vector<cv::Point>>>& regions,
                             std::vector<cv::Vec3b>& colors);

    // 将轮廓转换为平滑路径（贝塞尔曲线）
    std::string contourToPath(const std::vector<cv::Point>& contour);

    // 简化轮廓点（使用Douglas-Peucker）
    std::vector<cv::Point> simplifyContour(const std::vector<cv::Point>& contour);

    // 将点序列转换为SVG路径数据
    std::string pointsToPathData(const std::vector<cv::Point>& points, bool closed);

    // 颜色转十六进制
    std::string colorToHex(const cv::Vec3b& color);

    // 生成SVG内容
    std::string generateSVG(const std::vector<std::vector<std::vector<cv::Point>>>& regions,
                            const std::vector<cv::Vec3b>& colors);
};

} // namespace SketchMaster
