#include "MousePainter.h"
#include "../core/PathSimplifier.h"
#include <cmath>
#include <chrono>
#include <thread>
#include <QDebug>

namespace SketchMaster {

MousePainter::MousePainter(QObject* parent)
    : QObject(parent)
    , speed_(1.0)
    , speedLevel_(DrawSpeedLevel::Normal)
    , offsetX_(0)
    , offsetY_(0)
    , scale_(1.0)
    , areaSelector_(nullptr)
    , lineArtWidth_(0)
    , lineArtHeight_(0)
    , lineArtOriginX_(0)
    , lineArtOriginY_(0)
    , preDrawDelayMs_(0)
    , precision_(DrawPrecision::Normal)
    , state_(DrawingState::Idle)
    , running_(false)
    , paused_(false)
    , currentLineIndex_(0)
    , currentPointIndex_(0) {}

MousePainter::~MousePainter() {
    stopDrawing();
    if (drawingThread_.joinable()) {
        drawingThread_.join();
    }
}

bool MousePainter::initialize() {
    return mouseController_.initialize();
}

void MousePainter::setPolylines(const std::vector<Polyline>& polylines) {
    polylines_ = polylines;
}

void MousePainter::setSpeed(double speed) {
    speed_ = std::max(0.1, std::min(10.0, speed));
    emit speedChanged(speed_);
}

void MousePainter::setSpeedLevel(DrawSpeedLevel level) {
    speedLevel_ = level;
    speed_ = speedLevelToMultiplier(level);
    emit speedChanged(speed_);
}

void MousePainter::setOffset(int offsetX, int offsetY) {
    offsetX_ = offsetX;
    offsetY_ = offsetY;
}

void MousePainter::setScale(double scale) {
    scale_ = std::max(0.1, std::min(10.0, scale));
}

void MousePainter::setDrawingAreaSelector(DrawingAreaSelector* selector) {
    areaSelector_ = selector;
}

void MousePainter::setLineArtSize(int width, int height) {
    lineArtWidth_ = width;
    lineArtHeight_ = height;
}

void MousePainter::setLineArtOrigin(double originX, double originY) {
    lineArtOriginX_ = originX;
    lineArtOriginY_ = originY;
}

void MousePainter::setPreDrawDelay(int delayMs) {
    preDrawDelayMs_ = delayMs;
}

void MousePainter::setPrecision(DrawPrecision precision) {
    precision_ = precision;
    emit precisionChanged(precision == DrawPrecision::PixelPerfect);
}

void MousePainter::setPressure(PressureLevel pressure) {
    mouseController_.setPressure(pressure);
    emit pressureChanged(static_cast<int>(pressure));
}

void MousePainter::speedUp() {
    switch (speedLevel_) {
        case DrawSpeedLevel::VerySlow: setSpeedLevel(DrawSpeedLevel::Slow); break;
        case DrawSpeedLevel::Slow: setSpeedLevel(DrawSpeedLevel::Normal); break;
        case DrawSpeedLevel::Normal: setSpeedLevel(DrawSpeedLevel::Fast); break;
        case DrawSpeedLevel::Fast: setSpeedLevel(DrawSpeedLevel::VeryFast); break;
        case DrawSpeedLevel::VeryFast: break; // 已经是最高速
    }
}

void MousePainter::speedDown() {
    switch (speedLevel_) {
        case DrawSpeedLevel::VerySlow: break; // 已经是最低速
        case DrawSpeedLevel::Slow: setSpeedLevel(DrawSpeedLevel::VerySlow); break;
        case DrawSpeedLevel::Normal: setSpeedLevel(DrawSpeedLevel::Slow); break;
        case DrawSpeedLevel::Fast: setSpeedLevel(DrawSpeedLevel::Normal); break;
        case DrawSpeedLevel::VeryFast: setSpeedLevel(DrawSpeedLevel::Fast); break;
    }
}

void MousePainter::togglePrecision() {
    if (precision_ == DrawPrecision::Normal) {
        setPrecision(DrawPrecision::PixelPerfect);
    } else {
        setPrecision(DrawPrecision::Normal);
    }
}

void MousePainter::cyclePressure() {
    switch (mouseController_.getPressure()) {
        case PressureLevel::Light: setPressure(PressureLevel::Medium); break;
        case PressureLevel::Medium: setPressure(PressureLevel::Heavy); break;
        case PressureLevel::Heavy: setPressure(PressureLevel::Light); break;
    }
}

PressureLevel MousePainter::getPressure() const {
    return mouseController_.getPressure();
}

void MousePainter::startDrawing() {
    if (running_) {
        return;
    }
    
    if (polylines_.empty()) {
        emit drawingError("没有可绘制的路径");
        return;
    }
    
    if (!mouseController_.isInitialized()) {
        if (!initialize()) {
            emit drawingError("无法初始化鼠标控制器");
            return;
        }
    }
    
    running_ = true;
    paused_ = false;
    state_ = DrawingState::Drawing;
    currentLineIndex_ = 0;
    currentPointIndex_ = 0;
    
    emit drawingStarted();
    
    // 启动绘制线程
    if (drawingThread_.joinable()) {
        drawingThread_.join();
    }
    drawingThread_ = std::thread(&MousePainter::drawingThreadFunc, this);
}

void MousePainter::stopDrawing() {
    running_ = false;
    paused_ = false;
    state_ = DrawingState::Idle;
    
    if (drawingThread_.joinable()) {
        drawingThread_.join();
    }
    
    // 确保鼠标左键已释放
    mouseController_.releaseLeft();
    
    emit drawingStopped();
}

void MousePainter::pauseDrawing() {
    if (state_ == DrawingState::Drawing) {
        paused_ = true;
        state_ = DrawingState::Paused;
    }
}

void MousePainter::resumeDrawing() {
    if (state_ == DrawingState::Paused) {
        paused_ = false;
        state_ = DrawingState::Drawing;
    }
}

double MousePainter::getProgress() const {
    if (polylines_.empty()) return 0.0;
    
    int totalPoints = 0;
    for (const auto& poly : polylines_) {
        totalPoints += static_cast<int>(poly.points.size());
    }
    
    if (totalPoints == 0) return 0.0;
    
    int completedPoints = 0;
    for (int i = 0; i < currentLineIndex_; ++i) {
        completedPoints += static_cast<int>(polylines_[i].points.size());
    }
    completedPoints += currentPointIndex_;
    
    return completedPoints / static_cast<double>(totalPoints);
}

void MousePainter::drawingThreadFunc() {
    // 绘制前倒计时
    if (preDrawDelayMs_ > 0) {
        int delaySeconds = preDrawDelayMs_ / 1000;
        for (int i = delaySeconds; i > 0 && running_; --i) {
            emit countdownTick(i);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        emit countdownTick(0);
    }
    
    for (size_t i = 0; i < polylines_.size() && running_; ++i) {
        currentLineIndex_ = static_cast<int>(i);
        currentPointIndex_ = 0;
        
        emit lineStarted(static_cast<int>(i));
        
        // 根据精度模式选择绘制方法
        switch (precision_) {
            case DrawPrecision::PixelPerfect:
                drawPolylinePixelPerfect(polylines_[i]);
                break;
            case DrawPrecision::Normal:
            default:
                drawPolylineHighPrecision(polylines_[i]);
                break;
        }
        
        emit lineFinished(static_cast<int>(i));
        emit drawingProgress(getProgress());
    }
    
    if (running_) {
        state_ = DrawingState::Completed;
        emit drawingFinished();
    }
    
    running_ = false;
}

void MousePainter::drawPolyline(const Polyline& poly) {
    if (poly.points.size() < 2) return;
    
    // 移动到第一个点
    QPoint firstPoint = mapPointToScreen(poly.points[0].x, poly.points[0].y);
    
    mouseController_.moveTo(firstPoint.x(), firstPoint.y());
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // 按下鼠标左键
    mouseController_.pressLeft();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // 沿路径移动
    for (size_t i = 1; i < poly.points.size() && running_; ++i) {
        waitIfPaused();
        
        if (!running_) break;
        
        currentPointIndex_ = static_cast<int>(i);
        
        QPoint targetPoint = mapPointToScreen(poly.points[i].x, poly.points[i].y);
        QPoint prevPoint = mapPointToScreen(poly.points[i-1].x, poly.points[i-1].y);
        
        int duration = calculateDuration(
            Point2D(prevPoint.x(), prevPoint.y()),
            Point2D(targetPoint.x(), targetPoint.y())
        );
        
        // 使用平滑移动
        mouseController_.smoothMoveTo(targetPoint.x(), targetPoint.y(), duration);
    }
    
    // 释放鼠标左键
    mouseController_.releaseLeft();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

void MousePainter::drawPolylineHighPrecision(const Polyline& poly) {
    if (poly.points.size() < 2) return;
    
    // 高精度模式：将路径拟合为贝塞尔曲线，然后密集采样绘制
    auto bezierCurves = PathSimplifier::fitBezierCurves(poly.points, 1.0);
    
    if (bezierCurves.empty()) {
        // 如果拟合失败，回退到普通绘制
        drawPolyline(poly);
        return;
    }
    
    // 计算路径总长度，用于确定采样密度
    double totalLength = 0;
    for (const auto& curve : bezierCurves) {
        double dx = curve.p3.x - curve.p0.x;
        double dy = curve.p3.y - curve.p0.y;
        totalLength += std::sqrt(dx*dx + dy*dy);
    }
    
    // 每像素采样一次，确保精度
    int samplesPerPixel = 2;  // 每像素2个采样点
    
    // 生成密集采样点
    std::vector<Point2D> sampledPoints;
    sampledPoints.reserve(static_cast<size_t>(totalLength * samplesPerPixel));
    
    for (const auto& curve : bezierCurves) {
        double dx = curve.p3.x - curve.p0.x;
        double dy = curve.p3.y - curve.p0.y;
        double segLength = std::sqrt(dx*dx + dy*dy);
        int numSamples = std::max(10, static_cast<int>(segLength * samplesPerPixel));
        
        auto segPoints = PathSimplifier::sampleBezierCurve(curve, numSamples);
        
        // 避免重复添加端点
        if (!sampledPoints.empty() && !segPoints.empty()) {
            if (std::abs(sampledPoints.back().x - segPoints.front().x) < 0.5 &&
                std::abs(sampledPoints.back().y - segPoints.front().y) < 0.5) {
                segPoints.erase(segPoints.begin());
            }
        }
        
        sampledPoints.insert(sampledPoints.end(), segPoints.begin(), segPoints.end());
    }
    
    if (sampledPoints.size() < 2) {
        drawPolyline(poly);
        return;
    }
    
    // 移动到第一个点
    QPoint firstPoint = mapPointToScreen(sampledPoints[0].x, sampledPoints[0].y);
    mouseController_.moveTo(firstPoint.x(), firstPoint.y());
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // 按下鼠标左键
    mouseController_.pressLeft();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // 沿密集采样点移动
    for (size_t i = 1; i < sampledPoints.size() && running_; ++i) {
        waitIfPaused();
        if (!running_) break;
        
        currentPointIndex_ = static_cast<int>(i);
        
        QPoint targetPoint = mapPointToScreen(sampledPoints[i].x, sampledPoints[i].y);
        
        // 高精度模式：直接移动，不使用平滑插值（保持精确）
        // 但对于长距离，仍然使用平滑移动
        QPoint prevPoint = mapPointToScreen(sampledPoints[i-1].x, sampledPoints[i-1].y);
        double dist = std::sqrt(
            std::pow(targetPoint.x() - prevPoint.x(), 2) +
            std::pow(targetPoint.y() - prevPoint.y(), 2)
        );
        
        if (dist > 5.0) {
            // 距离较远，使用平滑移动
            int duration = calculateDuration(
                Point2D(prevPoint.x(), prevPoint.y()),
                Point2D(targetPoint.x(), targetPoint.y())
            );
            mouseController_.smoothMoveTo(targetPoint.x(), targetPoint.y(), duration);
        } else {
            // 距离近，直接移动（更精确）
            mouseController_.moveTo(targetPoint.x(), targetPoint.y());
            // 短暂延迟确保鼠标到达
            std::this_thread::sleep_for(std::chrono::microseconds(500));
        }
    }
    
    // 释放鼠标左键
    mouseController_.releaseLeft();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

void MousePainter::drawPolylinePixelPerfect(const Polyline& poly) {
    if (poly.points.size() < 2) return;
    
    // 像素级精确模式：将路径拟合为贝塞尔曲线，然后逐像素移动
    auto bezierCurves = PathSimplifier::fitBezierCurves(poly.points, 0.5);
    
    if (bezierCurves.empty()) {
        drawPolyline(poly);
        return;
    }
    
    // 生成密集采样点（每像素4个采样点）
    std::vector<Point2D> sampledPoints;
    
    for (const auto& curve : bezierCurves) {
        double dx = curve.p3.x - curve.p0.x;
        double dy = curve.p3.y - curve.p0.y;
        double segLength = std::sqrt(dx*dx + dy*dy);
        int numSamples = std::max(20, static_cast<int>(segLength * 4));  // 每像素4个采样点
        
        auto segPoints = PathSimplifier::sampleBezierCurve(curve, numSamples);
        
        // 避免重复添加端点
        if (!sampledPoints.empty() && !segPoints.empty()) {
            if (std::abs(sampledPoints.back().x - segPoints.front().x) < 0.5 &&
                std::abs(sampledPoints.back().y - segPoints.front().y) < 0.5) {
                segPoints.erase(segPoints.begin());
            }
        }
        
        sampledPoints.insert(sampledPoints.end(), segPoints.begin(), segPoints.end());
    }
    
    if (sampledPoints.size() < 2) {
        drawPolyline(poly);
        return;
    }
    
    // 移动到第一个点
    QPoint firstPoint = mapPointToScreen(sampledPoints[0].x, sampledPoints[0].y);
    mouseController_.moveTo(firstPoint.x(), firstPoint.y());
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // 按下鼠标左键
    mouseController_.pressLeft();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // 逐像素精确移动
    for (size_t i = 1; i < sampledPoints.size() && running_; ++i) {
        waitIfPaused();
        if (!running_) break;
        
        currentPointIndex_ = static_cast<int>(i);
        
        QPoint targetPoint = mapPointToScreen(sampledPoints[i].x, sampledPoints[i].y);
        QPoint prevPoint = mapPointToScreen(sampledPoints[i-1].x, sampledPoints[i-1].y);
        
        // 使用像素级精确移动
        int duration = calculateDuration(
            Point2D(prevPoint.x(), prevPoint.y()),
            Point2D(targetPoint.x(), targetPoint.y())
        );
        
        // 限制最大步长为1像素
        int dx = targetPoint.x() - prevPoint.x();
        int dy = targetPoint.y() - prevPoint.y();
        double dist = std::sqrt(dx*dx + dy*dy);
        
        if (dist > 1.0) {
            // 分段移动，每段1像素
            int steps = static_cast<int>(dist);
            for (int step = 1; step <= steps && running_; ++step) {
                double ratio = step / static_cast<double>(steps);
                int curX = static_cast<int>(prevPoint.x() + dx * ratio);
                int curY = static_cast<int>(prevPoint.y() + dy * ratio);
                mouseController_.moveTo(curX, curY);
                std::this_thread::sleep_for(std::chrono::microseconds(static_cast<int>(duration * 1000.0 / steps)));
            }
        } else {
            mouseController_.moveTo(targetPoint.x(), targetPoint.y());
            std::this_thread::sleep_for(std::chrono::microseconds(duration * 1000));
        }
    }
    
    // 释放鼠标左键
    mouseController_.releaseLeft();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

int MousePainter::calculateDuration(const Point2D& from, const Point2D& to) {
    double dx = to.x - from.x;
    double dy = to.y - from.y;
    double distance = std::sqrt(dx * dx + dy * dy);
    
    // 根据精度模式调整基础速度
    double baseDuration;
    switch (precision_) {
        case DrawPrecision::PixelPerfect:
            baseDuration = distance * 25.0;  // 像素级模式更慢
            break;
        case DrawPrecision::Normal:
        default:
            baseDuration = distance * 15.0;  // 高精度模式
            break;
    }
    
    double adjustedDuration = baseDuration / speed_;
    
    // 限制最小和最大时间
    return static_cast<int>(std::max(5.0, std::min(adjustedDuration, 5000.0)));
}

void MousePainter::waitIfPaused() {
    while (paused_ && running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

QPoint MousePainter::mapPointToScreen(double x, double y) const {
    if (areaSelector_ && areaSelector_->isValid() && lineArtWidth_ > 0 && lineArtHeight_ > 0) {
        return areaSelector_->mapPoint(x, y, lineArtWidth_, lineArtHeight_, lineArtOriginX_, lineArtOriginY_);
    }
    
    // 使用默认的偏移和缩放（减去原点偏移）
    int screenX = static_cast<int>((x - lineArtOriginX_) * scale_ + offsetX_);
    int screenY = static_cast<int>((y - lineArtOriginY_) * scale_ + offsetY_);
    return QPoint(screenX, screenY);
}

double MousePainter::speedLevelToMultiplier(DrawSpeedLevel level) {
    switch (level) {
        case DrawSpeedLevel::VerySlow: return 0.2;
        case DrawSpeedLevel::Slow: return 0.5;
        case DrawSpeedLevel::Normal: return 1.0;
        case DrawSpeedLevel::Fast: return 2.0;
        case DrawSpeedLevel::VeryFast: return 5.0;
        default: return 1.0;
    }
}

} // namespace SketchMaster
