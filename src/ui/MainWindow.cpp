#include "MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>
#include <QScreen>
#include <QIcon>
#include <QPixmap>
#include <QDir>
#include <QLabel>
#include <QShortcut>
#include <QKeySequence>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace SketchMaster {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , hotkeyManager_(nullptr)
    , undoShortcut_(nullptr)
    , hasImage_(false)
    , hasLineArt_(false)
    , topLeftVkCode_(VK_F7)
    , topLeftModifiers_(0)
    , bottomRightVkCode_(VK_F8)
    , bottomRightModifiers_(0)
    , startDrawVkCode_(VK_SPACE)
    , startDrawModifiers_(0)
    , stopDrawVkCode_(VK_ESCAPE)
    , stopDrawModifiers_(0) {
    setupUI();
    connectSignals();
    setupHotkeys();
    setupTrayIcon();
    setupIcon();

    setWindowTitle("图片矢量化自动绘图工具");
    resize(1200, 800);
}

MainWindow::~MainWindow() {
    mousePainter_.stopDrawing();
    if (hotkeyManager_) {
        hotkeyManager_->stop();
        hotkeyManager_->wait();
        delete hotkeyManager_;
    }
}

void MainWindow::setupUI() {
    // 创建中央部件
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // 创建分割器
    splitter_ = new QSplitter(Qt::Horizontal, this);

    // 创建画布
    canvas_ = new SketchCanvas(this);
    splitter_->addWidget(canvas_);

    // 创建控制面板（直接放入splitter，不再使用QScrollArea，因为QTabWidget已解决空间问题）
    controlPanel_ = new ControlPanel(this);
    controlPanel_->setMaximumWidth(340);
    controlPanel_->setMinimumWidth(280);
    splitter_->addWidget(controlPanel_);

    // 设置分割比例
    splitter_->setSizes(QList<int>() << 860 << 340);

    mainLayout->addWidget(splitter_);

    // 创建状态栏
    QStatusBar* statusBar = new QStatusBar(this);
    setStatusBar(statusBar);

    progressBar_ = new QProgressBar(this);
    progressBar_->setRange(0, 100);
    progressBar_->setValue(0);
    progressBar_->setVisible(false);
    statusBar->addPermanentWidget(progressBar_);

    countdownLabel_ = new QLabel(this);
    countdownLabel_->setVisible(false);
    statusBar->addWidget(countdownLabel_);

    // Ctrl+Z 撤销快捷键
    undoShortcut_ = new QShortcut(QKeySequence("Ctrl+Z"), this);
    connect(undoShortcut_, &QShortcut::activated, this, &MainWindow::onUndo);

    updateStatusBar("就绪 - 请导入图片开始。按F7设置左上角，F8设置右下角");
}

void MainWindow::connectSignals() {
    // 控制面板信号
    connect(controlPanel_, &ControlPanel::importImageClicked,
            this, &MainWindow::onImportImage);
    connect(controlPanel_, &ControlPanel::importSVGClicked,
            this, &MainWindow::onImportSVG);
    connect(controlPanel_, &ControlPanel::exportSVGClicked,
            this, &MainWindow::onExportSVG);
    connect(controlPanel_, &ControlPanel::traceToSVGClicked,
            this, &MainWindow::onTraceToSVG);
    connect(controlPanel_, &ControlPanel::generateLineArtClicked,
            this, &MainWindow::onGenerateLineArt);
    connect(controlPanel_, &ControlPanel::generateCannyClicked,
            this, &MainWindow::onGenerateCanny);
    connect(controlPanel_, &ControlPanel::deleteSelectedClicked,
            this, &MainWindow::onDeleteSelected);
    connect(controlPanel_, &ControlPanel::clearAllClicked,
            this, &MainWindow::onClearAll);
    connect(controlPanel_, &ControlPanel::undoClicked,
            this, &MainWindow::onUndo);
    connect(controlPanel_, &ControlPanel::resetZoomClicked,
            this, &MainWindow::onResetZoom);
    connect(controlPanel_, &ControlPanel::startDrawingClicked,
            this, &MainWindow::onStartDrawing);
    connect(controlPanel_, &ControlPanel::stopDrawingClicked,
            this, &MainWindow::onStopDrawing);

    // 参数变化信号
    connect(controlPanel_, &ControlPanel::contrastChanged,
            this, &MainWindow::onContrastChanged);
    connect(controlPanel_, &ControlPanel::blurSizeChanged,
            this, &MainWindow::onBlurSizeChanged);
    connect(controlPanel_, &ControlPanel::mergeDistanceChanged,
            this, &MainWindow::onMergeDistanceChanged);
    connect(controlPanel_, &ControlPanel::simplifyEpsilonChanged,
            this, &MainWindow::onSimplifyEpsilonChanged);
    connect(controlPanel_, &ControlPanel::backgroundOpacityChanged,
            this, &MainWindow::onBackgroundOpacityChanged);
    connect(controlPanel_, &ControlPanel::drawingSpeedChanged,
            this, &MainWindow::onDrawingSpeedChanged);
    connect(controlPanel_, &ControlPanel::editModeChanged,
            this, &MainWindow::onEditModeChanged);

    // 高精度/像素级/压感控制
    connect(controlPanel_, &ControlPanel::highPrecisionChanged,
            this, &MainWindow::onHighPrecisionChanged);
    connect(controlPanel_, &ControlPanel::pixelPerfectChanged,
            this, &MainWindow::onPixelPerfectChanged);
    connect(controlPanel_, &ControlPanel::pressureLevelChanged,
            this, &MainWindow::onPressureLevelChanged);

    // 速度控制按钮
    connect(controlPanel_, &ControlPanel::speedUpClicked,
            this, &MainWindow::onSpeedUp);
    connect(controlPanel_, &ControlPanel::speedDownClicked,
            this, &MainWindow::onSpeedDown);
    connect(controlPanel_, &ControlPanel::togglePrecisionClicked,
            this, &MainWindow::onTogglePrecision);
    connect(controlPanel_, &ControlPanel::cyclePressureClicked,
            this, &MainWindow::onCyclePressure);

    // 热键设置信号
    connect(controlPanel_, &ControlPanel::setTopLeftKeyClicked,
            this, &MainWindow::onSetTopLeftKeyClicked);
    connect(controlPanel_, &ControlPanel::setBottomRightKeyClicked,
            this, &MainWindow::onSetBottomRightKeyClicked);
    connect(controlPanel_, &ControlPanel::setStartDrawKeyClicked,
            this, &MainWindow::onSetStartDrawKeyClicked);
    connect(controlPanel_, &ControlPanel::setStopDrawKeyClicked,
            this, &MainWindow::onSetStopDrawKeyClicked);

    // 延迟时间变化信号
    connect(controlPanel_, &ControlPanel::delaySecondsChanged,
            this, &MainWindow::onDelaySecondsChanged);

    // 鼠标绘制信号
    connect(&mousePainter_, &MousePainter::drawingStarted,
            this, &MainWindow::onDrawingStarted);
    connect(&mousePainter_, &MousePainter::drawingFinished,
            this, &MainWindow::onDrawingFinished);
    connect(&mousePainter_, &MousePainter::drawingStopped,
            this, &MainWindow::onDrawingStopped);
    connect(&mousePainter_, &MousePainter::drawingProgress,
            this, &MainWindow::onDrawingProgress);
    connect(&mousePainter_, &MousePainter::drawingError,
            this, &MainWindow::onDrawingError);
    connect(&mousePainter_, &MousePainter::lineStarted,
            this, &MainWindow::onLineStarted);
    connect(&mousePainter_, &MousePainter::lineFinished,
            this, &MainWindow::onLineFinished);
    connect(&mousePainter_, &MousePainter::countdownTick,
            this, &MainWindow::onCountdownTick);

    // 区域选择信号
    connect(&areaSelector_, &DrawingAreaSelector::topLeftSet,
            [this](const QPoint& pt) {
                updateStatusBar(QString("已设置左上角: (%1, %2)").arg(pt.x()).arg(pt.y()));
            });
    connect(&areaSelector_, &DrawingAreaSelector::bottomRightSet,
            [this](const QPoint& pt) {
                updateStatusBar(QString("已设置右下角: (%1, %2)").arg(pt.x()).arg(pt.y()));
            });
}

void MainWindow::setupHotkeys() {
    hotkeyManager_ = new HotkeyManager(this);

#ifdef _WIN32
    // F7: 设置左上角
    int topLeftId = hotkeyManager_->registerHotkeyDynamic(topLeftVkCode_, topLeftModifiers_, [this]() {
        QMetaObject::invokeMethod(this, &MainWindow::onF7Pressed, Qt::QueuedConnection);
    });
    controlPanel_->setTopLeftHotkeyId(topLeftId);

    // F8: 设置右下角
    int bottomRightId = hotkeyManager_->registerHotkeyDynamic(bottomRightVkCode_, bottomRightModifiers_, [this]() {
        QMetaObject::invokeMethod(this, &MainWindow::onF8Pressed, Qt::QueuedConnection);
    });
    controlPanel_->setBottomRightHotkeyId(bottomRightId);

    // 空格键: 开始绘制
    int startDrawId = hotkeyManager_->registerHotkeyDynamic(startDrawVkCode_, startDrawModifiers_, [this]() {
        QMetaObject::invokeMethod(this, &MainWindow::onStartDrawing, Qt::QueuedConnection);
    });
    controlPanel_->setStartDrawHotkeyId(startDrawId);

    // ESC: 停止绘制
    int stopDrawId = hotkeyManager_->registerHotkeyDynamic(stopDrawVkCode_, stopDrawModifiers_, [this]() {
        QMetaObject::invokeMethod(this, &MainWindow::onStopDrawing, Qt::QueuedConnection);
    });
    controlPanel_->setStopDrawHotkeyId(stopDrawId);

    // 加速 [+]
    hotkeyManager_->registerHotkey(VK_OEM_PLUS, MOD_CONTROL, [this]() {
        QMetaObject::invokeMethod(this, &MainWindow::onSpeedUp, Qt::QueuedConnection);
    });
    // 减速 [-]
    hotkeyManager_->registerHotkey(VK_OEM_MINUS, MOD_CONTROL, [this]() {
        QMetaObject::invokeMethod(this, &MainWindow::onSpeedDown, Qt::QueuedConnection);
    });
    // 切换精度 [P]
    hotkeyManager_->registerHotkey('P', MOD_CONTROL, [this]() {
        QMetaObject::invokeMethod(this, &MainWindow::onTogglePrecision, Qt::QueuedConnection);
    });
    // 切换压感 [C]
    hotkeyManager_->registerHotkey('C', MOD_CONTROL, [this]() {
        QMetaObject::invokeMethod(this, &MainWindow::onCyclePressure, Qt::QueuedConnection);
    });
#else
    // Linux: 使用X11键码
    hotkeyManager_->registerHotkey(0xFFBF, 0, [this]() { // XK_F7
        QMetaObject::invokeMethod(this, &MainWindow::onF7Pressed, Qt::QueuedConnection);
    });

    hotkeyManager_->registerHotkey(0xFFC0, 0, [this]() { // XK_F8
        QMetaObject::invokeMethod(this, &MainWindow::onF8Pressed, Qt::QueuedConnection);
    });
#endif

    hotkeyManager_->start();
}

void MainWindow::setupTrayIcon() {
    // 尝试多个可能的图标路径
    QStringList iconPaths;
    iconPaths << "resources/icon.jpg"
              << "resources/icon.png"
              << "../resources/icon.jpg"
              << QApplication::applicationDirPath() + "/resources/icon.jpg";

    QIcon icon;
    for (const auto& path : iconPaths) {
        if (QFile::exists(path)) {
            icon = QIcon(path);
            break;
        }
    }
    if (icon.isNull()) {
        icon = QApplication::style()->standardIcon(QStyle::SP_ComputerIcon);
    }

    trayIcon_ = new QSystemTrayIcon(icon, this);
    trayIcon_->setToolTip("SketchMaster - 图片转线稿与自动绘制工具");
    trayIcon_->show();
}

void MainWindow::setupIcon() {
    // 不设置自定义图标，使用系统默认
}

void MainWindow::updateStatusBar(const QString& message) {
    statusBar()->showMessage(message, 5000);
}

// === 热键回调 ===

void MainWindow::onF7Pressed() {
    // 获取当前鼠标位置作为左上角
    mousePainter_.initialize();

    int x = 0, y = 0;
    // 通过MouseController获取当前鼠标位置
    MouseController mc;
    mc.initialize();
    if (mc.getCurrentPosition(x, y)) {
        areaSelector_.setTopLeft(QPoint(x, y));
        updateStatusBar(QString("已设置左上角: (%1, %2)").arg(x).arg(y));
    } else {
        updateStatusBar("无法获取鼠标位置");
    }
}

void MainWindow::onF8Pressed() {
    int x = 0, y = 0;
    MouseController mc;
    mc.initialize();
    if (mc.getCurrentPosition(x, y)) {
        areaSelector_.setBottomRight(QPoint(x, y));
        updateStatusBar(QString("已设置右下角: (%1, %2)，绘制区域已确定").arg(x).arg(y));
    } else {
        updateStatusBar("无法获取鼠标位置");
    }
}

// === 热键设置回调 ===

void MainWindow::onSetTopLeftKeyClicked() {
    controlPanel_->startWaitingForKey("topleft");
    updateStatusBar("请按下新的左上角快捷键...");
}

void MainWindow::onSetBottomRightKeyClicked() {
    controlPanel_->startWaitingForKey("bottomright");
    updateStatusBar("请按下新的右下角快捷键...");
}

void MainWindow::onSetStartDrawKeyClicked() {
    controlPanel_->startWaitingForKey("startdraw");
    updateStatusBar("请按下新的开始绘制快捷键...");
}

void MainWindow::onSetStopDrawKeyClicked() {
    controlPanel_->startWaitingForKey("stopdraw");
    updateStatusBar("请按下新的停止绘制快捷键...");
}

// === 动态热键重注册 ===

void MainWindow::reregisterTopLeftHotkey(int vkCode, unsigned int modifiers) {
    // 注销旧热键
    int oldId = controlPanel_->getTopLeftHotkeyId();
    if (oldId > 0) {
        hotkeyManager_->unregisterHotkey(oldId);
    }
    // 注册新热键
    int newId = hotkeyManager_->registerHotkeyDynamic(vkCode, modifiers, [this]() {
        QMetaObject::invokeMethod(this, &MainWindow::onF7Pressed, Qt::QueuedConnection);
    });
    controlPanel_->setTopLeftHotkeyId(newId);
    topLeftVkCode_ = vkCode;
    topLeftModifiers_ = modifiers;
    updateStatusBar(QString("左上角快捷键已更新为: %1").arg(HotkeyManager::hotkeyName(vkCode, modifiers)));
}

void MainWindow::reregisterBottomRightHotkey(int vkCode, unsigned int modifiers) {
    int oldId = controlPanel_->getBottomRightHotkeyId();
    if (oldId > 0) {
        hotkeyManager_->unregisterHotkey(oldId);
    }
    int newId = hotkeyManager_->registerHotkeyDynamic(vkCode, modifiers, [this]() {
        QMetaObject::invokeMethod(this, &MainWindow::onF8Pressed, Qt::QueuedConnection);
    });
    controlPanel_->setBottomRightHotkeyId(newId);
    bottomRightVkCode_ = vkCode;
    bottomRightModifiers_ = modifiers;
    updateStatusBar(QString("右下角快捷键已更新为: %1").arg(HotkeyManager::hotkeyName(vkCode, modifiers)));
}

void MainWindow::reregisterStartDrawHotkey(int vkCode, unsigned int modifiers) {
    int oldId = controlPanel_->getStartDrawHotkeyId();
    if (oldId > 0) {
        hotkeyManager_->unregisterHotkey(oldId);
    }
    int newId = hotkeyManager_->registerHotkeyDynamic(vkCode, modifiers, [this]() {
        QMetaObject::invokeMethod(this, &MainWindow::onStartDrawing, Qt::QueuedConnection);
    });
    controlPanel_->setStartDrawHotkeyId(newId);
    startDrawVkCode_ = vkCode;
    startDrawModifiers_ = modifiers;
    updateStatusBar(QString("开始绘制快捷键已更新为: %1").arg(HotkeyManager::hotkeyName(vkCode, modifiers)));
}

void MainWindow::reregisterStopDrawHotkey(int vkCode, unsigned int modifiers) {
    int oldId = controlPanel_->getStopDrawHotkeyId();
    if (oldId > 0) {
        hotkeyManager_->unregisterHotkey(oldId);
    }
    int newId = hotkeyManager_->registerHotkeyDynamic(vkCode, modifiers, [this]() {
        QMetaObject::invokeMethod(this, &MainWindow::onStopDrawing, Qt::QueuedConnection);
    });
    controlPanel_->setStopDrawHotkeyId(newId);
    stopDrawVkCode_ = vkCode;
    stopDrawModifiers_ = modifiers;
    updateStatusBar(QString("停止绘制快捷键已更新为: %1").arg(HotkeyManager::hotkeyName(vkCode, modifiers)));
}

// === 延迟时间变化 ===

void MainWindow::onDelaySecondsChanged(int seconds) {
    updateStatusBar(QString("延迟时间已设置为: %1 秒").arg(seconds));
}

// === 文件操作槽函数 ===

void MainWindow::onImportImage() {
    QString fileName = QFileDialog::getOpenFileName(this,
        "导入图片",
        QString(),
        "图片文件 (*.png *.jpg *.jpeg *.bmp *.tiff *.gif);;所有文件 (*)");

    if (fileName.isEmpty()) return;

    if (!lineArtGenerator_.loadImage(fileName.toLocal8Bit().toStdString())) {
        QMessageBox::critical(this, "错误", "无法加载图片文件！");
        return;
    }

    currentImagePath_ = fileName;
    hasImage_ = true;
    hasLineArt_ = false;

    // 显示原始图片
    canvas_->setBackgroundImage(lineArtGenerator_.getOriginalImage());
    canvas_->clearPolylines();

    updateStatusBar(QString("已加载图片: %1 (%2x%3)")
        .arg(fileName)
        .arg(lineArtGenerator_.getWidth())
        .arg(lineArtGenerator_.getHeight()));
}

void MainWindow::onImportSVG() {
    QString fileName = QFileDialog::getOpenFileName(this,
        "导入SVG线稿",
        QString(),
        "SVG文件 (*.svg);;所有文件 (*)");

    if (fileName.isEmpty()) return;

    if (!svgImporter_.importFromFile(fileName.toLocal8Bit().toStdString())) {
        QMessageBox::critical(this, "错误",
            QString("无法导入SVG文件: %1").arg(QString::fromStdString(svgImporter_.getError())));
        return;
    }

    auto polylines = svgImporter_.getPolylines();
    if (polylines.empty()) {
        QMessageBox::warning(this, "警告", "SVG文件中没有找到可绘制的路径！");
        return;
    }

    canvas_->setPolylines(polylines);
    hasLineArt_ = true;

    updateStatusBar(QString("已导入SVG: %1 条路径").arg(polylines.size()));
}

void MainWindow::onExportSVG() {
    if (!hasLineArt_) {
        QMessageBox::warning(this, "警告", "没有可导出的线稿！");
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this,
        "导出SVG线稿",
        QString(),
        "SVG文件 (*.svg);;所有文件 (*)");

    if (fileName.isEmpty()) return;

    if (!fileName.endsWith(".svg", Qt::CaseInsensitive)) {
        fileName += ".svg";
    }

    auto polylines = canvas_->getPolylines();
    int width = lineArtGenerator_.getWidth();
    int height = lineArtGenerator_.getHeight();

    if (width == 0 || height == 0) {
        width = 800;
        height = 600;
    }

    if (!svgExporter_.exportToFile(fileName.toLocal8Bit().toStdString(), polylines, width, height)) {
        QMessageBox::critical(this, "错误", "导出SVG文件失败！");
        return;
    }

    updateStatusBar(QString("已导出SVG: %1").arg(fileName));
}

void MainWindow::onTraceToSVG() {
    if (!hasImage_) {
        QMessageBox::warning(this, "警告", "请先导入图片！");
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this,
        "图片转SVG (保留原图)",
        QString(),
        "SVG文件 (*.svg);;所有文件 (*)");

    if (fileName.isEmpty()) return;

    if (!fileName.endsWith(".svg", Qt::CaseInsensitive)) {
        fileName += ".svg";
    }

    updateStatusBar("正在进行图片追踪转换...");
    QApplication::processEvents();

    // 从LineArtGenerator获取原始图片
    cv::Mat originalImage = lineArtGenerator_.getOriginalImageCV();
    if (originalImage.empty()) {
        QMessageBox::critical(this, "错误", "无法获取原始图片数据！");
        return;
    }

    // 设置追踪参数
    TraceParams params;
    params.colors = controlPanel_->getTraceColors();
    params.blurSize = controlPanel_->getTraceBlurSize();
    params.curveThreshold = controlPanel_->getTraceCurveThreshold();
    params.fillHoles = controlPanel_->getTraceFillHoles();
    params.minArea = controlPanel_->getTraceMinArea();
    params.smoothPaths = controlPanel_->getTraceSmoothPaths();

    imageTracer_.setParams(params);

    if (!imageTracer_.loadImage(originalImage)) {
        QMessageBox::critical(this, "错误", QString::fromStdString(imageTracer_.getError()));
        return;
    }

    if (!imageTracer_.traceToSVG(fileName.toLocal8Bit().toStdString())) {
        QMessageBox::critical(this, "错误", QString::fromStdString(imageTracer_.getError()));
        return;
    }

    updateStatusBar(QString("图片转SVG完成: %1 (颜色数:%2)")
        .arg(fileName)
        .arg(params.colors));
}

// === 线稿生成槽函数 ===

void MainWindow::onGenerateLineArt() {
    if (!hasImage_) {
        QMessageBox::warning(this, "警告", "请先导入图片！");
        return;
    }

    updateStatusBar("正在生成线稿...");
    QApplication::processEvents();

    double contrast = controlPanel_->getContrast();
    int blurSize = controlPanel_->getBlurSize();
    double mergeDistance = controlPanel_->getMergeDistance();
    double simplifyEpsilon = controlPanel_->getSimplifyEpsilon();

    auto polylines = lineArtGenerator_.generateLineArt(
        contrast, blurSize, mergeDistance, simplifyEpsilon);

    if (polylines.empty()) {
        QMessageBox::warning(this, "警告", "未能生成线稿，请尝试调整参数！");
        return;
    }

    canvas_->setPolylines(polylines);
    hasLineArt_ = true;

    updateStatusBar(QString("线稿生成完成: %1 条路径").arg(polylines.size()));
}

void MainWindow::onGenerateCanny() {
    if (!hasImage_) {
        QMessageBox::warning(this, "警告", "请先导入图片！");
        return;
    }

    updateStatusBar("正在使用Canny边缘检测生成线稿...");
    QApplication::processEvents();

    double mergeDistance = controlPanel_->getMergeDistance();
    double simplifyEpsilon = controlPanel_->getSimplifyEpsilon();

    auto polylines = lineArtGenerator_.generateLineArtCanny(
        50, 150, mergeDistance, simplifyEpsilon);

    if (polylines.empty()) {
        QMessageBox::warning(this, "警告", "未能生成线稿，请尝试调整参数！");
        return;
    }

    canvas_->setPolylines(polylines);
    hasLineArt_ = true;

    updateStatusBar(QString("Canny线稿生成完成: %1 条路径").arg(polylines.size()));
}

// === 参数变化槽函数 ===

void MainWindow::onContrastChanged(double value) {
    updateStatusBar(QString("对比度: %1").arg(value));
}

void MainWindow::onBlurSizeChanged(int value) {
    updateStatusBar(QString("模糊核大小: %1").arg(value));
}

void MainWindow::onMergeDistanceChanged(double value) {
    updateStatusBar(QString("合并距离: %1").arg(value));
}

void MainWindow::onSimplifyEpsilonChanged(double value) {
    updateStatusBar(QString("简化阈值: %1").arg(value));
}

void MainWindow::onBackgroundOpacityChanged(double value) {
    canvas_->setBackgroundOpacity(value);
}

void MainWindow::onDrawingSpeedChanged(double value) {
    mousePainter_.setSpeed(value);
    updateStatusBar(QString("绘制速度: %1x").arg(value));
}

void MainWindow::onHighPrecisionChanged(bool enabled) {
    mousePainter_.setHighPrecision(enabled);
    updateStatusBar(QString("高精度模式 (贝塞尔曲线): %1").arg(enabled ? "开启" : "关闭"));
}

void MainWindow::onPixelPerfectChanged(bool enabled) {
    mousePainter_.setPixelPerfect(enabled);
    updateStatusBar(QString("像素级精确模式: %1").arg(enabled ? "开启" : "关闭"));
}

void MainWindow::onPressureLevelChanged(int level) {
    mousePainter_.setPressureLevel(level);
    QString levelStr;
    switch (level) {
        case 1: levelStr = "轻压 (细线)"; break;
        case 2: levelStr = "中压 (正常)"; break;
        case 3: levelStr = "重压 (粗线)"; break;
        default: levelStr = "未知"; break;
    }
    updateStatusBar(QString("压感级别: %1").arg(levelStr));
}

void MainWindow::onSpeedUp() {
    mousePainter_.speedUp();
    updateStatusBar("加速: 绘制速度提升");
}

void MainWindow::onSpeedDown() {
    mousePainter_.speedDown();
    updateStatusBar("减速: 绘制速度降低");
}

void MainWindow::onTogglePrecision() {
    mousePainter_.togglePrecision();
    updateStatusBar("已切换绘制精度模式");
}

void MainWindow::onCyclePressure() {
    mousePainter_.cyclePressure();
    updateStatusBar("已切换压感级别");
}

// === 编辑控制槽函数 ===

void MainWindow::onEditModeChanged(int mode) {
    canvas_->setEditMode(static_cast<EditMode>(mode));
    QString modeStr;
    switch (static_cast<EditMode>(mode)) {
        case EditMode::View: modeStr = "查看模式"; break;
        case EditMode::Select: modeStr = "选择模式"; break;
        case EditMode::Draw: modeStr = "手绘模式"; break;
        case EditMode::Erase: modeStr = "擦除模式"; break;
        case EditMode::Cut: modeStr = "切割模式 (点击线段切割)"; break;
    }
    updateStatusBar(QString("编辑模式: %1").arg(modeStr));
}

void MainWindow::onDeleteSelected() {
    canvas_->deleteSelectedPolyline();
}

void MainWindow::onClearAll() {
    auto reply = QMessageBox::question(this, "确认", "确定要清空所有线稿吗？",
                                       QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        canvas_->clearPolylines();
        hasLineArt_ = false;
    }
}

void MainWindow::onUndo() {
    if (canvas_->canUndo()) {
        canvas_->undo();
        updateStatusBar("已撤销上一步操作");
    } else {
        updateStatusBar("没有可撤销的操作");
    }
}

void MainWindow::onResetZoom() {
    canvas_->resetZoom();
}

// === 鼠标绘制槽函数 ===

void MainWindow::onStartDrawing() {
    auto polylines = canvas_->getPolylines();
    if (polylines.empty()) {
        QMessageBox::warning(this, "警告", "没有可绘制的线稿！");
        return;
    }

    // 设置绘制参数
    mousePainter_.setPolylines(polylines);
    mousePainter_.setSpeed(controlPanel_->getDrawingSpeed());
    mousePainter_.setDrawingAreaSelector(&areaSelector_);
    mousePainter_.setLineArtSize(lineArtGenerator_.getWidth(), lineArtGenerator_.getHeight());

    // 使用控制面板中的延迟时间（默认10秒，范围3-60秒）
    int delaySeconds = controlPanel_->getDelaySeconds();
    mousePainter_.setPreDrawDelay(delaySeconds * 1000);

    // 如果区域未设置，使用默认居中
    if (!areaSelector_.isValid()) {
        QScreen* screen = QApplication::primaryScreen();
        if (screen) {
            QRect screenGeometry = screen->geometry();
            int offsetX = (screenGeometry.width() - lineArtGenerator_.getWidth()) / 2;
            int offsetY = (screenGeometry.height() - lineArtGenerator_.getHeight()) / 2;
            mousePainter_.setOffset(offsetX, offsetY);
        }
    }

    mousePainter_.startDrawing();
}

void MainWindow::onStopDrawing() {
    mousePainter_.stopDrawing();
}

// === 绘制状态槽函数 ===

void MainWindow::onDrawingStarted() {
    progressBar_->setVisible(true);
    progressBar_->setValue(0);
    countdownLabel_->setVisible(true);
    updateStatusBar("开始自动绘制...");
}

void MainWindow::onDrawingFinished() {
    progressBar_->setVisible(false);
    countdownLabel_->setVisible(false);
    updateStatusBar("绘制完成！");
}

void MainWindow::onDrawingStopped() {
    progressBar_->setVisible(false);
    countdownLabel_->setVisible(false);
    updateStatusBar("绘制已停止");
}

void MainWindow::onDrawingProgress(double progress) {
    progressBar_->setValue(static_cast<int>(progress * 100));
}

void MainWindow::onDrawingError(const QString& error) {
    QMessageBox::critical(this, "绘制错误", error);
    progressBar_->setVisible(false);
    countdownLabel_->setVisible(false);
}

void MainWindow::onLineStarted(int lineIndex) {
    updateStatusBar(QString("正在绘制第 %1 条线...").arg(lineIndex + 1));
}

void MainWindow::onLineFinished(int lineIndex) {
    // 可以在这里添加完成一条线的处理
}

void MainWindow::onCountdownTick(int secondsLeft) {
    if (secondsLeft > 0) {
        countdownLabel_->setText(QString("倒计时: %1 秒后开始绘制").arg(secondsLeft));
        updateStatusBar(QString("倒计时: %1 秒后开始绘制...").arg(secondsLeft));
    } else {
        countdownLabel_->setText("开始绘制！");
    }
}

} // namespace SketchMaster
