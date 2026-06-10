#pragma once

#include "../core/PathSimplifier.h"
#include <thread>
#include <atomic>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace SketchMaster {

// 压感级别
enum class PressureLevel {
    Light = 1,    // 轻压 - 细线
    Medium = 2,   // 中压 - 正常
    Heavy = 3     // 重压 - 粗线
};

// 鼠标控制器：控制鼠标移动和点击（Windows API实现）
class MouseController {
public:
    MouseController();
    ~MouseController();

    // 初始化
    bool initialize();
    
    // 释放资源
    void cleanup();
    
    // 移动鼠标到指定位置（绝对坐标，屏幕像素）
    bool moveTo(int x, int y);
    
    // 相对移动
    bool moveRelative(int dx, int dy);
    
    // 按下鼠标左键
    bool pressLeft();
    
    // 释放鼠标左键
    bool releaseLeft();
    
    // 点击鼠标左键
    bool clickLeft();
    
    // 获取当前鼠标位置
    bool getCurrentPosition(int& x, int& y);
    
    // 平滑移动鼠标（贝塞尔曲线插值）
    bool smoothMoveTo(int x, int y, int durationMs);
    
    // 像素级精确移动（逐像素移动）
    bool pixelPreciseMoveTo(int x, int y, int durationMs);
    
    // 设置压感级别（影响线条粗细）
    void setPressure(PressureLevel pressure);
    
    // 获取当前压感
    PressureLevel getPressure() const { return currentPressure_; }
    
    // 设置线条粗细（像素）
    void setLineThickness(double thickness) { lineThickness_ = thickness; }
    
    // 获取线条粗细
    double getLineThickness() const { return lineThickness_; }
    
    // 是否已初始化
    bool isInitialized() const { return initialized_; }

private:
    std::atomic<bool> initialized_;
    std::atomic<PressureLevel> currentPressure_;
    double lineThickness_;
};

} // namespace SketchMaster
