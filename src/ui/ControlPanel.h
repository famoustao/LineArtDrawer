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

namespace SketchMaster {

// 控制面板：参数调节和控制按钮
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
    
    // 鼠标绘制控制
    void startDrawingClicked();
    void stopDrawingClicked();
    void resetZoomClicked();
    void speedUpClicked();
    void speedDownClicked();
    void togglePrecisionClicked();
    void cyclePressureClicked();

private:
    void setupUI();
    void connectSignals();

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
};

} // namespace SketchMaster
