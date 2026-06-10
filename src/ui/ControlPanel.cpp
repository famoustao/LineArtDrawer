#include "ControlPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFrame>

namespace SketchMaster {

ControlPanel::ControlPanel(QWidget* parent)
    : QWidget(parent) {
    setupUI();
    connectSignals();
}

ControlPanel::~ControlPanel() {}

void ControlPanel::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // === 文件操作组 ===
    QGroupBox* fileGroup = new QGroupBox("文件操作", this);
    QVBoxLayout* fileLayout = new QVBoxLayout(fileGroup);

    importImageBtn_ = new QPushButton("导入图片", this);
    importSVGBtn_ = new QPushButton("导入SVG线稿", this);
    exportSVGBtn_ = new QPushButton("导出SVG线稿", this);
    traceToSVGBtn_ = new QPushButton("图片转SVG (保留原图)", this);

    fileLayout->addWidget(importImageBtn_);
    fileLayout->addWidget(importSVGBtn_);
    fileLayout->addWidget(exportSVGBtn_);
    fileLayout->addWidget(traceToSVGBtn_);

    mainLayout->addWidget(fileGroup);

    // === 线稿生成参数组 ===
    QGroupBox* paramGroup = new QGroupBox("线稿生成参数", this);
    QGridLayout* paramLayout = new QGridLayout(paramGroup);

    // 对比度
    paramLayout->addWidget(new QLabel("对比度:", this), 0, 0);
    contrastSpin_ = new QDoubleSpinBox(this);
    contrastSpin_->setRange(0.1, 5.0);
    contrastSpin_->setValue(1.0);
    contrastSpin_->setSingleStep(0.1);
    paramLayout->addWidget(contrastSpin_, 0, 1);

    // 模糊核大小
    paramLayout->addWidget(new QLabel("模糊核大小:", this), 1, 0);
    blurSizeSpin_ = new QSpinBox(this);
    blurSizeSpin_->setRange(1, 21);
    blurSizeSpin_->setValue(5);
    blurSizeSpin_->setSingleStep(2);
    paramLayout->addWidget(blurSizeSpin_, 1, 1);

    // 合并距离
    paramLayout->addWidget(new QLabel("合并距离:", this), 2, 0);
    mergeDistanceSpin_ = new QDoubleSpinBox(this);
    mergeDistanceSpin_->setRange(0.0, 50.0);
    mergeDistanceSpin_->setValue(5.0);
    mergeDistanceSpin_->setSingleStep(1.0);
    paramLayout->addWidget(mergeDistanceSpin_, 2, 1);

    // 简化阈值
    paramLayout->addWidget(new QLabel("简化阈值:", this), 3, 0);
    simplifyEpsilonSpin_ = new QDoubleSpinBox(this);
    simplifyEpsilonSpin_->setRange(0.1, 20.0);
    simplifyEpsilonSpin_->setValue(2.0);
    simplifyEpsilonSpin_->setSingleStep(0.5);
    paramLayout->addWidget(simplifyEpsilonSpin_, 3, 1);

    mainLayout->addWidget(paramGroup);

    // === 生成按钮组 ===
    QGroupBox* genGroup = new QGroupBox("生成线稿", this);
    QVBoxLayout* genLayout = new QVBoxLayout(genGroup);

    generateBtn_ = new QPushButton("自适应阈值生成", this);
    generateCannyBtn_ = new QPushButton("Canny边缘检测生成", this);

    genLayout->addWidget(generateBtn_);
    genLayout->addWidget(generateCannyBtn_);

    mainLayout->addWidget(genGroup);

    // === 显示控制组 ===
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

    resetZoomBtn_ = new QPushButton("重置缩放", this);
    displayLayout->addWidget(resetZoomBtn_);

    mainLayout->addWidget(displayGroup);

    // === 编辑模式组 ===
    QGroupBox* editGroup = new QGroupBox("编辑模式", this);
    QVBoxLayout* editLayout = new QVBoxLayout(editGroup);

    editModeCombo_ = new QComboBox(this);
    editModeCombo_->addItem("查看模式", 0);
    editModeCombo_->addItem("选择模式", 1);
    editModeCombo_->addItem("手绘模式", 2);
    editModeCombo_->addItem("擦除模式", 3);
    editLayout->addWidget(editModeCombo_);

    deleteBtn_ = new QPushButton("删除选中", this);
    clearBtn_ = new QPushButton("清空全部", this);
    editLayout->addWidget(deleteBtn_);
    editLayout->addWidget(clearBtn_);

    mainLayout->addWidget(editGroup);

    // === 绘制精度控制组 ===
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

    mainLayout->addWidget(precisionGroup);

    // === 鼠标绘制控制组 ===
    QGroupBox* drawGroup = new QGroupBox("鼠标自动绘制", this);
    QVBoxLayout* drawLayout = new QVBoxLayout(drawGroup);

    QHBoxLayout* speedLayout = new QHBoxLayout();
    speedLayout->addWidget(new QLabel("绘制速度:", this));
    drawingSpeedSpin_ = new QDoubleSpinBox(this);
    drawingSpeedSpin_->setRange(0.1, 10.0);
    drawingSpeedSpin_->setValue(1.0);
    drawingSpeedSpin_->setSingleStep(0.1);
    speedLayout->addWidget(drawingSpeedSpin_);
    drawLayout->addLayout(speedLayout);

    QHBoxLayout* speedBtnLayout = new QHBoxLayout();
    speedDownBtn_ = new QPushButton("减速 [-]", this);
    speedUpBtn_ = new QPushButton("加速 [+]", this);
    speedBtnLayout->addWidget(speedDownBtn_);
    speedBtnLayout->addWidget(speedUpBtn_);
    drawLayout->addLayout(speedBtnLayout);

    togglePrecisionBtn_ = new QPushButton("切换精度 [P]", this);
    drawLayout->addWidget(togglePrecisionBtn_);

    cyclePressureBtn_ = new QPushButton("切换压感 [C]", this);
    drawLayout->addWidget(cyclePressureBtn_);

    startDrawBtn_ = new QPushButton("开始绘制", this);
    stopDrawBtn_ = new QPushButton("停止绘制", this);
    drawLayout->addWidget(startDrawBtn_);
    drawLayout->addWidget(stopDrawBtn_);

    mainLayout->addWidget(drawGroup);

    // === 图片转SVG参数组 ===
    QGroupBox* traceGroup = new QGroupBox("图片转SVG参数", this);
    QGridLayout* traceLayout = new QGridLayout(traceGroup);

    traceLayout->addWidget(new QLabel("颜色数量:", this), 0, 0);
    traceColorsSpin_ = new QSpinBox(this);
    traceColorsSpin_->setRange(2, 256);
    traceColorsSpin_->setValue(16);
    traceLayout->addWidget(traceColorsSpin_, 0, 1);

    traceLayout->addWidget(new QLabel("模糊大小:", this), 1, 0);
    traceBlurSpin_ = new QSpinBox(this);
    traceBlurSpin_->setRange(0, 10);
    traceBlurSpin_->setValue(1);
    traceLayout->addWidget(traceBlurSpin_, 1, 1);

    traceLayout->addWidget(new QLabel("曲线阈值:", this), 2, 0);
    traceThresholdSpin_ = new QDoubleSpinBox(this);
    traceThresholdSpin_->setRange(0.1, 10.0);
    traceThresholdSpin_->setValue(1.0);
    traceThresholdSpin_->setSingleStep(0.5);
    traceLayout->addWidget(traceThresholdSpin_, 2, 1);

    traceLayout->addWidget(new QLabel("最小区域:", this), 3, 0);
    traceMinAreaSpin_ = new QSpinBox(this);
    traceMinAreaSpin_->setRange(1, 500);
    traceMinAreaSpin_->setValue(20);
    traceLayout->addWidget(traceMinAreaSpin_, 3, 1);

    traceFillHolesCheck_ = new QCheckBox("填充孔洞", this);
    traceFillHolesCheck_->setChecked(true);
    traceLayout->addWidget(traceFillHolesCheck_, 4, 0, 1, 2);

    traceSmoothPathsCheck_ = new QCheckBox("平滑路径", this);
    traceSmoothPathsCheck_->setChecked(true);
    traceLayout->addWidget(traceSmoothPathsCheck_, 5, 0, 1, 2);

    mainLayout->addWidget(traceGroup);

    // 添加弹性空间
    mainLayout->addStretch();
}

void ControlPanel::connectSignals() {
    // 按钮信号
    connect(importImageBtn_, &QPushButton::clicked, this, &ControlPanel::importImageClicked);
    connect(importSVGBtn_, &QPushButton::clicked, this, &ControlPanel::importSVGClicked);
    connect(exportSVGBtn_, &QPushButton::clicked, this, &ControlPanel::exportSVGClicked);
    connect(traceToSVGBtn_, &QPushButton::clicked, this, &ControlPanel::traceToSVGClicked);
    connect(generateBtn_, &QPushButton::clicked, this, &ControlPanel::generateLineArtClicked);
    connect(generateCannyBtn_, &QPushButton::clicked, this, &ControlPanel::generateCannyClicked);
    connect(deleteBtn_, &QPushButton::clicked, this, &ControlPanel::deleteSelectedClicked);
    connect(clearBtn_, &QPushButton::clicked, this, &ControlPanel::clearAllClicked);
    connect(startDrawBtn_, &QPushButton::clicked, this, &ControlPanel::startDrawingClicked);
    connect(stopDrawBtn_, &QPushButton::clicked, this, &ControlPanel::stopDrawingClicked);
    connect(resetZoomBtn_, &QPushButton::clicked, this, &ControlPanel::resetZoomClicked);
    connect(speedUpBtn_, &QPushButton::clicked, this, &ControlPanel::speedUpClicked);
    connect(speedDownBtn_, &QPushButton::clicked, this, &ControlPanel::speedDownClicked);
    connect(togglePrecisionBtn_, &QPushButton::clicked, this, &ControlPanel::togglePrecisionClicked);
    connect(cyclePressureBtn_, &QPushButton::clicked, this, &ControlPanel::cyclePressureClicked);

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

    // 编辑模式
    connect(editModeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ControlPanel::editModeChanged);
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

} // namespace SketchMaster
