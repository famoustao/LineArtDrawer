#include "MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>
#include <QScreen>
#include <QIcon>
#include <QPixmap>
#include <QDir>
#include <QScrollArea>
#include <QLabel>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace SketchMaster {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , hotkeyManager_(nullptr)
    , hasImage_(false)
    , hasLineArt_(false) {
    setupUI();
    connectSignals();
    setupHotkeys();
    setupTrayIcon();
    setupIcon();
    
    // 创建自定义标题栏部件
    QWidget* titleBar = new QWidget(this);
    QHBoxLayout* titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(10, 5, 10, 5);
    
    QLabel* titleLeft = new QLabel("图片自动绘制工具", this);
    titleLeft->setStyleSheet("font-size: 14px; font-weight: bold; color: #333;");
    
    QLabel* titleRight = new QLabel("涛2026年6月编译", this);
    titleRight->setStyleSheet("font-size: 12px; color: #666;");
    titleRight->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    
    titleLayout->addWidget(titleLeft);
    titleLayout->addStretch();
    titleLayout->addWidget(titleRight);
    
    setWindowTitle("图片转矢量图自动绘图工具");
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
    
    // 创建控制面板（放在滚动区域内）
    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setMaximumWidth(340);
    scrollArea->setMinimumWidth(280);
    
    controlPanel_ = new ControlPanel(this);
    scrollArea->setWidget(controlPanel_);
    splitter_->addWidget(scrollArea);
    
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
    // Windows: 使用虚拟键码 VK_F7(0x76), VK_F8(0x77)
    hotkeyManager_->registerHotkey(VK_F7, 0, [this]() {
        QMetaObject::invokeMethod(this, &MainWindow::onF7Pressed, Qt::QueuedConnection);
    });

    hotkeyManager_->registerHotkey(VK_F8, 0, [this]() {
        QMetaObject::invokeMethod(this, &MainWindow::onF8Pressed, Qt::QueuedConnection);
    });

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
    // 空格键暂停/继续
    hotkeyManager_->registerHotkey(VK_SPACE, 0, [this]() {
        QMetaObject::invokeMethod(this, &MainWindow::onStopDrawing, Qt::QueuedConnection);
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
    QStringList iconPaths;
    iconPaths << "resources/icon.jpg"
              << "resources/icon.png"
              << "../resources/icon.jpg"
              << QApplication::applicationDirPath() + "/resources/icon.jpg";
    
    for (const auto& path : iconPaths) {
        if (QFile::exists(path)) {
            QIcon icon(path);
            setWindowIcon(icon);
            // 同时设置应用程序图标（影响任务栏）
            QApplication::setWindowIcon(icon);
            break;
        }
    }
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

// === 文件操作槽函数 ===

void MainWindow::onImportImage() {
    QString fileName = QFileDialog::getOpenFileName(this,
        "导入图片",
        QString(),
        "图片文件 (*.png *.jpg *.jpeg *.bmp *.tiff *.gif);;所有文件 (*)");
    
    if (fileName.isEmpty()) return;
    
    if (!lineArtGenerator_.loadImage(fileName.toStdString())) {
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
    
    if (!svgImporter_.importFromFile(fileName.toStdString())) {
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

    if (!svgExporter_.exportToFile(fileName.toStdString(), polylines, width, height)) {
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

    if (!imageTracer_.traceToSVG(fileName.toStdString())) {
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
    
    // 设置3秒延迟
    mousePainter_.setPreDrawDelay(3000);
    
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
