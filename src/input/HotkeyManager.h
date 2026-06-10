#pragma once

#include <QObject>
#include <QThread>
#include <atomic>
#include <functional>

#ifdef _WIN32
#include <windows.h>
#endif

namespace SketchMaster {

// 热键回调类型
using HotkeyCallback = std::function<void()>;

// 全局热键管理器（Windows RegisterHotKey 实现）
class HotkeyManager : public QThread {
    Q_OBJECT

public:
    explicit HotkeyManager(QObject* parent = nullptr);
    ~HotkeyManager();

    // 注册热键（Windows虚拟键码 + 修饰键）
    bool registerHotkey(int vkCode, unsigned int modifiers, HotkeyCallback callback);
    
    // 注销所有热键
    void unregisterAll();
    
    // 停止监听
    void stop();

signals:
    void hotkeyTriggered(int id);

protected:
    void run() override;

private:
    std::atomic<bool> running_;
    struct HotkeyInfo {
        int vkCode;
        unsigned int modifiers;
        int atomId;          // Windows热键ID
        HotkeyCallback callback;
    };
    std::vector<HotkeyInfo> hotkeys_;
    int nextHotkeyId_;
};

} // namespace SketchMaster
