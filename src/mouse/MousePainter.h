#pragma once

#include "MouseController.h"
#include "DrawingAreaSelector.h"
#include <QObject>
#include <thread>
#include <atomic>
#include <vector>
#include <functional>

namespace SketchMaster {

// 绘制精度模式
enum class DrawPrecision {
    Normal,      // 普通模式 - 平滑移动
    PixelPerfect // 像素级精确模式 - 逐像素移动
};

// 绘制速度级别
enum class DrawSpeedLevel {
    VerySlow = 1,  // 0.2x
    Slow = 2,      // 0.5x
    Normal = 3,    // 1.0x
    Fast = 4,      // 2.0x
    VeryFast = 5   // 5.0x
};

// 鼠标绘制状态
enum class DrawingState {
    Idle,       // 空闲
    Drawing,    // 绘制中
    Paused,     // 暂停
    Completed   // 完成
};

// 鼠标自动绘制器：按路径控制鼠标绘制
class MousePainter : public QObject {
    Q_OBJECT

public:
    explicit MousePainter(QObject* parent = nullptr);
    ~MousePainter();

    // 初始化鼠标控制器
    bool initialize();
    
    // 设置绘制路径
    void setPolylines(const std::vector<Polyline>& polylines);
    
    // 设置绘制速度倍率（1.0为正常速度）
    void setSpeed(double speed);
    
    // 设置绘制速度级别
    void setSpeedLevel(DrawSpeedLevel level);
    
    // 设置绘制偏移（将线稿坐标映射到屏幕坐标）
    void setOffset(int offsetX, int offsetY);
    
    // 设置绘制缩放
    void setScale(double scale);
    
    // 设置绘制区域选择器
    void setDrawingAreaSelector(DrawingAreaSelector* selector);
    
    // 设置线稿尺寸（用于区域映射）
    void setLineArtSize(int width, int height);
    
    // 设置线稿坐标原点偏移（用于正确映射）
    void setLineArtOrigin(double originX, double originY);
    
    // 设置绘制前延迟（毫秒）
    void setPreDrawDelay(int delayMs);
    
    // 设置绘制精度模式
    void setPrecision(DrawPrecision precision);
    
    // 设置压感级别
    void setPressure(PressureLevel pressure);
    
    // 便捷方法：设置高精度模式（贝塞尔曲线）
    void setHighPrecision(bool enabled) {
        setPrecision(enabled ? DrawPrecision::Normal : DrawPrecision::Normal);
        // 高精度模式使用Normal精度但启用贝塞尔拟合，在drawPolylineHighPrecision中处理
    }
    
    // 便捷方法：设置像素级精确模式
    void setPixelPerfect(bool enabled) {
        setPrecision(enabled ? DrawPrecision::PixelPerfect : DrawPrecision::Normal);
    }
    
    // 便捷方法：设置压感级别（整数）
    void setPressureLevel(int level) {
        switch (level) {
            case 1: setPressure(PressureLevel::Light); break;
            case 2: setPressure(PressureLevel::Medium); break;
            case 3: setPressure(PressureLevel::Heavy); break;
            default: setPressure(PressureLevel::Medium); break;
        }
    }
    
    // 开始绘制
    void startDrawing();
    
    // 停止绘制
    void stopDrawing();
    
    // 暂停/继续
    void pauseDrawing();
    void resumeDrawing();
    
    // 加速/减速
    void speedUp();
    void speedDown();
    
    // 切换精度模式
    void togglePrecision();
    
    // 切换压感
    void cyclePressure();
    
    // 获取当前状态
    DrawingState getState() const { return state_; }
    
    // 获取当前绘制进度（0.0 - 1.0）
    double getProgress() const;
    
    // 获取当前速度级别
    DrawSpeedLevel getSpeedLevel() const { return speedLevel_; }
    
    // 获取当前精度模式
    DrawPrecision getPrecision() const { return precision_; }
    
    // 获取当前压感
    PressureLevel getPressure() const;
    
    // 是否正在运行
    bool isRunning() const { return running_; }

signals:
    void drawingStarted();
    void drawingFinished();
    void drawingStopped();
    void drawingProgress(double progress);
    void drawingError(const QString& error);
    void lineStarted(int lineIndex);
    void lineFinished(int lineIndex);
    void countdownTick(int secondsLeft);
    void speedChanged(double speed);
    void precisionChanged(bool pixelPerfect);
    void pressureChanged(int pressureLevel);

private:
    MouseController mouseController_;
    std::vector<Polyline> polylines_;
    double speed_;
    DrawSpeedLevel speedLevel_;
    int offsetX_;
    int offsetY_;
    double scale_;
    
    DrawingAreaSelector* areaSelector_;
    int lineArtWidth_;
    int lineArtHeight_;
    double lineArtOriginX_;
    double lineArtOriginY_;
    int preDrawDelayMs_;
    DrawPrecision precision_;
    
    std::atomic<DrawingState> state_;
    std::atomic<bool> running_;
    std::atomic<bool> paused_;
    std::atomic<int> currentLineIndex_;
    std::atomic<int> currentPointIndex_;
    
    std::thread drawingThread_;
    
    // 绘制线程函数
    void drawingThreadFunc();
    
    // 绘制单条路径（标准模式）
    void drawPolyline(const Polyline& poly);
    
    // 绘制单条路径（高精度模式，贝塞尔曲线插值）
    void drawPolylineHighPrecision(const Polyline& poly);
    
    // 绘制单条路径（像素级精确模式）
    void drawPolylinePixelPerfect(const Polyline& poly);
    
    // 计算两点间的绘制时间（毫秒）
    int calculateDuration(const Point2D& from, const Point2D& to);
    
    // 等待暂停恢复
    void waitIfPaused();
    
    // 坐标映射
    QPoint mapPointToScreen(double x, double y) const;
    
    // 根据速度级别计算实际速度
    double speedLevelToMultiplier(DrawSpeedLevel level);
};

} // namespace SketchMaster
