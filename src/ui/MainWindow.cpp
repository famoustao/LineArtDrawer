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
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>

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
    , stopDrawModifiers_(0)
    , externalProcess_(nullptr)
    , tempDir_(nullptr) {
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
    if (externalProcess_) {
        externalProcess_->kill();
        externalProcess_->deleteLater();
        externalProcess_ = nullptr;
    }
    if (tempDir_) {
        delete tempDir_;
        tempDir_ = nullptr;
    }
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

    // 创建控制面板
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

    // Ctrl+Y 重做快捷键
    redoShortcut_ = new QShortcut(QKeySequence("Ctrl+Y"), this);
    connect(redoShortcut_, &QShortcut::activated, this, &MainWindow::onRedo);

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
    connect(controlPanel_, &ControlPanel::generateLineArtClicked,
            this, &MainWindow::onGenerateLineArt);
    connect(controlPanel_, &ControlPanel::generateCannyClicked,
            this, &MainWindow::onGenerateCanny);
    connect(controlPanel_, &ControlPanel::generateWithAlgorithmClicked,
            this, &MainWindow::onGenerateWithAlgorithm);
    connect(controlPanel_, &ControlPanel::deleteSelectedClicked,
            this, &MainWindow::onDeleteSelected);
    connect(controlPanel_, &ControlPanel::clearAllClicked,
            this, &MainWindow::onClearAll);
    connect(controlPanel_, &ControlPanel::undoClicked,
            this, &MainWindow::onUndo);
    connect(controlPanel_, &ControlPanel::redoClicked,
            this, &MainWindow::onRedo);
    connect(controlPanel_, &ControlPanel::resetZoomClicked,
            this, &MainWindow::onResetZoom);
    connect(controlPanel_, &ControlPanel::zoomSliderChanged,
            this, [this](int value) {
        canvas_->setZoomFromSlider(value);
    });
    connect(controlPanel_, &ControlPanel::startDrawingClicked,
            this, &MainWindow::onStartDrawing);
    connect(controlPanel_, &ControlPanel::stopDrawingClicked,
            this, &MainWindow::onStopDrawing);

    // 新增信号连接：切割/合并/手绘模式
    connect(controlPanel_, &ControlPanel::cutModeToggled,
            this, &MainWindow::onCutModeToggled);
    connect(controlPanel_, &ControlPanel::smartMergeClicked,
            this, &MainWindow::onSmartMergeClicked);
    connect(controlPanel_, &ControlPanel::drawModeToggled,
            this, &MainWindow::onDrawModeToggled);

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

    // 画布切割模式状态变化 -> 同步更新控制面板按钮
    connect(canvas_, &SketchCanvas::cutModeChanged,
            controlPanel_, &ControlPanel::setCutModeActive);

    // 画布手绘模式状态变化 -> 同步更新控制面板按钮
    connect(canvas_, &SketchCanvas::drawModeChanged,
            controlPanel_, &ControlPanel::setDrawModeActive);
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

        // F8设置右下角后，如果区域有效，自动开始倒计时并绘制
        if (areaSelector_.isValid()) {
            updateStatusBar("绘制区域已确定，即将自动开始倒计时...");
            onStartDrawing();
        }
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

void MainWindow::onGenerateWithAlgorithm() {
    if (!hasImage_) {
        QMessageBox::warning(this, "警告", "请先导入图片！");
        return;
    }

    int algoIndex = controlPanel_->getSelectedAlgorithm();

    // 算法名称列表（与ControlPanel中的下拉框索引对应）
    // 索引0-8: 内置算法（Canny, DoG, Sobel, Scharr, Laplacian, LSD, 形态学, HED, MLSD）
    // 索引9: 分隔符
    // 索引10-16: 外部算法（ControlNet LineArt, ControlNet MLSD, ArtLine, CycleGAN, SAGAN, APDrawingGAN, DiT+LoRA）

    double mergeDistance = controlPanel_->getMergeDistance();
    double simplifyEpsilon = controlPanel_->getSimplifyEpsilon();

    // 内置算法 (索引 0-8)
    if (algoIndex >= 0 && algoIndex <= 8) {
        updateStatusBar("正在使用内置算法生成线稿...");
        QApplication::processEvents();

        std::vector<Polyline> polylines;

        switch (algoIndex) {
            case 0: // Canny
                updateStatusBar("正在使用 Canny 边缘检测...");
                QApplication::processEvents();
                polylines = lineArtGenerator_.generateLineArtCanny(50, 150, mergeDistance, simplifyEpsilon);
                break;
            case 1: // DoG
                updateStatusBar("正在使用 DoG 高斯差分...");
                QApplication::processEvents();
                polylines = lineArtGenerator_.generateLineArtDoG(1.0, 3.0, mergeDistance, simplifyEpsilon);
                break;
            case 2: // Sobel
                updateStatusBar("正在使用 Sobel 算子...");
                QApplication::processEvents();
                polylines = lineArtGenerator_.generateLineArtSobel(mergeDistance, simplifyEpsilon);
                break;
            case 3: // Scharr
                updateStatusBar("正在使用 Scharr 滤波器...");
                QApplication::processEvents();
                polylines = lineArtGenerator_.generateLineArtScharr(mergeDistance, simplifyEpsilon);
                break;
            case 4: // Laplacian
                updateStatusBar("正在使用 Laplacian 拉普拉斯...");
                QApplication::processEvents();
                polylines = lineArtGenerator_.generateLineArtLaplacian(3, mergeDistance, simplifyEpsilon);
                break;
            case 5: // LSD
                updateStatusBar("正在使用 LSD 直线检测...");
                QApplication::processEvents();
                polylines = lineArtGenerator_.generateLineArtLSD(mergeDistance, simplifyEpsilon);
                break;
            case 6: // Morphological
                updateStatusBar("正在使用形态学边缘检测...");
                QApplication::processEvents();
                polylines = lineArtGenerator_.generateLineArtMorphological(3, mergeDistance, simplifyEpsilon);
                break;
            case 7: // HED
                updateStatusBar("正在使用 HED 深度边缘检测...");
                QApplication::processEvents();
                polylines = lineArtGenerator_.generateLineArtHED(mergeDistance, simplifyEpsilon);
                break;
            case 8: // MLSD
                updateStatusBar("正在使用 MLSD 直线检测...");
                QApplication::processEvents();
                polylines = lineArtGenerator_.generateLineArtMLSD(mergeDistance, simplifyEpsilon);
                break;
            default:
                break;
        }

        if (polylines.empty()) {
            // 对于 HED/MLSD，如果返回空可能是模型未找到，给出更详细的提示
            if (algoIndex == 7 || algoIndex == 8) {
                QString modelName = (algoIndex == 7) ? "HED" : "MLSD";
                QString modelDir = (algoIndex == 7) ? "models/hed/" : "models/mlsd/";
                QMessageBox::warning(this, "模型未找到",
                    QString("未能加载 %1 模型。\n\n"
                            "请在程序目录下的 %2 文件夹中放置模型文件。\n"
                            "下载方式请参考 models/ 目录下的 README.md。\n\n"
                            "或者运行 models/download_models.bat 下载模型。")
                        .arg(modelName).arg(modelDir));
            } else {
                QMessageBox::warning(this, "警告", "未能生成线稿，请尝试调整参数！");
            }
            return;
        }

        canvas_->setPolylines(polylines);
        hasLineArt_ = true;

        QStringList algoNames = {"Canny", "DoG", "Sobel", "Scharr", "Laplacian", "LSD", "形态学", "HED", "MLSD"};
        updateStatusBar(QString("%1 线稿生成完成: %2 条路径")
            .arg(algoNames[algoIndex])
            .arg(polylines.size()));
        return;
    }

    // 外部算法 (索引 10-16，跳过分隔符索引9)
    if (algoIndex >= 10) {
        // 外部算法名称映射
        QStringList algoNames = {
            "controlnet_lineart",  // 索引10
            "controlnet_mlsd",      // 索引11
            "artline",              // 索引12
            "cyclegan",             // 索引13
            "sagan",                // 索引14
            "apdrawinggan",         // 索引15
            "dit_lora"              // 索引16
        };

        int externalIndex = algoIndex - 10;
        if (externalIndex < 0 || externalIndex >= algoNames.size()) {
            QMessageBox::warning(this, "警告", "未知的算法选择！");
            return;
        }

        QString algoName = algoNames[externalIndex];

        // 查找Python脚本路径
        QStringList scriptPaths;
        scriptPaths << QDir::currentPath() + "/scripts/lineart_external.py"
                     << QCoreApplication::applicationDirPath() + "/scripts/lineart_external.py"
                     << QDir::currentPath() + "/../scripts/lineart_external.py";

        QString scriptPath;
        for (const auto& path : scriptPaths) {
            if (QFile::exists(path)) {
                scriptPath = path;
                break;
            }
        }

        if (scriptPath.isEmpty()) {
            QMessageBox::critical(this, "错误",
                "未找到外部算法脚本 (scripts/lineart_external.py)。\n"
                "请确保脚本文件位于程序目录下的 scripts/ 文件夹中。");
            return;
        }

        // 创建临时目录用于存放中间文件
        if (tempDir_) {
            delete tempDir_;
        }
        tempDir_ = new QTemporaryDir();
        if (!tempDir_->isValid()) {
            QMessageBox::critical(this, "错误", "无法创建临时目录！");
            return;
        }

        // 保存当前图片到临时文件
        QString tempInputPath = tempDir_->path() + "/input.png";
        externalOutputPath_ = tempDir_->path() + "/output.png";

        cv::Mat originalImage = lineArtGenerator_.getOriginalImageCV();
        if (originalImage.empty()) {
            QMessageBox::critical(this, "错误", "无法获取原始图片数据！");
            return;
        }
        cv::imwrite(tempInputPath.toLocal8Bit().toStdString(), originalImage);

        // 启动Python进程
        if (externalProcess_) {
            externalProcess_->kill();
            externalProcess_->deleteLater();
        }

        externalProcess_ = new QProcess(this);

        connect(externalProcess_, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, &MainWindow::onExternalAlgorithmFinished);
        connect(externalProcess_, &QProcess::errorOccurred,
                this, &MainWindow::onExternalAlgorithmError);

        // 查找Python可执行文件
        QString pythonCmd = "python3";
        QProcess whichPython;
        whichPython.start("which", QStringList() << "python3");
        whichPython.waitForFinished(3000);
        if (whichPython.exitCode() != 0) {
            pythonCmd = "python";
        }

        QStringList args;
        args << scriptPath
             << "--algorithm" << algoName
             << "--input" << tempInputPath
             << "--output" << externalOutputPath_;

        updateStatusBar(QString("正在调用外部算法 %1，请稍候...").arg(algoName));
        QApplication::processEvents();

        externalProcess_->start(pythonCmd, args);
    }
}

void MainWindow::onExternalAlgorithmFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    if (exitStatus != QProcess::NormalExit || exitCode != 0) {
        QString errorMsg = QString::fromLocal8Bit(externalProcess_->readAllStandardError());
        if (errorMsg.isEmpty()) {
            errorMsg = QString::fromLocal8Bit(externalProcess_->readAllStandardOutput());
        }
        QMessageBox::critical(this, "外部算法执行失败",
            QString("Python脚本执行失败 (退出码: %1)\n\n错误信息:\n%2\n\n"
                   "提示:\n"
                   "- 请确保已安装 Python 3.8+\n"
                   "- 请确保已安装所需依赖: pip install torch torchvision diffusers opencv-python Pillow numpy\n"
                   "- 部分算法需要 GPU 支持")
                .arg(exitCode)
                .arg(errorMsg.isEmpty() ? "无详细错误信息" : errorMsg));
        updateStatusBar("外部算法执行失败");
        return;
    }

    // 读取输出
    QString output = QString::fromLocal8Bit(externalProcess_->readAllStandardOutput()).trimmed();

    // 尝试解析JSON输出
    QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8());
    if (doc.isObject()) {
        QJsonObject obj = doc.object();
        if (obj["status"].toString() == "error") {
            QMessageBox::critical(this, "外部算法错误",
                QString("算法执行出错:\n%1").arg(obj["message"].toString()));
            updateStatusBar("外部算法执行出错");
            return;
        }
    }

    // 读取输出的边缘图像
    cv::Mat edgeImg = cv::imread(externalOutputPath_.toLocal8Bit().toStdString(), cv::IMREAD_UNCHANGED);
    if (edgeImg.empty()) {
        QMessageBox::critical(this, "错误",
            "未能读取外部算法输出的边缘图像。\n"
            "请检查输出文件: " + externalOutputPath_);
        updateStatusBar("外部算法输出读取失败");
        return;
    }

    // 使用PathSimplifier提取路径
    double mergeDistance = controlPanel_->getMergeDistance();
    double simplifyEpsilon = controlPanel_->getSimplifyEpsilon();

    auto polylines = lineArtGenerator_.generateLineArtFromEdgeImage(edgeImg, mergeDistance, simplifyEpsilon);

    if (polylines.empty()) {
        QMessageBox::warning(this, "警告", "外部算法生成的边缘图像中未检测到有效路径！");
        return;
    }

    canvas_->setPolylines(polylines);
    hasLineArt_ = true;

    // 显示提示信息（如果有）
    QString message;
    if (doc.isObject()) {
        message = doc.object()["message"].toString();
    }

    QString statusMsg = QString("外部算法线稿生成完成: %1 条路径").arg(polylines.size());
    if (!message.isEmpty()) {
        statusMsg += " (" + message + ")";
    }
    updateStatusBar(statusMsg);
}

void MainWindow::onExternalAlgorithmError(QProcess::ProcessError error) {
    QString errorStr;
    switch (error) {
        case QProcess::FailedToStart:
            errorStr = "无法启动Python进程。请确保已安装 Python 3.8+，并且 'python3' 或 'python' 命令可用。";
            break;
        case QProcess::Crashed:
            errorStr = "Python进程崩溃。可能是依赖库版本不兼容。";
            break;
        case QProcess::Timedout:
            errorStr = "Python进程执行超时。";
            break;
        default:
            errorStr = QString("Python进程发生未知错误 (错误码: %1)。").arg(error);
            break;
    }

    QMessageBox::critical(this, "外部算法错误", errorStr);
    updateStatusBar("外部算法执行失败: " + errorStr);
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

void MainWindow::onCutModeToggled(bool active) {
    if (active) {
        // 如果手绘模式正在使用，先退出
        if (canvas_->getCanvasState() == CanvasState::Drawing) {
            canvas_->stopDrawMode();
        }
        canvas_->startCutMode();
        updateStatusBar("切割模式：左键画切割线，右键退出");
    } else {
        canvas_->stopCutMode();
        updateStatusBar("已退出切割模式");
    }
}

void MainWindow::onSmartMergeClicked() {
    canvas_->smartMergeAll();
    updateStatusBar("智能合并完成");
}

void MainWindow::onDrawModeToggled(bool active) {
    if (active) {
        // 如果切割模式正在使用，先退出
        if (canvas_->getCanvasState() == CanvasState::Cutting) {
            canvas_->stopCutMode();
        }
        canvas_->startDrawMode();
        updateStatusBar("手绘模式：左键手绘，再次点击退出");
    } else {
        canvas_->stopDrawMode();
        updateStatusBar("已退出手绘模式");
    }
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

void MainWindow::onRedo() {
    if (canvas_->canRedo()) {
        canvas_->redo();
        updateStatusBar("已重做操作");
    } else {
        updateStatusBar("没有可重做的操作");
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

    // 如果 lineArtGenerator 宽高为0（比如通过导入SVG加载的），从 polylines 计算边界框
    int artWidth = lineArtGenerator_.getWidth();
    int artHeight = lineArtGenerator_.getHeight();
    double artOriginX = 0, artOriginY = 0;
    if (artWidth <= 0 || artHeight <= 0) {
        double minX = 1e9, minY = 1e9, maxX = -1e9, maxY = -1e9;
        for (const auto& poly : polylines) {
            for (const auto& pt : poly.points) {
                if (pt.x < minX) minX = pt.x;
                if (pt.y < minY) minY = pt.y;
                if (pt.x > maxX) maxX = pt.x;
                if (pt.y > maxY) maxY = pt.y;
            }
        }
        artOriginX = minX;
        artOriginY = minY;
        artWidth = static_cast<int>(maxX - minX) + 1;
        artHeight = static_cast<int>(maxY - minY) + 1;
        if (artWidth <= 0) artWidth = 800;
        if (artHeight <= 0) artHeight = 600;
    }
    mousePainter_.setLineArtSize(artWidth, artHeight);
    mousePainter_.setLineArtOrigin(artOriginX, artOriginY);

    // 使用控制面板中的延迟时间（默认5秒，范围3-60秒）
    int delaySeconds = controlPanel_->getDelaySeconds();
    mousePainter_.setPreDrawDelay(delaySeconds * 1000);

    // 如果区域未设置，使用默认居中
    if (!areaSelector_.isValid()) {
        QScreen* screen = QApplication::primaryScreen();
        if (screen) {
            QRect screenGeometry = screen->geometry();
            int offsetX = (screenGeometry.width() - artWidth) / 2;
            int offsetY = (screenGeometry.height() - artHeight) / 2;
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
