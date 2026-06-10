#include "HotkeyManager.h"
#include <QString>
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
    return registerHotkeyDynamic(vkCode, modifiers, callback) > 0;
}

int HotkeyManager::registerHotkeyDynamic(int vkCode, unsigned int modifiers, HotkeyCallback callback) {
    std::lock_guard<std::mutex> lock(hotkeysMutex_);
    int hotkeyId = nextHotkeyId_++;

#ifdef _WIN32
    // 转换修饰键为Windows格式
    UINT fsModifiers = 0;
    if (modifiers & MOD_ALT)       fsModifiers |= MOD_ALT;
    if (modifiers & MOD_CONTROL)   fsModifiers |= MOD_CONTROL;
    if (modifiers & MOD_SHIFT)     fsModifiers |= MOD_SHIFT;

    if (RegisterHotKey(nullptr, hotkeyId, fsModifiers, vkCode)) {
        hotkeys_.push_back({vkCode, modifiers, hotkeyId, callback});
        return hotkeyId;
    } else {
        std::cerr << "HotkeyManager: 注册热键失败, 错误码: " << GetLastError() << std::endl;
        return -1;
    }
#else
    hotkeys_.push_back({vkCode, modifiers, hotkeyId, callback});
    return hotkeyId;
#endif
}

bool HotkeyManager::unregisterHotkey(int hotkeyId) {
    std::lock_guard<std::mutex> lock(hotkeysMutex_);
    for (auto it = hotkeys_.begin(); it != hotkeys_.end(); ++it) {
        if (it->atomId == hotkeyId) {
#ifdef _WIN32
            UnregisterHotKey(nullptr, it->atomId);
#endif
            hotkeys_.erase(it);
            return true;
        }
    }
    return false;
}

void HotkeyManager::unregisterAll() {
    std::lock_guard<std::mutex> lock(hotkeysMutex_);
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

QString HotkeyManager::hotkeyName(int vkCode, unsigned int modifiers) {
    QString name;
    if (modifiers & MOD_CONTROL) name += "Ctrl+";
    if (modifiers & MOD_SHIFT)   name += "Shift+";
    if (modifiers & MOD_ALT)     name += "Alt+";

#ifdef _WIN32
    if (vkCode >= VK_F1 && vkCode <= VK_F24) {
        name += "F" + QString::number(vkCode - VK_F1 + 1);
    } else if (vkCode == VK_SPACE) {
        name += "Space";
    } else if (vkCode == VK_ESCAPE) {
        name += "ESC";
    } else if (vkCode == VK_RETURN) {
        name += "Enter";
    } else if (vkCode >= 0x30 && vkCode <= 0x5A) {
        name += QChar(vkCode);
    } else {
        name += "Key(" + QString::number(vkCode) + ")";
    }
#else
    // Linux: 简单显示键码
    if (vkCode == 0xFFBF) name += "F7";
    else if (vkCode == 0xFFC0) name += "F8";
    else if (vkCode == 0xFF08) name += "Backspace";
    else name += "Key(" + QString::number(vkCode) + ")";
#endif
    return name;
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
                std::lock_guard<std::mutex> lock(hotkeysMutex_);
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
