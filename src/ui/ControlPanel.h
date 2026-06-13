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

    // 更新切割模式按钮状态
    void setCutModeActive(bool active);

    // 更新手绘模式按钮状态
    void setDrawModeActive(bool active);

    // 获取当前选择的算法索引
    int getSelectedAlgorithm() const;

    // 内置算法数量（用于区分内置和外部算法）
    // 0-8: Canny, DoG, Sobel, Scharr, Laplacian, LSD, 形态学, HED, MLSD
    static const int BUILTIN_ALGORITHM_COUNT = 9;

signals:
    // 线稿生成相关
    void generateLineArtClicked();
    void generateCannyClicked();
    void generateWithAlgorithmClicked();
    void algorithmChanged(int index);
    void importImageClicked();
    void exportSVGClicked();
    void importSVGClicked();

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
    void deleteSelectedClicked();
    void clearAllClicked();
    void undoClicked();
    void redoClicked();

    // 新增：切割/合并/手绘模式信号
    void cutModeToggled(bool active);
    void smartMergeClicked();
    void drawModeToggled(bool active);

    // 鼠标绘制控制
    void startDrawingClicked();
    void stopDrawingClicked();
    void resetZoomClicked();
    void zoomSliderChanged(int value);
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

    // 按钮
    QPushButton* importImageBtn_;
    QPushButton* generateBtn_;
    QPushButton* generateCannyBtn_;
    QPushButton* generateWithAlgoBtn_;
    QPushButton* exportSVGBtn_;
    QPushButton* importSVGBtn_;
    QPushButton* deleteBtn_;
    QPushButton* clearBtn_;
    QPushButton* undoBtn_;
    QPushButton* redoBtn_;
    QPushButton* startDrawBtn_;
    QPushButton* stopDrawBtn_;
    QPushButton* resetZoomBtn_;
    QSlider* zoomSlider_;
    QLabel* zoomPercentLabel_;
    QPushButton* speedUpBtn_;
    QPushButton* speedDownBtn_;
    QPushButton* togglePrecisionBtn_;
    QPushButton* cyclePressureBtn_;

    // 新增按钮：切割、智能合并、手绘模式
    QPushButton* cutBtn_;
    QPushButton* smartMergeBtn_;
    QPushButton* drawModeBtn_;

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

    // 算法选择
    QComboBox* algorithmCombo_;
    QLabel* algorithmHintLabel_;
};

} // namespace SketchMaster
