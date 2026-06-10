#pragma once

#include <QMainWindow>
#include <QSplitter>
#include <QFileDialog>
#include <QMessageBox>
#include <QStatusBar>
#include <QProgressBar>
#include <QLabel>
#include <QSystemTrayIcon>
#include "SketchCanvas.h"
#include "ControlPanel.h"
#include "../core/LineArtGenerator.h"
#include "../core/SVGExporter.h"
#include "../core/SVGImporter.h"
#include "../core/ImageTracer.h"
#include "../mouse/MousePainter.h"
#include "../mouse/DrawingAreaSelector.h"
#include "../input/HotkeyManager.h"

namespace SketchMaster {

// 主窗口
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    // 文件操作
    void onImportImage();
    void onImportSVG();
    void onExportSVG();
    void onTraceToSVG();
    
    // 线稿生成
    void onGenerateLineArt();
    void onGenerateCanny();
    
    // 参数变化
    void onContrastChanged(double value);
    void onBlurSizeChanged(int value);
    void onMergeDistanceChanged(double value);
    void onSimplifyEpsilonChanged(double value);
    void onBackgroundOpacityChanged(double value);
    void onDrawingSpeedChanged(double value);
    void onHighPrecisionChanged(bool enabled);
    void onPixelPerfectChanged(bool enabled);
    void onPressureLevelChanged(int level);
    
    // 速度/精度/压感快捷控制
    void onSpeedUp();
    void onSpeedDown();
    void onTogglePrecision();
    void onCyclePressure();
    
    // 编辑控制
    void onEditModeChanged(int mode);
    void onDeleteSelected();
    void onClearAll();
    void onResetZoom();
    
    // 鼠标绘制
    void onStartDrawing();
    void onStopDrawing();
    
    // 绘制状态
    void onDrawingStarted();
    void onDrawingFinished();
    void onDrawingStopped();
    void onDrawingProgress(double progress);
    void onDrawingError(const QString& error);
    void onLineStarted(int lineIndex);
    void onLineFinished(int lineIndex);
    void onCountdownTick(int secondsLeft);
    
    // 热键回调
    void onF7Pressed();
    void onF8Pressed();

private:
    void setupUI();
    void connectSignals();
    void updateStatusBar(const QString& message);
    void setupHotkeys();
    void setupTrayIcon();
    void setupIcon();
    
    // 核心组件
    LineArtGenerator lineArtGenerator_;
    SVGExporter svgExporter_;
    SVGImporter svgImporter_;
    ImageTracer imageTracer_;
    MousePainter mousePainter_;
    DrawingAreaSelector areaSelector_;
    HotkeyManager* hotkeyManager_;
    
    // UI组件
    QSplitter* splitter_;
    SketchCanvas* canvas_;
    ControlPanel* controlPanel_;
    QProgressBar* progressBar_;
    QLabel* countdownLabel_;
    QSystemTrayIcon* trayIcon_;
    
    // 当前状态
    bool hasImage_;
    bool hasLineArt_;
    QString currentImagePath_;
};

} // namespace SketchMaster
