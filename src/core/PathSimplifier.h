#pragma once

#include <vector>
#include <opencv2/opencv.hpp>

namespace SketchMaster {

// 点结构
struct Point2D {
    double x, y;
    Point2D(double x = 0, double y = 0) : x(x), y(y) {}
    
    bool operator==(const Point2D& other) const {
        return std::abs(x - other.x) < 1e-6 && std::abs(y - other.y) < 1e-6;
    }
};

// 线段结构
struct LineSegment {
    Point2D start;
    Point2D end;
    double thickness;
    
    LineSegment(const Point2D& s = Point2D(), const Point2D& e = Point2D(), double t = 1.0)
        : start(s), end(e), thickness(t) {}
};

// 路径（多段线）
struct Polyline {
    std::vector<Point2D> points;
    double thickness;
    
    Polyline(double t = 1.0) : thickness(t) {}
};

// 贝塞尔曲线段
struct BezierCurve {
    Point2D p0, p1, p2, p3;  // 起点、控制点1、控制点2、终点
    double thickness;
    
    BezierCurve(const Point2D& start = Point2D(), const Point2D& cp1 = Point2D(),
                const Point2D& cp2 = Point2D(), const Point2D& end = Point2D(), double t = 1.0)
        : p0(start), p1(cp1), p2(cp2), p3(end), thickness(t) {}
};

// 路径简化与合并类
class PathSimplifier {
public:
    PathSimplifier();
    ~PathSimplifier();

    // 从边缘图像提取轮廓并转换为路径（高精度模式）
    std::vector<Polyline> extractFromEdges(const cv::Mat& edges, bool highPrecision = true);
    
    // Douglas-Peucker算法简化路径
    static std::vector<Point2D> douglasPeucker(const std::vector<Point2D>& points, double epsilon);
    
    // 将路径点拟合为贝塞尔曲线
    static std::vector<BezierCurve> fitBezierCurves(const std::vector<Point2D>& points, double errorThreshold = 2.0);
    
    // 在贝塞尔曲线上采样点（用于鼠标绘制）
    static std::vector<Point2D> sampleBezierCurve(const BezierCurve& curve, int numPoints);
    
    // 合并相近线段
    std::vector<Polyline> mergeNearbyLines(std::vector<Polyline>& polylines, double mergeDistance = 5.0);
    
    // 平滑路径
    static std::vector<Point2D> smoothPath(const std::vector<Point2D>& points, int windowSize = 3);
    
    // 设置最小线段长度过滤
    void setMinLineLength(double length) { minLineLength_ = length; }
    
    // 设置简化阈值
    void setEpsilon(double epsilon) { epsilon_ = epsilon; }

private:
    double minLineLength_;
    double epsilon_;
    
    // 计算点到线段的距离
    static double pointToSegmentDistance(const Point2D& p, const LineSegment& seg);
    
    // 检查两条线段是否可以合并
    bool canMerge(const Polyline& a, const Polyline& b, double threshold);
    
    // 合并两条线段
    Polyline mergePolylines(const Polyline& a, const Polyline& b);
    
    // 计算路径总长度
    static double calculatePathLength(const std::vector<Point2D>& points);
    
    // 在路径上找到距离某点最近的参数t
    static double findClosestT(const Point2D& p, const BezierCurve& curve);
};

} // namespace SketchMaster
