#include "HotkeyManager.h"
#include <iostream>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace SketchMaster {

HotkeyManager::HotkeyManager(QObject* parent)
    : QThread(parent), running_(false), nextHotkeyId_(1) {}

HotkeyManager::~HotkeyManager() {
    stop();
    wait();
    unregisterAll();
}

bool HotkeyManager::registerHotkey(int vkCode, unsigned int modifiers, HotkeyCallback callback) {
#ifdef _WIN32
    int hotkeyId = nextHotkeyId_++;
    
    // 转换修饰键为Windows格式
    UINT fsModifiers = 0;
    if (modifiers & MOD_ALT)       fsModifiers |= MOD_ALT;
    if (modifiers & MOD_CONTROL)   fsModifiers |= MOD_CONTROL;
    if (modifiers & MOD_SHIFT)     fsModifiers |= MOD_SHIFT;
    
    if (RegisterHotKey(nullptr, hotkeyId, fsModifiers, vkCode)) {
        hotkeys_.push_back({vkCode, modifiers, hotkeyId, callback});
        return true;
    } else {
        std::cerr << "HotkeyManager: 注册热键失败, 错误码: " << GetLastError() << std::endl;
        return false;
    }
#else
    hotkeys_.push_back({vkCode, modifiers, nextHotkeyId_++, callback});
    return true;
#endif
}

void HotkeyManager::unregisterAll() {
#ifdef _WIN32
    for (const auto& hk : hotkeys_) {
        UnregisterHotKey(nullptr, hk.atomId);
    }
#endif
    hotkeys_.clear();
}

void HotkeyManager::stop() {
    running_ = false;
}

void HotkeyManager::run() {
#ifdef _WIN32
    running_ = true;
    MSG msg;

    while (running_) {
        // PeekMessage 不阻塞，允许及时检查 running_ 标志
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_HOTKEY) {
                int hotkeyId = static_cast<int>(msg.wParam);
                for (size_t i = 0; i < hotkeys_.size(); ++i) {
                    if (hotkeys_[i].atomId == hotkeyId) {
                        emit hotkeyTriggered(static_cast<int>(i));
                        if (hotkeys_[i].callback) {
                            hotkeys_[i].callback();
                        }
                        break;
                    }
                }
            }
        }
        
        // 短暂休眠避免CPU占用过高
        QThread::msleep(5);
    }
#else
    running_ = true;
    while (running_) {
        QThread::msleep(100);
    }
#endif
}

} // namespace SketchMaster
