#include "PathSimplifier.h"
#include <cmath>
#include <algorithm>

namespace SketchMaster {

PathSimplifier::PathSimplifier() 
    : minLineLength_(5.0), epsilon_(2.0) {}

PathSimplifier::~PathSimplifier() {}

std::vector<Polyline> PathSimplifier::extractFromEdges(const cv::Mat& edges, bool highPrecision) {
    std::vector<Polyline> result;
    
    if (edges.empty()) return result;
    
    // 查找轮廓 - 使用更精细的CHAIN_APPROX_NONE保留所有点
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours(edges.clone(), contours, hierarchy, cv::RETR_LIST, cv::CHAIN_APPROX_NONE);
    
    for (const auto& contour : contours) {
        if (contour.size() < 2) continue;
        
        // 转换为Polyline
        Polyline poly;
        for (const auto& pt : contour) {
            poly.points.emplace_back(pt.x, pt.y);
        }
        
        if (highPrecision) {
            // 高精度模式：减少简化，保留更多细节
            // 只进行轻微的平滑，不进行大幅度简化
            if (poly.points.size() > 3) {
                poly.points = smoothPath(poly.points, 3);
            }
            // 使用更小的epsilon进行轻微简化
            poly.points = douglasPeucker(poly.points, 0.5);
        } else {
            // 标准模式
            poly.points = douglasPeucker(poly.points, epsilon_);
            if (poly.points.size() > 3) {
                poly.points = smoothPath(poly.points, 3);
            }
        }
        
        // 过滤短路径
        if (poly.points.size() >= 2) {
            double length = calculatePathLength(poly.points);
            if (length >= minLineLength_) {
                result.push_back(poly);
            }
        }
    }
    
    return result;
}

std::vector<Point2D> PathSimplifier::douglasPeucker(const std::vector<Point2D>& points, double epsilon) {
    if (points.size() <= 2) return points;
    
    // 找到距离线段最远的点
    double maxDist = 0;
    size_t maxIndex = 0;
    
    LineSegment seg(points.front(), points.back());
    
    for (size_t i = 1; i < points.size() - 1; ++i) {
        double dist = pointToSegmentDistance(points[i], seg);
        if (dist > maxDist) {
            maxDist = dist;
            maxIndex = i;
        }
    }
    
    if (maxDist > epsilon) {
        // 递归处理
        std::vector<Point2D> left(points.begin(), points.begin() + maxIndex + 1);
        std::vector<Point2D> right(points.begin() + maxIndex, points.end());
        
        auto leftResult = douglasPeucker(left, epsilon);
        auto rightResult = douglasPeucker(right, epsilon);
        
        // 合并结果
        std::vector<Point2D> result = leftResult;
        result.insert(result.end(), rightResult.begin() + 1, rightResult.end());
        return result;
    } else {
        return {points.front(), points.back()};
    }
}

std::vector<BezierCurve> PathSimplifier::fitBezierCurves(const std::vector<Point2D>& points, double errorThreshold) {
    std::vector<BezierCurve> result;
    
    if (points.size() < 4) {
        // 点太少，直接作为直线段
        if (points.size() >= 2) {
            BezierCurve curve(points.front(), points.front(), points.back(), points.back());
            result.push_back(curve);
        }
        return result;
    }
    
    // 将路径分段拟合为贝塞尔曲线
    size_t startIdx = 0;
    
    while (startIdx < points.size() - 1) {
        // 尝试用4个点拟合一段贝塞尔曲线
        size_t endIdx = std::min(startIdx + 3, points.size() - 1);
        
        // 计算控制点（使用相邻点的切线方向）
        Point2D p0 = points[startIdx];
        Point2D p3 = points[endIdx];
        
        // 计算切线方向
        Point2D tangent1(0, 0), tangent2(0, 0);
        if (startIdx + 1 < points.size()) {
            tangent1.x = points[startIdx + 1].x - p0.x;
            tangent1.y = points[startIdx + 1].y - p0.y;
        }
        if (endIdx > 0) {
            tangent2.x = p3.x - points[endIdx - 1].x;
            tangent2.y = p3.y - points[endIdx - 1].y;
        }
        
        // 归一化切线并缩放
        auto normalize = [](Point2D& v) {
            double len = std::sqrt(v.x * v.x + v.y * v.y);
            if (len > 0) {
                v.x /= len;
                v.y /= len;
            }
        };
        normalize(tangent1);
        normalize(tangent2);
        
        // 控制点距离（使用弦长的1/3）
        double chordLen = std::sqrt((p3.x - p0.x) * (p3.x - p0.x) + (p3.y - p0.y) * (p3.y - p0.y));
        double cpDist = chordLen / 3.0;
        
        Point2D p1(p0.x + tangent1.x * cpDist, p0.y + tangent1.y * cpDist);
        Point2D p2(p3.x - tangent2.x * cpDist, p3.y - tangent2.y * cpDist);
        
        result.emplace_back(p0, p1, p2, p3);
        
        startIdx = endIdx;
    }
    
    return result;
}

std::vector<Point2D> PathSimplifier::sampleBezierCurve(const BezierCurve& curve, int numPoints) {
    std::vector<Point2D> result;
    result.reserve(numPoints);
    
    for (int i = 0; i <= numPoints; ++i) {
        double t = i / static_cast<double>(numPoints);
        
        // 三次贝塞尔曲线公式
        double mt = 1 - t;
        double mt2 = mt * mt;
        double mt3 = mt2 * mt;
        double t2 = t * t;
        double t3 = t2 * t;
        
        double x = mt3 * curve.p0.x + 3 * mt2 * t * curve.p1.x +
                   3 * mt * t2 * curve.p2.x + t3 * curve.p3.x;
        double y = mt3 * curve.p0.y + 3 * mt2 * t * curve.p1.y +
                   3 * mt * t2 * curve.p2.y + t3 * curve.p3.y;
        
        result.emplace_back(x, y);
    }
    
    return result;
}

std::vector<Polyline> PathSimplifier::mergeNearbyLines(std::vector<Polyline>& polylines, double mergeDistance) {
    if (polylines.size() < 2) return polylines;
    
    std::vector<bool> merged(polylines.size(), false);
    std::vector<Polyline> result;
    
    for (size_t i = 0; i < polylines.size(); ++i) {
        if (merged[i]) continue;
        
        Polyline current = polylines[i];
        merged[i] = true;
        
        bool foundMerge;
        do {
            foundMerge = false;
            for (size_t j = 0; j < polylines.size(); ++j) {
                if (merged[j]) continue;
                
                if (canMerge(current, polylines[j], mergeDistance)) {
                    current = mergePolylines(current, polylines[j]);
                    merged[j] = true;
                    foundMerge = true;
                    break;
                }
            }
        } while (foundMerge);
        
        result.push_back(current);
    }
    
    return result;
}

std::vector<Point2D> PathSimplifier::smoothPath(const std::vector<Point2D>& points, int windowSize) {
    if (points.size() <= 3 || windowSize < 2) return points;
    
    std::vector<Point2D> result;
    result.reserve(points.size());
    
    int halfWindow = windowSize / 2;
    
    for (size_t i = 0; i < points.size(); ++i) {
        double sumX = 0, sumY = 0;
        int count = 0;
        
        for (int j = -halfWindow; j <= halfWindow; ++j) {
            int idx = static_cast<int>(i) + j;
            if (idx >= 0 && idx < static_cast<int>(points.size())) {
                sumX += points[idx].x;
                sumY += points[idx].y;
                ++count;
            }
        }
        
        result.emplace_back(sumX / count, sumY / count);
    }
    
    return result;
}

double PathSimplifier::pointToSegmentDistance(const Point2D& p, const LineSegment& seg) {
    double dx = seg.end.x - seg.start.x;
    double dy = seg.end.y - seg.start.y;
    
    if (dx == 0 && dy == 0) {
        dx = p.x - seg.start.x;
        dy = p.y - seg.start.y;
        return std::sqrt(dx*dx + dy*dy);
    }
    
    double t = ((p.x - seg.start.x) * dx + (p.y - seg.start.y) * dy) / (dx*dx + dy*dy);
    t = std::max(0.0, std::min(1.0, t));
    
    double projX = seg.start.x + t * dx;
    double projY = seg.start.y + t * dy;
    
    double distX = p.x - projX;
    double distY = p.y - projY;
    
    return std::sqrt(distX*distX + distY*distY);
}

bool PathSimplifier::canMerge(const Polyline& a, const Polyline& b, double threshold) {
    if (a.points.empty() || b.points.empty()) return false;
    
    // 检查端点之间的距离
    const Point2D& aStart = a.points.front();
    const Point2D& aEnd = a.points.back();
    const Point2D& bStart = b.points.front();
    const Point2D& bEnd = b.points.back();
    
    auto dist = [](const Point2D& p1, const Point2D& p2) {
        double dx = p1.x - p2.x;
        double dy = p1.y - p2.y;
        return std::sqrt(dx*dx + dy*dy);
    };
    
    // 如果任一端点接近，则可以合并
    if (dist(aEnd, bStart) < threshold || dist(aStart, bEnd) < threshold ||
        dist(aEnd, bEnd) < threshold || dist(aStart, bStart) < threshold) {
        return true;
    }
    
    // 检查线段之间的最小距离
    for (size_t i = 0; i < a.points.size() - 1; ++i) {
        LineSegment segA(a.points[i], a.points[i+1]);
        for (size_t j = 0; j < b.points.size() - 1; ++j) {
            if (pointToSegmentDistance(b.points[j], segA) < threshold ||
                pointToSegmentDistance(b.points[j+1], segA) < threshold) {
                return true;
            }
        }
    }
    
    return false;
}

Polyline PathSimplifier::mergePolylines(const Polyline& a, const Polyline& b) {
    Polyline result;
    
    auto dist = [](const Point2D& p1, const Point2D& p2) {
        double dx = p1.x - p2.x;
        double dy = p1.y - p2.y;
        return std::sqrt(dx*dx + dy*dy);
    };
    
    const Point2D& aStart = a.points.front();
    const Point2D& aEnd = a.points.back();
    const Point2D& bStart = b.points.front();
    const Point2D& bEnd = b.points.back();
    
    double d1 = dist(aEnd, bStart);
    double d2 = dist(aStart, bEnd);
    double d3 = dist(aEnd, bEnd);
    double d4 = dist(aStart, bStart);
    
    double minDist = std::min({d1, d2, d3, d4});
    
    if (minDist == d1) {
        // aEnd -> bStart
        result.points = a.points;
        result.points.insert(result.points.end(), b.points.begin(), b.points.end());
    } else if (minDist == d2) {
        // bEnd -> aStart
        result.points = b.points;
        result.points.insert(result.points.end(), a.points.begin(), a.points.end());
    } else if (minDist == d3) {
        // aEnd -> bEnd (反转b)
        result.points = a.points;
        result.points.insert(result.points.end(), b.points.rbegin(), b.points.rend());
    } else {
        // aStart -> bStart (反转a)
        result.points.assign(a.points.rbegin(), a.points.rend());
        result.points.insert(result.points.end(), b.points.begin(), b.points.end());
    }
    
    result.thickness = (a.thickness + b.thickness) / 2.0;
    
    // 简化合并后的路径
    result.points = douglasPeucker(result.points, epsilon_);
    
    return result;
}

double PathSimplifier::calculatePathLength(const std::vector<Point2D>& points) {
    if (points.size() < 2) return 0.0;
    
    double length = 0.0;
    for (size_t i = 1; i < points.size(); ++i) {
        double dx = points[i].x - points[i-1].x;
        double dy = points[i].y - points[i-1].y;
        length += std::sqrt(dx*dx + dy*dy);
    }
    return length;
}

double PathSimplifier::findClosestT(const Point2D& p, const BezierCurve& curve) {
    // 使用二分搜索找到曲线上距离p最近的参数t
    double bestT = 0;
    double bestDist = std::numeric_limits<double>::max();
    
    const int samples = 100;
    for (int i = 0; i <= samples; ++i) {
        double t = i / static_cast<double>(samples);
        
        double mt = 1 - t;
        double mt2 = mt * mt;
        double mt3 = mt2 * mt;
        double t2 = t * t;
        double t3 = t2 * t;
        
        double x = mt3 * curve.p0.x + 3 * mt2 * t * curve.p1.x +
                   3 * mt * t2 * curve.p2.x + t3 * curve.p3.x;
        double y = mt3 * curve.p0.y + 3 * mt2 * t * curve.p1.y +
                   3 * mt * t2 * curve.p2.y + t3 * curve.p3.y;
        
        double dist = std::sqrt((x - p.x) * (x - p.x) + (y - p.y) * (y - p.y));
        if (dist < bestDist) {
            bestDist = dist;
            bestT = t;
        }
    }
    
    return bestT;
}

} // namespace SketchMaster
