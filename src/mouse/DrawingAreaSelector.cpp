#include "DrawingAreaSelector.h"
#include <algorithm>

namespace SketchMaster {

DrawingAreaSelector::DrawingAreaSelector(QObject* parent)
    : QObject(parent)
    , topLeft_(0, 0)
    , bottomRight_(0, 0)
    , hasTopLeft_(false)
    , hasBottomRight_(false) {}

DrawingAreaSelector::~DrawingAreaSelector() {}

void DrawingAreaSelector::setTopLeft(const QPoint& point) {
    topLeft_ = point;
    hasTopLeft_ = true;
    emit topLeftSet(point);
    emit areaChanged();
}

void DrawingAreaSelector::setBottomRight(const QPoint& point) {
    bottomRight_ = point;
    hasBottomRight_ = true;
    emit bottomRightSet(point);
    emit areaChanged();
}

int DrawingAreaSelector::getWidth() const {
    if (!isValid()) return 0;
    return std::abs(bottomRight_.x() - topLeft_.x());
}

int DrawingAreaSelector::getHeight() const {
    if (!isValid()) return 0;
    return std::abs(bottomRight_.y() - topLeft_.y());
}

bool DrawingAreaSelector::isValid() const {
    return hasTopLeft_ && hasBottomRight_;
}

void DrawingAreaSelector::reset() {
    topLeft_ = QPoint(0, 0);
    bottomRight_ = QPoint(0, 0);
    hasTopLeft_ = false;
    hasBottomRight_ = false;
    emit areaChanged();
}

QPoint DrawingAreaSelector::mapPoint(double x, double y, int lineArtWidth, int lineArtHeight) const {
    if (!isValid() || lineArtWidth <= 0 || lineArtHeight <= 0) {
        return QPoint(static_cast<int>(x), static_cast<int>(y));
    }

    int areaWidth = getWidth();
    int areaHeight = getHeight();

    // 计算缩放比例（保持宽高比）
    double scaleX = areaWidth / static_cast<double>(lineArtWidth);
    double scaleY = areaHeight / static_cast<double>(lineArtHeight);
    double scale = std::min(scaleX, scaleY);

    // 计算居中偏移
    int scaledWidth = static_cast<int>(lineArtWidth * scale);
    int scaledHeight = static_cast<int>(lineArtHeight * scale);
    int offsetX = (areaWidth - scaledWidth) / 2;
    int offsetY = (areaHeight - scaledHeight) / 2;

    int mappedX = topLeft_.x() + offsetX + static_cast<int>(x * scale);
    int mappedY = topLeft_.y() + offsetY + static_cast<int>(y * scale);

    return QPoint(mappedX, mappedY);
}

} // namespace SketchMaster
