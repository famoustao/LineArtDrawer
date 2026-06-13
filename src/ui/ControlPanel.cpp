#include "ControlPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFrame>
#include <QSlider>
#include <QKeyEvent>
#include <QFocusEvent>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace SketchMaster {

ControlPanel::ControlPanel(QWidget* parent)
    : QWidget(parent)
    , topLeftHotkeyId_(-1)
    , bottomRightHotkeyId_(-1)
    , startDrawHotkeyId_(-1)
    , stopDrawHotkeyId_(-1) {
    setupUI();
    connectSignals();
}

ControlPanel::~ControlPanel() {}

void ControlPanel::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(5);
    mainLayout->setContentsMargins(5, 5, 5, 5);

    // === 创建 QTabWidget ===
    tabWidget_ = new QTabWidget(this);

    // ============================
    // Tab1: 文件操作
    // ============================
    QWidget* tab1 = new QWidget(this);
    QVBoxLayout* tab1Layout = new QVBoxLayout(tab1);

    importImageBtn_ = new QPushButton("导入图片", this);
    importSVGBtn_ = new QPushButton("导入SVG线稿", this);
    exportSVGBtn_ = new QPushButton("导出SVG线稿", this);

    tab1Layout->addWidget(importImageBtn_);
    tab1Layout->addWidget(importSVGBtn_);
    tab1Layout->addWidget(exportSVGBtn_);
    tab1Layout->addStretch();

    tabWidget_->addTab(tab1, "文件操作");

    // ============================
    // Tab2: 线稿生成
    // ============================
    QWidget* tab2 = new QWidget(this);
    QVBoxLayout* tab2Layout = new QVBoxLayout(tab2);

    QGroupBox* paramGroup = new QGroupBox("线稿生成参数", this);
    QGridLayout* paramLayout = new QGridLayout(paramGroup);

    // 对比度 + 提示
    paramLayout->addWidget(new QLabel("对比度:", this), 0, 0);
    contrastSpin_ = new QDoubleSpinBox(this);
    contrastSpin_->setRange(0.1, 5.0);
    contrastSpin_->setValue(1.0);
    contrastSpin_->setSingleStep(0.1);
    paramLayout->addWidget(contrastSpin_, 0, 1);
    QLabel* contrastHint = new QLabel("推荐1.5-3.0", this);
    contrastHint->setStyleSheet("color: #888; font-size: 11px;");
    paramLayout->addWidget(contrastHint, 0, 2);

    // 模糊核大小 + 提示
    paramLayout->addWidget(new QLabel("模糊核大小:", this), 1, 0);
    blurSizeSpin_ = new QSpinBox(this);
    blurSizeSpin_->setRange(1, 21);
    blurSizeSpin_->setValue(5);
    blurSizeSpin_->setSingleStep(2);
    paramLayout->addWidget(blurSizeSpin_, 1, 1);
    QLabel* blurHint = new QLabel("推荐3-7(奇数)", this);
    blurHint->setStyleSheet("color: #888; font-size: 11px;");
    paramLayout->addWidget(blurHint, 1, 2);

    // 合并距离 + 提示
    paramLayout->addWidget(new QLabel("合并距离:", this), 2, 0);
    mergeDistanceSpin_ = new QDoubleSpinBox(this);
    mergeDistanceSpin_->setRange(0.0, 50.0);
    mergeDistanceSpin_->setValue(5.0);
    mergeDistanceSpin_->setSingleStep(1.0);
    paramLayout->addWidget(mergeDistanceSpin_, 2, 1);
    QLabel* mergeHint = new QLabel("推荐3-8", this);
    mergeHint->setStyleSheet("color: #888; font-size: 11px;");
    paramLayout->addWidget(mergeHint, 2, 2);

    // 简化阈值 + 提示
    paramLayout->addWidget(new QLabel("简化阈值:", this), 3, 0);
    simplifyEpsilonSpin_ = new QDoubleSpinBox(this);
    simplifyEpsilonSpin_->setRange(0.1, 20.0);
    simplifyEpsilonSpin_->setValue(2.0);
    simplifyEpsilonSpin_->setSingleStep(0.5);
    paramLayout->addWidget(simplifyEpsilonSpin_, 3, 1);
    QLabel* simplifyHint = new QLabel("推荐1.0-3.0", this);
    simplifyHint->setStyleSheet("color: #888; font-size: 11px;");
    paramLayout->addWidget(simplifyHint, 3, 2);

    tab2Layout->addWidget(paramGroup);

    // 生成按钮组
    QGroupBox* genGroup = new QGroupBox("生成线稿", this);
    QVBoxLayout* genLayout = new QVBoxLayout(genGroup);

    // 算法选择下拉框
    QHBoxLayout* algoLayout = new QHBoxLayout();
    algoLayout->addWidget(new QLabel("算法:", this));
    algorithmCombo_ = new QComboBox(this);
    algorithmCombo_->addItem("Canny 边缘检测 (快速/通用)", 0);
    algorithmCombo_->addItem("DoG 高斯差分 (快速/细线条)", 1);
    algorithmCombo_->addItem("Sobel 算子 (快速/简单)", 2);
    algorithmCombo_->addItem("Scharr 滤波器 (快速/精确)", 3);
    algorithmCombo_->addItem("Laplacian 拉普拉斯 (快速/二阶微分)", 4);
    algorithmCombo_->addItem("LSD 直线检测 (建筑/机械)", 5);
    algorithmCombo_->addItem("形态学边缘 (快速/粗线条)", 6);
    algorithmCombo_->addItem("HED 深度边缘 (人像/复杂) [需模型]", 7);
    algorithmCombo_->addItem("MLSD 直线检测 (建筑/直线) [需模型]", 8);
    algorithmCombo_->insertSeparator(algorithmCombo_->count());
    algorithmCombo_->addItem("ControlNet LineArt (插画/漫画)", 10);
    algorithmCombo_->addItem("ControlNet MLSD (建筑/直线)", 11);
    algorithmCombo_->addItem("ArtLine (人像/肖像)", 12);
    algorithmCombo_->addItem("CycleGAN (风格转换)", 13);
    algorithmCombo_->addItem("SAGAN (自注意力生成)", 14);
    algorithmCombo_->addItem("APDrawingGAN (肖像线稿)", 15);
    algorithmCombo_->addItem("DiT+LoRA (专业/考古)", 16);
    algoLayout->addWidget(algorithmCombo_);
    genLayout->addLayout(algoLayout);

    // 算法提示标签
    algorithmHintLabel_ = new QLabel("适用场景: 快速/通用", this);
    algorithmHintLabel_->setStyleSheet("color: #888; font-size: 11px;");
    algorithmHintLabel_->setWordWrap(true);
    genLayout->addWidget(algorithmHintLabel_);

    generateBtn_ = new QPushButton("自适应阈值生成", this);
    generateCannyBtn_ = new QPushButton("Canny边缘检测生成", this);
    generateWithAlgoBtn_ = new QPushButton("使用选中算法生成", this);
    generateWithAlgoBtn_->setStyleSheet("background-color: #4CAF50; color: white; font-weight: bold; padding: 6px;");

    genLayout->addWidget(generateBtn_);
    genLayout->addWidget(generateCannyBtn_);
    genLayout->addWidget(generateWithAlgoBtn_);

    tab2Layout->addWidget(genGroup);

    // 编辑操作组（替换原来的编辑模式下拉框）
    QGroupBox* editGroup = new QGroupBox("编辑操作", this);
    QVBoxLayout* editLayout = new QVBoxLayout(editGroup);

    // 第一行按钮：撤销、重做、删除选中、清空全部
    QHBoxLayout* editBtnLayout1 = new QHBoxLayout();
    undoBtn_ = new QPushButton("撤销", this);
    redoBtn_ = new QPushButton("重做", this);
    deleteBtn_ = new QPushButton("删除选中", this);
    clearBtn_ = new QPushButton("清空全部", this);
    editBtnLayout1->addWidget(undoBtn_);
    editBtnLayout1->addWidget(redoBtn_);
    editBtnLayout1->addWidget(deleteBtn_);
    editBtnLayout1->addWidget(clearBtn_);
    editLayout->addLayout(editBtnLayout1);

    // 第二行按钮：切割线段、智能合并、手绘模式
    QHBoxLayout* editBtnLayout2 = new QHBoxLayout();
    cutBtn_ = new QPushButton("切割线段", this);
    cutBtn_->setCheckable(true);
    cutBtn_->setToolTip("左键画切割线，右键退出切割模式");
    smartMergeBtn_ = new QPushButton("智能合并", this);
    smartMergeBtn_->setToolTip("自动合并所有端点相近的线段");
    drawModeBtn_ = new QPushButton("手绘模式", this);
    drawModeBtn_->setCheckable(true);
    drawModeBtn_->setToolTip("左键手绘新线段，再次点击退出手绘模式");
    editBtnLayout2->addWidget(cutBtn_);
    editBtnLayout2->addWidget(smartMergeBtn_);
    editBtnLayout2->addWidget(drawModeBtn_);
    editLayout->addLayout(editBtnLayout2);

    tab2Layout->addWidget(editGroup);

    // 显示控制组
    QGroupBox* displayGroup = new QGroupBox("显示控制", this);
    QVBoxLayout* displayLayout = new QVBoxLayout(displayGroup);

    QHBoxLayout* opacityLayout = new QHBoxLayout();
    opacityLayout->addWidget(new QLabel("底图透明度:", this));
    opacitySlider_ = new QSlider(Qt::Horizontal, this);
    opacitySlider_->setRange(0, 100);
    opacitySlider_->setValue(50);
    opacityLayout->addWidget(opacitySlider_);
    opacityLabel_ = new QLabel("50%", this);
    opacityLayout->addWidget(opacityLabel_);
    displayLayout->addLayout(opacityLayout);

    // 缩放滑块
    QHBoxLayout* zoomLayout = new QHBoxLayout();
    zoomLayout->addWidget(new QLabel("缩放:", this));
    zoomSlider_ = new QSlider(Qt::Horizontal, this);
    zoomSlider_->setRange(10, 500);
    zoomSlider_->setValue(100);
    zoomSlider_->setToolTip("拖动滑块放大/缩小图片");
    zoomLayout->addWidget(zoomSlider_);
    zoomPercentLabel_ = new QLabel("100%", this);
    zoomLayout->addWidget(zoomPercentLabel_);
    displayLayout->addLayout(zoomLayout);

    resetZoomBtn_ = new QPushButton("重置缩放", this);
    displayLayout->addWidget(resetZoomBtn_);

    tab2Layout->addWidget(displayGroup);
    tab2Layout->addStretch();

    tabWidget_->addTab(tab2, "线稿生成");

    // ============================
    // Tab3: 绘制控制
    // ============================
    QWidget* tab3 = new QWidget(this);
    QVBoxLayout* tab3Layout = new QVBoxLayout(tab3);

    // 绘制参数组
    QGroupBox* drawParamGroup = new QGroupBox("绘制参数", this);
    QGridLayout* drawParamLayout = new QGridLayout(drawParamGroup);

    drawParamLayout->addWidget(new QLabel("绘制速度:", this), 0, 0);
    drawingSpeedSpin_ = new QDoubleSpinBox(this);
    drawingSpeedSpin_->setRange(0.1, 10.0);
    drawingSpeedSpin_->setValue(1.0);
    drawingSpeedSpin_->setSingleStep(0.1);
    drawParamLayout->addWidget(drawingSpeedSpin_, 0, 1);

    drawParamLayout->addWidget(new QLabel("延迟时间(秒):", this), 1, 0);
    delaySpin_ = new QSpinBox(this);
    delaySpin_->setRange(3, 60);
    delaySpin_->setValue(5);  // 默认改为5秒
    delaySpin_->setSingleStep(1);
    delaySpin_->setToolTip("绘制开始前的延迟时间（3-60秒）");
    drawParamLayout->addWidget(delaySpin_, 1, 1);

    tab3Layout->addWidget(drawParamGroup);

    // 精度控制组
    QGroupBox* precisionGroup = new QGroupBox("绘制精度控制", this);
    QVBoxLayout* precisionLayout = new QVBoxLayout(precisionGroup);

    highPrecisionCheck_ = new QCheckBox("高精度模式 (贝塞尔曲线)", this);
    highPrecisionCheck_->setChecked(true);
    precisionLayout->addWidget(highPrecisionCheck_);

    pixelPerfectCheck_ = new QCheckBox("像素级精确模式", this);
    pixelPerfectCheck_->setChecked(false);
    precisionLayout->addWidget(pixelPerfectCheck_);

    QHBoxLayout* pressureLayout = new QHBoxLayout();
    pressureLayout->addWidget(new QLabel("压感级别:", this));
    pressureCombo_ = new QComboBox(this);
    pressureCombo_->addItem("轻压 (细线)", 1);
    pressureCombo_->addItem("中压 (正常)", 2);
    pressureCombo_->addItem("重压 (粗线)", 3);
    pressureLayout->addWidget(pressureCombo_);
    precisionLayout->addLayout(pressureLayout);

    tab3Layout->addWidget(precisionGroup);

    // 快捷控制组
    QGroupBox* shortcutGroup = new QGroupBox("快捷控制", this);
    QVBoxLayout* shortcutLayout = new QVBoxLayout(shortcutGroup);

    QHBoxLayout* speedBtnLayout = new QHBoxLayout();
    speedDownBtn_ = new QPushButton("减速 [-]", this);
    speedUpBtn_ = new QPushButton("加速 [+]", this);
    speedBtnLayout->addWidget(speedDownBtn_);
    speedBtnLayout->addWidget(speedUpBtn_);
    shortcutLayout->addLayout(speedBtnLayout);

    togglePrecisionBtn_ = new QPushButton("切换精度 [P]", this);
    shortcutLayout->addWidget(togglePrecisionBtn_);

    cyclePressureBtn_ = new QPushButton("切换压感 [C]", this);
    shortcutLayout->addWidget(cyclePressureBtn_);

    tab3Layout->addWidget(shortcutGroup);

    // 开始/停止绘制组
    QGroupBox* drawCtrlGroup = new QGroupBox("绘制控制", this);
    QVBoxLayout* drawCtrlLayout = new QVBoxLayout(drawCtrlGroup);

    startDrawBtn_ = new QPushButton("开始绘制 [空格]", this);
    stopDrawBtn_ = new QPushButton("停止绘制 [ESC]", this);
    drawCtrlLayout->addWidget(startDrawBtn_);
    drawCtrlLayout->addWidget(stopDrawBtn_);

    tab3Layout->addWidget(drawCtrlGroup);

    // 热键设置组
    QGroupBox* hotkeyGroup = new QGroupBox("热键设置", this);
    QGridLayout* hotkeyLayout = new QGridLayout(hotkeyGroup);

    // 设置左上角
    hotkeyLayout->addWidget(new QLabel("设置左上角:", this), 0, 0);
    setTopLeftKeyBtn_ = new QPushButton("设置", this);
    setTopLeftKeyBtn_->setMaximumWidth(60);
    hotkeyLayout->addWidget(setTopLeftKeyBtn_, 0, 1);
    topLeftKeyLabel_ = new QLabel("F7", this);
    topLeftKeyLabel_->setStyleSheet("color: #0066cc; font-weight: bold; border: 1px solid #ccc; padding: 2px 8px; background: #f0f0f0;");
    hotkeyLayout->addWidget(topLeftKeyLabel_, 0, 2);

    // 设置右下角
    hotkeyLayout->addWidget(new QLabel("设置右下角:", this), 1, 0);
    setBottomRightKeyBtn_ = new QPushButton("设置", this);
    setBottomRightKeyBtn_->setMaximumWidth(60);
    hotkeyLayout->addWidget(setBottomRightKeyBtn_, 1, 1);
    bottomRightKeyLabel_ = new QLabel("F8", this);
    bottomRightKeyLabel_->setStyleSheet("color: #0066cc; font-weight: bold; border: 1px solid #ccc; padding: 2px 8px; background: #f0f0f0;");
    hotkeyLayout->addWidget(bottomRightKeyLabel_, 1, 2);

    // 开始绘制
    hotkeyLayout->addWidget(new QLabel("开始绘制:", this), 2, 0);
    setStartDrawKeyBtn_ = new QPushButton("设置", this);
    setStartDrawKeyBtn_->setMaximumWidth(60);
    hotkeyLayout->addWidget(setStartDrawKeyBtn_, 2, 1);
    startDrawKeyLabel_ = new QLabel("Space", this);
    startDrawKeyLabel_->setStyleSheet("color: #0066cc; font-weight: bold; border: 1px solid #ccc; padding: 2px 8px; background: #f0f0f0;");
    hotkeyLayout->addWidget(startDrawKeyLabel_, 2, 2);

    // 停止绘制
    hotkeyLayout->addWidget(new QLabel("停止绘制:", this), 3, 0);
    setStopDrawKeyBtn_ = new QPushButton("设置", this);
    setStopDrawKeyBtn_->setMaximumWidth(60);
    hotkeyLayout->addWidget(setStopDrawKeyBtn_, 3, 1);
    stopDrawKeyLabel_ = new QLabel("ESC", this);
    stopDrawKeyLabel_->setStyleSheet("color: #0066cc; font-weight: bold; border: 1px solid #ccc; padding: 2px 8px; background: #f0f0f0;");
    hotkeyLayout->addWidget(stopDrawKeyLabel_, 3, 2);

    tab3Layout->addWidget(hotkeyGroup);
    tab3Layout->addStretch();

    tabWidget_->addTab(tab3, "绘制控制");

    // 将tabWidget添加到主布局
    mainLayout->addWidget(tabWidget_);
}

void ControlPanel::connectSignals() {
    // 按钮信号
    connect(importImageBtn_, &QPushButton::clicked, this, &ControlPanel::importImageClicked);
    connect(importSVGBtn_, &QPushButton::clicked, this, &ControlPanel::importSVGClicked);
    connect(exportSVGBtn_, &QPushButton::clicked, this, &ControlPanel::exportSVGClicked);
    connect(generateBtn_, &QPushButton::clicked, this, &ControlPanel::generateLineArtClicked);
    connect(generateCannyBtn_, &QPushButton::clicked, this, &ControlPanel::generateCannyClicked);
    connect(generateWithAlgoBtn_, &QPushButton::clicked, this, &ControlPanel::generateWithAlgorithmClicked);

    // 算法选择变化信号
    connect(algorithmCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        // 更新提示标签
        QString hint;
        switch (index) {
            case 0:  hint = "适用场景: 快速/通用 - 经典Canny边缘检测，适合大多数图片"; break;
            case 1:  hint = "适用场景: 快速/细线条 - 高斯差分，对细线条效果好"; break;
            case 2:  hint = "适用场景: 快速/简单 - 经典梯度边缘检测"; break;
            case 3:  hint = "适用场景: 快速/精确 - 比Sobel更精确的梯度检测"; break;
            case 4:  hint = "适用场景: 快速/二阶微分 - 二阶微分边缘检测，对噪声敏感"; break;
            case 5:  hint = "适用场景: 建筑/机械 - 直线段检测，适合建筑结构图"; break;
            case 6:  hint = "适用场景: 快速/粗线条 - 膨胀减腐蚀提取边缘"; break;
            case 7:  hint = "适用场景: 人像/复杂 - 深度学习边缘检测，需要 models/hed/ 模型文件"; break;
            case 8:  hint = "适用场景: 建筑/直线 - 深度学习直线检测，需要 models/mlsd/ 模型文件"; break;
            case 10: hint = "适用场景: 插画/漫画 - 需要Python+PyTorch+diffusers"; break;
            case 11: hint = "适用场景: 建筑/直线 - 需要Python+PyTorch+diffusers"; break;
            case 12: hint = "适用场景: 人像/肖像 - 需要Python+PyTorch"; break;
            case 13: hint = "适用场景: 风格转换 - 需要Python+PyTorch"; break;
            case 14: hint = "适用场景: 自注意力生成 - 需要Python+PyTorch"; break;
            case 15: hint = "适用场景: 肖像线稿 - 需要Python+PyTorch"; break;
            case 16: hint = "适用场景: 专业/考古 - 需要Python+PyTorch"; break;
            default: hint = ""; break;
        }
        algorithmHintLabel_->setText(hint);
        emit algorithmChanged(index);
    });
    connect(deleteBtn_, &QPushButton::clicked, this, &ControlPanel::deleteSelectedClicked);
    connect(clearBtn_, &QPushButton::clicked, this, &ControlPanel::clearAllClicked);
    connect(undoBtn_, &QPushButton::clicked, this, &ControlPanel::undoClicked);
    connect(redoBtn_, &QPushButton::clicked, this, &ControlPanel::redoClicked);
    connect(startDrawBtn_, &QPushButton::clicked, this, &ControlPanel::startDrawingClicked);
    connect(stopDrawBtn_, &QPushButton::clicked, this, &ControlPanel::stopDrawingClicked);
    connect(resetZoomBtn_, &QPushButton::clicked, this, &ControlPanel::resetZoomClicked);
    connect(zoomSlider_, &QSlider::valueChanged, this, [this](int value) {
        zoomPercentLabel_->setText(QString("%1%").arg(value));
        emit zoomSliderChanged(value);
    });
    connect(speedUpBtn_, &QPushButton::clicked, this, &ControlPanel::speedUpClicked);
    connect(speedDownBtn_, &QPushButton::clicked, this, &ControlPanel::speedDownClicked);
    connect(togglePrecisionBtn_, &QPushButton::clicked, this, &ControlPanel::togglePrecisionClicked);
    connect(cyclePressureBtn_, &QPushButton::clicked, this, &ControlPanel::cyclePressureClicked);

    // 新增按钮信号：切割模式（切换）
    connect(cutBtn_, &QPushButton::toggled, this, [this](bool checked) {
        emit cutModeToggled(checked);
    });

    // 智能合并
    connect(smartMergeBtn_, &QPushButton::clicked, this, &ControlPanel::smartMergeClicked);

    // 手绘模式（切换）
    connect(drawModeBtn_, &QPushButton::toggled, this, [this](bool checked) {
        emit drawModeToggled(checked);
    });

    // 热键设置按钮信号
    connect(setTopLeftKeyBtn_, &QPushButton::clicked, this, &ControlPanel::setTopLeftKeyClicked);
    connect(setBottomRightKeyBtn_, &QPushButton::clicked, this, &ControlPanel::setBottomRightKeyClicked);
    connect(setStartDrawKeyBtn_, &QPushButton::clicked, this, &ControlPanel::setStartDrawKeyClicked);
    connect(setStopDrawKeyBtn_, &QPushButton::clicked, this, &ControlPanel::setStopDrawKeyClicked);

    // 参数变化信号
    connect(contrastSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ControlPanel::contrastChanged);
    connect(blurSizeSpin_, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ControlPanel::blurSizeChanged);
    connect(mergeDistanceSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ControlPanel::mergeDistanceChanged);
    connect(simplifyEpsilonSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ControlPanel::simplifyEpsilonChanged);
    connect(drawingSpeedSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ControlPanel::drawingSpeedChanged);
    connect(delaySpin_, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ControlPanel::delaySecondsChanged);

    // 精度控制
    connect(highPrecisionCheck_, &QCheckBox::stateChanged, [this](int state) {
        emit highPrecisionChanged(state == Qt::Checked);
    });
    connect(pixelPerfectCheck_, &QCheckBox::stateChanged, [this](int state) {
        emit pixelPerfectChanged(state == Qt::Checked);
    });
    connect(pressureCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this](int index) {
                emit pressureLevelChanged(index + 1);
            });

    // 透明度滑块
    connect(opacitySlider_, &QSlider::valueChanged, [this](int value) {
        opacityLabel_->setText(QString::number(value) + "%");
        emit backgroundOpacityChanged(value / 100.0);
    });
}

void ControlPanel::setCutModeActive(bool active) {
    cutBtn_->setChecked(active);
    if (active) {
        cutBtn_->setStyleSheet("background-color: #ffcccc; font-weight: bold;");
        cutBtn_->setText("切割线段 (开)");
    } else {
        cutBtn_->setStyleSheet("");
        cutBtn_->setText("切割线段");
    }
}

void ControlPanel::setDrawModeActive(bool active) {
    drawModeBtn_->setChecked(active);
    if (active) {
        drawModeBtn_->setStyleSheet("background-color: #ccccff; font-weight: bold;");
        drawModeBtn_->setText("手绘模式 (开)");
    } else {
        drawModeBtn_->setStyleSheet("");
        drawModeBtn_->setText("手绘模式");
    }
}

void ControlPanel::keyPressEvent(QKeyEvent* event) {
    if (!waitingForKeyWhich_.isEmpty()) {
        // 处于等待按键模式，捕获按键
        int key = event->key();
        int modifiers = 0;
        if (event->modifiers() & Qt::ControlModifier) modifiers |= MOD_CONTROL;
        if (event->modifiers() & Qt::ShiftModifier)   modifiers |= MOD_SHIFT;
        if (event->modifiers() & Qt::AltModifier)     modifiers |= MOD_ALT;

        // 转换Qt键码到Windows虚拟键码
        int vkCode = key;
        if (key >= Qt::Key_F1 && key <= Qt::Key_F24) {
            vkCode = VK_F1 + (key - Qt::Key_F1);
        } else if (key == Qt::Key_Space) {
            vkCode = VK_SPACE;
        } else if (key == Qt::Key_Escape) {
            vkCode = VK_ESCAPE;
        } else if (key == Qt::Key_Return || key == Qt::Key_Enter) {
            vkCode = VK_RETURN;
        } else if (key >= Qt::Key_A && key <= Qt::Key_Z) {
            vkCode = 'A' + (key - Qt::Key_A);
        } else if (key >= Qt::Key_0 && key <= Qt::Key_9) {
            vkCode = '0' + (key - Qt::Key_0);
        }

        // 更新显示
        updateHotkeyDisplay(waitingForKeyWhich_, vkCode, modifiers);

        // 退出等待按键模式
        waitingForKeyWhich_.clear();
        setFocusPolicy(Qt::NoFocus); // 恢复默认焦点策略
        return;
    }

    QWidget::keyPressEvent(event);
}

void ControlPanel::startWaitingForKey(const QString& which) {
    waitingForKeyWhich_ = which;
    setFocusPolicy(Qt::StrongFocus);
    setFocus();

    // 更新对应标签为"请按键..."
    if (which == "topleft") {
        topLeftKeyLabel_->setText("请按键...");
        topLeftKeyLabel_->setStyleSheet("color: #cc6600; font-weight: bold; border: 1px solid #cc6600; padding: 2px 8px; background: #fff8e0;");
    } else if (which == "bottomright") {
        bottomRightKeyLabel_->setText("请按键...");
        bottomRightKeyLabel_->setStyleSheet("color: #cc6600; font-weight: bold; border: 1px solid #cc6600; padding: 2px 8px; background: #fff8e0;");
    } else if (which == "startdraw") {
        startDrawKeyLabel_->setText("请按键...");
        startDrawKeyLabel_->setStyleSheet("color: #cc6600; font-weight: bold; border: 1px solid #cc6600; padding: 2px 8px; background: #fff8e0;");
    } else if (which == "stopdraw") {
        stopDrawKeyLabel_->setText("请按键...");
        stopDrawKeyLabel_->setStyleSheet("color: #cc6600; font-weight: bold; border: 1px solid #cc6600; padding: 2px 8px; background: #fff8e0;");
    }
}

void ControlPanel::updateHotkeyDisplay(const QString& which, int vkCode, unsigned int modifiers) {
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
    if (vkCode == 0xFFBF) name += "F7";
    else if (vkCode == 0xFFC0) name += "F8";
    else name += "Key(" + QString::number(vkCode) + ")";
#endif

    QString style = "color: #0066cc; font-weight: bold; border: 1px solid #ccc; padding: 2px 8px; background: #f0f0f0;";

    if (which == "topleft") {
        topLeftKeyLabel_->setText(name);
        topLeftKeyLabel_->setStyleSheet(style);
    } else if (which == "bottomright") {
        bottomRightKeyLabel_->setText(name);
        bottomRightKeyLabel_->setStyleSheet(style);
    } else if (which == "startdraw") {
        startDrawKeyLabel_->setText(name);
        startDrawKeyLabel_->setStyleSheet(style);
    } else if (which == "stopdraw") {
        stopDrawKeyLabel_->setText(name);
        stopDrawKeyLabel_->setStyleSheet(style);
    }
}

// Getter方法实现
double ControlPanel::getContrast() const {
    return contrastSpin_->value();
}

int ControlPanel::getBlurSize() const {
    return blurSizeSpin_->value();
}

double ControlPanel::getMergeDistance() const {
    return mergeDistanceSpin_->value();
}

double ControlPanel::getSimplifyEpsilon() const {
    return simplifyEpsilonSpin_->value();
}

double ControlPanel::getBackgroundOpacity() const {
    return opacitySlider_->value() / 100.0;
}

double ControlPanel::getDrawingSpeed() const {
    return drawingSpeedSpin_->value();
}

bool ControlPanel::isHighPrecisionEnabled() const {
    return highPrecisionCheck_->isChecked();
}

bool ControlPanel::isPixelPerfectEnabled() const {
    return pixelPerfectCheck_->isChecked();
}

int ControlPanel::getPressureLevel() const {
    return pressureCombo_->currentIndex() + 1;
}

int ControlPanel::getDelaySeconds() const {
    return delaySpin_->value();
}

// Setter方法实现
void ControlPanel::setContrast(double value) {
    contrastSpin_->setValue(value);
}

void ControlPanel::setBlurSize(int value) {
    blurSizeSpin_->setValue(value);
}

void ControlPanel::setMergeDistance(double value) {
    mergeDistanceSpin_->setValue(value);
}

void ControlPanel::setSimplifyEpsilon(double value) {
    simplifyEpsilonSpin_->setValue(value);
}

void ControlPanel::setBackgroundOpacity(double value) {
    int intValue = static_cast<int>(value * 100);
    opacitySlider_->setValue(intValue);
    opacityLabel_->setText(QString::number(intValue) + "%");
}

void ControlPanel::setDrawingSpeed(double value) {
    drawingSpeedSpin_->setValue(value);
}

void ControlPanel::setHighPrecision(bool enabled) {
    highPrecisionCheck_->setChecked(enabled);
}

void ControlPanel::setPixelPerfect(bool enabled) {
    pixelPerfectCheck_->setChecked(enabled);
}

void ControlPanel::setPressureLevel(int level) {
    if (level >= 1 && level <= 3) {
        pressureCombo_->setCurrentIndex(level - 1);
    }
}

// 图像追踪参数 Getter
int ControlPanel::getTraceColors() const {
    return traceColorsSpin_->value();
}

int ControlPanel::getTraceBlurSize() const {
    return traceBlurSpin_->value();
}

double ControlPanel::getTraceCurveThreshold() const {
    return traceThresholdSpin_->value();
}

bool ControlPanel::getTraceFillHoles() const {
    return traceFillHolesCheck_->isChecked();
}

int ControlPanel::getTraceMinArea() const {
    return traceMinAreaSpin_->value();
}

bool ControlPanel::getTraceSmoothPaths() const {
    return traceSmoothPathsCheck_->isChecked();
}

int ControlPanel::getSelectedAlgorithm() const {
    return algorithmCombo_->currentIndex();
}

} // namespace SketchMaster
