#include "SketchCanvas.h"
#include <QPainter>
#include <QMouseEvent>
#include <cmath>
#include <opencv2/imgproc.hpp>

namespace SketchMaster {

SketchCanvas::SketchCanvas(QWidget* parent)
    : QWidget(parent)
    , editMode_(EditMode::View)
    , backgroundOpacity_(0.5)
    , lineColor_(Qt::black)
    , zoom_(1.0)
    , isDrawing_(false)
    , selectedPolylineIndex_(-1) {
    setMinimumSize(400, 300);
    setMouseTracking(true);
}

SketchCanvas::~SketchCanvas() {}

void SketchCanvas::saveState() {
    QVector<Polyline> state(polylines_.begin(), polylines_.end());
    undoStack_.push(state);
    while (undoStack_.size() > MAX_UNDO_STEPS) {
        undoStack_.pop();
    }
    // 新操作后清空重做栈
    redoStack_.clear();
}

void SketchCanvas::undo() {
    if (undoStack_.isEmpty()) return;
    // 保存当前状态到重做栈
    QVector<Polyline> current(polylines_.begin(), polylines_.end());
    redoStack_.push(current);
    // 恢复上一步
    QVector<Polyline> prevState = undoStack_.pop();
    polylines_.assign(prevState.begin(), prevState.end());
    selectedPolylineIndex_ = -1;
    update();
    emit polylineModified();
}

void SketchCanvas::redo() {
    if (redoStack_.isEmpty()) return;
    // 保存当前状态到撤销栈
    QVector<Polyline> current(polylines_.begin(), polylines_.end());
    undoStack_.push(current);
    // 恢复下一步
    QVector<Polyline> nextState = redoStack_.pop();
    polylines_.assign(nextState.begin(), nextState.end());
    selectedPolylineIndex_ = -1;
    update();
    emit polylineModified();
}

void SketchCanvas::setBackgroundImage(const QImage& image) {
    backgroundImage_ = image;
    update();
}

void SketchCanvas::setBackgroundImage(const cv::Mat& mat) {
    if (mat.empty()) return;

    cv::Mat rgb;
    if (mat.channels() == 3) {
        cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
    } else if (mat.channels() == 4) {
        cv::cvtColor(mat, rgb, cv::COLOR_BGRA2RGBA);
    } else if (mat.channels() == 1) {
        cv::cvtColor(mat, rgb, cv::COLOR_GRAY2RGB);
    } else {
        rgb = mat.clone();
    }

    backgroundImage_ = QImage(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888).copy();
    update();
}

void SketchCanvas::setPolylines(const std::vector<Polyline>& polylines) {
    polylines_ = polylines;
    selectedPolylineIndex_ = -1;
    update();
}

void SketchCanvas::setBackgroundOpacity(double opacity) {
    backgroundOpacity_ = std::max(0.0, std::min(1.0, opacity));
    update();
}

void SketchCanvas::setZoom(double zoom) {
    zoom_ = std::max(0.1, std::min(10.0, zoom));
    update();
}

void SketchCanvas::resetZoom() {
    zoom_ = 1.0;
    offset_ = QPoint(0, 0);
    update();
}

void SketchCanvas::addPolyline(const Polyline& poly) {
    saveState(); // 添加线条前保存状态
    polylines_.push_back(poly);
    update();
    emit polylineModified();
}

void SketchCanvas::deleteSelectedPolyline() {
    if (selectedPolylineIndex_ >= 0 && selectedPolylineIndex_ < static_cast<int>(polylines_.size())) {
        saveState(); // 删除前保存状态
        polylines_.erase(polylines_.begin() + selectedPolylineIndex_);
        selectedPolylineIndex_ = -1;
        update();
        emit polylineModified();
    }
}

void SketchCanvas::cutSelectedPolyline(const QPoint& cutPos) {
    if (selectedPolylineIndex_ < 0 || selectedPolylineIndex_ >= static_cast<int>(polylines_.size())) {
        return;
    }

    Polyline& poly = polylines_[selectedPolylineIndex_];
    if (poly.points.size() < 3) return; // 至少需要3个点才能切割

    int segIndex = -1;
    double t = 0;
    findNearestSegment(cutPos, selectedPolylineIndex_, segIndex, t);

    if (segIndex < 0 || segIndex >= static_cast<int>(poly.points.size()) - 1) return;

    // 在切割点插入新点
    Point2D cutPoint(
        poly.points[segIndex].x + t * (poly.points[segIndex + 1].x - poly.points[segIndex].x),
        poly.points[segIndex].y + t * (poly.points[segIndex + 1].y - poly.points[segIndex].y)
    );

    saveState();

    // 将路径在切割点处分成两段
    Polyline poly1, poly2;
    poly1.thickness = poly.thickness;
    poly2.thickness = poly.thickness;

    for (int i = 0; i <= segIndex; ++i) {
        poly1.points.push_back(poly.points[i]);
    }
    poly1.points.push_back(cutPoint);

    poly2.points.push_back(cutPoint);
    for (int i = segIndex + 1; i < static_cast<int>(poly.points.size()); ++i) {
        poly2.points.push_back(poly.points[i]);
    }

    // 替换原始路径为第一段，插入第二段
    polylines_[selectedPolylineIndex_] = poly1;
    polylines_.insert(polylines_.begin() + selectedPolylineIndex_ + 1, poly2);

    update();
    emit polylineModified();
}

void SketchCanvas::clearPolylines() {
    saveState(); // 清空前保存状态
    polylines_.clear();
    selectedPolylineIndex_ = -1;
    update();
    emit polylineModified();
}

void SketchCanvas::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制背景
    drawBackground(painter);

    // 绘制线稿
    drawPolylines(painter);

    // 绘制选中高亮
    drawSelection(painter);

    // 绘制当前正在绘制的路径
    if (isDrawing_ && currentPolyline_.points.size() > 1) {
        painter.setPen(QPen(Qt::blue, 2.0 / zoom_, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        QPolygonF poly;
        for (const auto& pt : currentPolyline_.points) {
            poly << QPointF(pt.x, pt.y);
        }
        painter.drawPolyline(poly);
    }
}

void SketchCanvas::drawBackground(QPainter& painter) {
    if (backgroundImage_.isNull()) {
        painter.fillRect(rect(), Qt::white);
        return;
    }

    painter.save();
    painter.setOpacity(backgroundOpacity_);

    // 计算绘制区域
    QRect targetRect = QRect(offset_.x(), offset_.y(),
                            static_cast<int>(backgroundImage_.width() * zoom_),
                            static_cast<int>(backgroundImage_.height() * zoom_));

    painter.drawImage(targetRect, backgroundImage_);
    painter.restore();
}

void SketchCanvas::drawPolylines(QPainter& painter) {
    painter.save();
    painter.translate(offset_);
    painter.scale(zoom_, zoom_);

    for (const auto& poly : polylines_) {
        if (poly.points.size() < 2) continue;

        double thickness = poly.thickness > 0 ? poly.thickness : 1.0;
        painter.setPen(QPen(lineColor_, thickness, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

        QPolygonF polygon;
        for (const auto& pt : poly.points) {
            polygon << QPointF(pt.x, pt.y);
        }
        painter.drawPolyline(polygon);
    }

    painter.restore();
}

void SketchCanvas::drawSelection(QPainter& painter) {
    if (selectedPolylineIndex_ < 0 || selectedPolylineIndex_ >= static_cast<int>(polylines_.size())) {
        return;
    }

    const auto& poly = polylines_[selectedPolylineIndex_];
    if (poly.points.size() < 2) return;

    painter.save();
    painter.translate(offset_);
    painter.scale(zoom_, zoom_);

    // 绘制高亮路径
    double thickness = (poly.thickness > 0 ? poly.thickness : 1.0) + 2.0;
    painter.setPen(QPen(Qt::red, thickness, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

    QPolygonF polygon;
    for (const auto& pt : poly.points) {
        polygon << QPointF(pt.x, pt.y);
    }
    painter.drawPolyline(polygon);

    // 绘制端点
    painter.setBrush(Qt::red);
    for (const auto& pt : poly.points) {
        painter.drawEllipse(QPointF(pt.x, pt.y), 3.0, 3.0);
    }

    painter.restore();
}

QPoint SketchCanvas::worldToScreen(const Point2D& pt) const {
    return QPoint(
        static_cast<int>(pt.x * zoom_ + offset_.x()),
        static_cast<int>(pt.y * zoom_ + offset_.y())
    );
}

Point2D SketchCanvas::screenToWorld(const QPoint& pt) const {
    return Point2D(
        (pt.x() - offset_.x()) / zoom_,
        (pt.y() - offset_.y()) / zoom_
    );
}

int SketchCanvas::findNearestPolyline(const QPoint& pos, double threshold) {
    int nearestIndex = -1;
    double minDist = threshold;

    for (size_t i = 0; i < polylines_.size(); ++i) {
        double dist = distanceToPolyline(pos, polylines_[i]);
        if (dist < minDist) {
            minDist = dist;
            nearestIndex = static_cast<int>(i);
        }
    }

    return nearestIndex;
}

void SketchCanvas::findNearestSegment(const QPoint& pos, int polyIndex, int& segIndex, double& t) {
    segIndex = -1;
    t = 0;
    if (polyIndex < 0 || polyIndex >= static_cast<int>(polylines_.size())) return;

    const auto& poly = polylines_[polyIndex];
    double minDist = std::numeric_limits<double>::max();

    for (size_t i = 0; i < poly.points.size() - 1; ++i) {
        QPoint p1 = worldToScreen(poly.points[i]);
        QPoint p2 = worldToScreen(poly.points[i + 1]);

        double dx = p2.x() - p1.x();
        double dy = p2.y() - p1.y();
        double lenSq = dx * dx + dy * dy;

        double segT;
        if (lenSq == 0) {
            segT = 0;
        } else {
            segT = std::max(0.0, std::min(1.0,
                ((pos.x() - p1.x()) * dx + (pos.y() - p1.y()) * dy) / lenSq));
        }

        double projX = p1.x() + segT * dx;
        double projY = p1.y() + segT * dy;

        double dist = std::sqrt(
            (pos.x() - projX) * (pos.x() - projX) +
            (pos.y() - projY) * (pos.y() - projY)
        );

        if (dist < minDist) {
            minDist = dist;
            segIndex = static_cast<int>(i);
            t = segT;
        }
    }
}

double SketchCanvas::distanceToPolyline(const QPoint& pos, const Polyline& poly) {
    if (poly.points.size() < 2) return std::numeric_limits<double>::max();

    double minDist = std::numeric_limits<double>::max();

    for (size_t i = 0; i < poly.points.size() - 1; ++i) {
        QPoint p1 = worldToScreen(poly.points[i]);
        QPoint p2 = worldToScreen(poly.points[i + 1]);

        // 计算点到线段的距离
        double dx = p2.x() - p1.x();
        double dy = p2.y() - p1.y();
        double lenSq = dx * dx + dy * dy;

        double t;
        if (lenSq == 0) {
            t = 0;
        } else {
            t = std::max(0.0, std::min(1.0,
                ((pos.x() - p1.x()) * dx + (pos.y() - p1.y()) * dy) / lenSq));
        }

        double projX = p1.x() + t * dx;
        double projY = p1.y() + t * dy;

        double dist = std::sqrt(
            (pos.x() - projX) * (pos.x() - projX) +
            (pos.y() - projY) * (pos.y() - projY)
        );

        minDist = std::min(minDist, dist);
    }

    return minDist;
}

void SketchCanvas::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        switch (editMode_) {
            case EditMode::Select: {
                int index = findNearestPolyline(event->pos());
                selectedPolylineIndex_ = index;
                emit polylineSelected(index);
                update();
                break;
            }
            case EditMode::Draw: {
                isDrawing_ = true;
                currentPolyline_ = Polyline();
                currentPolyline_.points.push_back(screenToWorld(event->pos()));
                break;
            }
            case EditMode::Erase: {
                int index = findNearestPolyline(event->pos());
                if (index >= 0) {
                    saveState(); // 擦除前保存状态
                    polylines_.erase(polylines_.begin() + index);
                    emit polylineModified();
                    update();
                }
                break;
            }
            case EditMode::Cut: {
                // 先选中最近的路径，再在点击位置切割
                int index = findNearestPolyline(event->pos());
                if (index >= 0) {
                    selectedPolylineIndex_ = index;
                    cutSelectedPolyline(event->pos());
                }
                break;
            }
            case EditMode::Composite: {
                // 综合模式：左键选择/切割，右键删除
                if (event->button() == Qt::LeftButton) {
                    int index = findNearestPolyline(event->pos());
                    if (index >= 0) {
                        // 如果点击的是已选中的线段，则切割
                        if (index == selectedPolylineIndex_) {
                            cutSelectedPolyline(event->pos());
                        } else {
                            // 否则选中
                            selectedPolylineIndex_ = index;
                        }
                        emit polylineSelected(selectedPolylineIndex_);
                        update();
                    } else {
                        selectedPolylineIndex_ = -1;
                        update();
                    }
                } else if (event->button() == Qt::RightButton) {
                    // 右键删除选中的线段
                    if (selectedPolylineIndex_ >= 0) {
                        saveState();
                        polylines_.erase(polylines_.begin() + selectedPolylineIndex_);
                        selectedPolylineIndex_ = -1;
                        emit polylineModified();
                        update();
                    }
                }
                break;
            }
            default:
                break;
        }
    }

    lastMousePos_ = event->pos();
}

void SketchCanvas::mouseMoveEvent(QMouseEvent* event) {
    if (editMode_ == EditMode::Draw && isDrawing_) {
        currentPolyline_.points.push_back(screenToWorld(event->pos()));
        update();
    }

    lastMousePos_ = event->pos();
}

void SketchCanvas::mouseReleaseEvent(QMouseEvent* event) {
    if (editMode_ == EditMode::Draw && isDrawing_ && event->button() == Qt::LeftButton) {
        isDrawing_ = false;
        if (currentPolyline_.points.size() >= 2) {
            saveState(); // 手绘完成前保存状态
            polylines_.push_back(currentPolyline_);
            emit polylineModified();
        }
        currentPolyline_ = Polyline();
        update();
    }
}

void SketchCanvas::wheelEvent(QWheelEvent* event) {
    double delta = event->angleDelta().y() / 120.0;
    double newZoom = zoom_ * std::pow(1.1, delta);
    setZoom(newZoom);
}

} // namespace SketchMaster
