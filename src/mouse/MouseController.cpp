#include "MouseController.h"
#include <cmath>
#include <chrono>
#include <thread>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace SketchMaster {

MouseController::MouseController()
    : initialized_(false)
    , currentPressure_(PressureLevel::Medium)
    , lineThickness_(1.0) {}

MouseController::~MouseController() {
    cleanup();
}

bool MouseController::initialize() {
    if (initialized_) return true;
    initialized_ = true;
    return true;
}

void MouseController::cleanup() {
    initialized_ = false;
}

bool MouseController::moveTo(int x, int y) {
#ifdef _WIN32
    // 使用SetCursorPos设置绝对位置
    return SetCursorPos(x, y) != 0;
#else
    return false;
#endif
}

bool MouseController::moveRelative(int dx, int dy) {
#ifdef _WIN32
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    input.mi.dx = dx;
    input.mi.dy = dy;
    input.mi.dwFlags = MOUSEEVENTF_MOVE;
    SendInput(1, &input, sizeof(INPUT));
    return true;
#else
    return false;
#endif
}

bool MouseController::pressLeft() {
#ifdef _WIN32
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    SendInput(1, &input, sizeof(INPUT));
    return true;
#else
    return false;
#endif
}

bool MouseController::releaseLeft() {
#ifdef _WIN32
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(1, &input, sizeof(INPUT));
    return true;
#else
    return false;
#endif
}

bool MouseController::clickLeft() {
    pressLeft();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    releaseLeft();
    return true;
}

bool MouseController::getCurrentPosition(int& x, int& y) {
#ifdef _WIN32
    POINT pt;
    if (GetCursorPos(&pt)) {
        x = pt.x;
        y = pt.y;
        return true;
    }
    return false;
#else
    return false;
#endif
}

bool MouseController::smoothMoveTo(int x, int y, int durationMs) {
    int startX, startY;
    if (!getCurrentPosition(startX, startY)) {
        return false;
    }
    
    // 使用三次贝塞尔曲线进行平滑移动
    int steps = durationMs / 10;
    if (steps < 10) steps = 10;
    double stepTime = durationMs / static_cast<double>(steps);
    
    // 控制点（创建轻微的曲线效果，模拟人类手部运动）
    srand(static_cast<unsigned>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
    double cp1x = startX + (x - startX) * 0.3 + (rand() % 20 - 10);
    double cp1y = startY + (y - startY) * 0.3 + (rand() % 20 - 10);
    double cp2x = startX + (x - startX) * 0.7 + (rand() % 20 - 10);
    double cp2y = startY + (y - startY) * 0.7 + (rand() % 20 - 10);
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i <= steps; ++i) {
        double t = i / static_cast<double>(steps);
        
        // 三次贝塞尔曲线
        double mt = 1 - t;
        double mt2 = mt * mt;
        double mt3 = mt2 * mt;
        double t2 = t * t;
        double t3 = t2 * t;
        
        int curX = static_cast<int>(mt3 * startX + 3 * mt2 * t * cp1x +
                                    3 * mt * t2 * cp2x + t3 * x);
        int curY = static_cast<int>(mt3 * startY + 3 * mt2 * t * cp1y +
                                    3 * mt * t2 * cp2y + t3 * y);
        
        moveTo(curX, curY);
        
        // 精确时间控制
        auto expectedTime = startTime + std::chrono::milliseconds(
            static_cast<int>(i * stepTime));
        auto now = std::chrono::high_resolution_clock::now();
        if (expectedTime > now) {
            std::this_thread::sleep_for(expectedTime - now);
        }
    }
    
    // 确保到达目标位置
    moveTo(x, y);
    
    return true;
}

bool MouseController::pixelPreciseMoveTo(int x, int y, int durationMs) {
    int startX, startY;
    if (!getCurrentPosition(startX, startY)) {
        return false;
    }
    
    // 计算距离
    int dx = x - startX;
    int dy = y - startY;
    int distance = static_cast<int>(std::sqrt(dx * dx + dy * dy));
    
    if (distance == 0) return true;
    
    // 逐像素移动
    const int steps = distance;  // 每步移动1像素
    const double stepTime = durationMs / static_cast<double>(steps);
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    for (int i = 1; i <= steps; ++i) {
        double ratio = i / static_cast<double>(steps);
        int curX = static_cast<int>(startX + dx * ratio);
        int curY = static_cast<int>(startY + dy * ratio);
        
        moveTo(curX, curY);
        
        // 精确时间控制
        auto expectedTime = startTime + std::chrono::microseconds(
            static_cast<int>(i * stepTime * 1000));
        auto now = std::chrono::high_resolution_clock::now();
        if (expectedTime > now) {
            std::this_thread::sleep_for(expectedTime - now);
        }
    }
    
    // 确保到达目标位置
    moveTo(x, y);
    
    return true;
}

void MouseController::setPressure(PressureLevel pressure) {
    currentPressure_ = pressure;
    // 根据压感调整线条粗细
    switch (pressure) {
        case PressureLevel::Light:
            lineThickness_ = 0.5;
            break;
        case PressureLevel::Medium:
            lineThickness_ = 1.0;
            break;
        case PressureLevel::Heavy:
            lineThickness_ = 2.0;
            break;
    }
}

} // namespace SketchMaster
