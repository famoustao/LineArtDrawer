#pragma once

#include <QWidget>
#include <QSlider>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QGroupBox>
#include <QCheckBox>
#include <QTabWidget>

namespace SketchMaster {

// 控制面板：参数调节和控制按钮（使用QTabWidget分为4个标签页）
class ControlPanel : public QWidget {
    Q_OBJECT

public:
    explicit ControlPanel(QWidget* parent = nullptr);
    ~ControlPanel();

    // 获取当前参数值
    double getContrast() const;
    int getBlurSize() const;
    double getMergeDistance() const;
    double getSimplifyEpsilon() const;
    double getBackgroundOpacity() const;
    double getDrawingSpeed() const;
    bool isHighPrecisionEnabled() const;
    bool isPixelPerfectEnabled() const;
    int getPressureLevel() const;

    // 设置参数值
    void setContrast(double value);
    void setBlurSize(int value);
    void setMergeDistance(double value);
    void setSimplifyEpsilon(double value);
    void setBackgroundOpacity(double value);
    void setDrawingSpeed(double value);
    void setHighPrecision(bool enabled);
    void setPixelPerfect(bool enabled);
    void setPressureLevel(int level);

    // 图像追踪参数
    int getTraceColors() const;
    int getTraceBlurSize() const;
    double getTraceCurveThreshold() const;
    bool getTraceFillHoles() const;
    int getTraceMinArea() const;
    bool getTraceSmoothPaths() const;

    // 延迟时间
    int getDelaySeconds() const;

    // 热键相关
    int getTopLeftHotkeyId() const { return topLeftHotkeyId_; }
    int getBottomRightHotkeyId() const { return bottomRightHotkeyId_; }
    int getStartDrawHotkeyId() const { return startDrawHotkeyId_; }
    int getStopDrawHotkeyId() const { return stopDrawHotkeyId_; }
    void setTopLeftHotkeyId(int id) { topLeftHotkeyId_ = id; }
    void setBottomRightHotkeyId(int id) { bottomRightHotkeyId_ = id; }
    void setStartDrawHotkeyId(int id) { startDrawHotkeyId_ = id; }
    void setStopDrawHotkeyId(int id) { stopDrawHotkeyId_ = id; }

    // 更新热键显示文本
    void updateHotkeyDisplay(const QString& which, int vkCode, unsigned int modifiers);

    // 进入等待按键模式
    void startWaitingForKey(const QString& which);

signals:
    // 线稿生成相关
    void generateLineArtClicked();
    void generateCannyClicked();
    void importImageClicked();
    void exportSVGClicked();
    void importSVGClicked();
    void traceToSVGClicked();

    // 参数变化
    void contrastChanged(double value);
    void blurSizeChanged(int value);
    void mergeDistanceChanged(double value);
    void simplifyEpsilonChanged(double value);
    void backgroundOpacityChanged(double value);
    void drawingSpeedChanged(double value);
    void highPrecisionChanged(bool enabled);
    void pixelPerfectChanged(bool enabled);
    void pressureLevelChanged(int level);

    // 编辑控制
    void editModeChanged(int mode); // 0=View, 1=Select, 2=Draw, 3=Erase
    void deleteSelectedClicked();
    void clearAllClicked();
    void undoClicked();
    void redoClicked();

    // 鼠标绘制控制
    void startDrawingClicked();
    void stopDrawingClicked();
    void resetZoomClicked();
    void speedUpClicked();
    void speedDownClicked();
    void togglePrecisionClicked();
    void cyclePressureClicked();

    // 热键设置信号
    void setTopLeftKeyClicked();
    void setBottomRightKeyClicked();
    void setStartDrawKeyClicked();
    void setStopDrawKeyClicked();

    // 延迟时间变化
    void delaySecondsChanged(int seconds);

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    void setupUI();
    void connectSignals();

    // QTabWidget
    QTabWidget* tabWidget_;

    // 图像处理参数
    QDoubleSpinBox* contrastSpin_;
    QSpinBox* blurSizeSpin_;
    QDoubleSpinBox* mergeDistanceSpin_;
    QDoubleSpinBox* simplifyEpsilonSpin_;

    // 显示参数
    QSlider* opacitySlider_;
    QLabel* opacityLabel_;

    // 绘制参数
    QDoubleSpinBox* drawingSpeedSpin_;
    QCheckBox* highPrecisionCheck_;
    QCheckBox* pixelPerfectCheck_;
    QComboBox* pressureCombo_;

    // 编辑模式
    QComboBox* editModeCombo_;

    // 按钮
    QPushButton* importImageBtn_;
    QPushButton* generateBtn_;
    QPushButton* generateCannyBtn_;
    QPushButton* exportSVGBtn_;
    QPushButton* importSVGBtn_;
    QPushButton* traceToSVGBtn_;
    QPushButton* deleteBtn_;
    QPushButton* clearBtn_;
    QPushButton* undoBtn_;
    QPushButton* redoBtn_;
    QPushButton* startDrawBtn_;
    QPushButton* stopDrawBtn_;
    QPushButton* resetZoomBtn_;
    QPushButton* speedUpBtn_;
    QPushButton* speedDownBtn_;
    QPushButton* togglePrecisionBtn_;
    QPushButton* cyclePressureBtn_;

    // 图像追踪参数
    QSpinBox* traceColorsSpin_;
    QSpinBox* traceBlurSpin_;
    QDoubleSpinBox* traceThresholdSpin_;
    QCheckBox* traceFillHolesCheck_;
    QSpinBox* traceMinAreaSpin_;
    QCheckBox* traceSmoothPathsCheck_;

    // 延迟时间
    QSpinBox* delaySpin_;

    // 热键设置按钮和显示标签
    QPushButton* setTopLeftKeyBtn_;
    QLabel* topLeftKeyLabel_;
    QPushButton* setBottomRightKeyBtn_;
    QLabel* bottomRightKeyLabel_;
    QPushButton* setStartDrawKeyBtn_;
    QLabel* startDrawKeyLabel_;
    QPushButton* setStopDrawKeyBtn_;
    QLabel* stopDrawKeyLabel_;

    // 热键ID（用于动态注销）
    int topLeftHotkeyId_;
    int bottomRightHotkeyId_;
    int startDrawHotkeyId_;
    int stopDrawHotkeyId_;

    // 等待按键模式
    QString waitingForKeyWhich_;
};

} // namespace SketchMaster
