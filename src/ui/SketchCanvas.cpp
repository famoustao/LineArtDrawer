#include "SketchCanvas.h"
#include <QPainter>
#include <QMouseEvent>
#include <QMenu>
#include <cmath>
#include <algorithm>
#include <opencv2/imgproc.hpp>

namespace SketchMaster {

SketchCanvas::SketchCanvas(QWidget* parent)
    : QWidget(parent)
    , canvasState_(CanvasState::Idle)
    , backgroundOpacity_(0.5)
    , lineColor_(Qt::black)
    , zoom_(1.0)
    , isDrawing_(false)
    , selectedPolylineIndex_(-1)
    , draggingPointIndex_(-1) {
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
    saveState();
    polylines_.push_back(poly);
    update();
    emit polylineModified();
}

void SketchCanvas::deleteSelectedPolyline() {
    if (selectedPolylineIndex_ >= 0 && selectedPolylineIndex_ < static_cast<int>(polylines_.size())) {
        saveState();
        polylines_.erase(polylines_.begin() + selectedPolylineIndex_);
        selectedPolylineIndex_ = -1;
        update();
        emit polylineModified();
    }
}

void SketchCanvas::clearPolylines() {
    saveState();
    polylines_.clear();
    selectedPolylineIndex_ = -1;
    update();
    emit polylineModified();
}

// === 模式切换 ===

void SketchCanvas::startCutMode() {
    canvasState_ = CanvasState::Cutting;
    cutLineStart_ = QPoint();
    cutLineEnd_ = QPoint();
    selectedPolylineIndex_ = -1;
    update();
    emit cutModeChanged(true);
}

void SketchCanvas::stopCutMode() {
    canvasState_ = CanvasState::Idle;
    cutLineStart_ = QPoint();
    cutLineEnd_ = QPoint();
    update();
    emit cutModeChanged(false);
}

void SketchCanvas::startDrawMode() {
    canvasState_ = CanvasState::Drawing;
    isDrawing_ = false;
    selectedPolylineIndex_ = -1;
    update();
    emit drawModeChanged(true);
}

void SketchCanvas::stopDrawMode() {
    canvasState_ = CanvasState::Idle;
    isDrawing_ = false;
    currentPolyline_ = Polyline();
    update();
    emit drawModeChanged(false);
}

// === 合并逻辑 ===

void SketchCanvas::mergeSelectedWithNearby() {
    if (selectedPolylineIndex_ < 0 || selectedPolylineIndex_ >= static_cast<int>(polylines_.size())) return;

    saveState();

    double mergeThreshold = 10.0 / zoom_; // 合并阈值（世界坐标）

    // 先检查自身闭合（起点终点距离近）
    Polyline& selectedPoly = polylines_[selectedPolylineIndex_];
    if (selectedPoly.points.size() >= 3) {
        Point2D& startPt = selectedPoly.points.front();
        Point2D& endPt = selectedPoly.points.back();
        double dist = std::sqrt((startPt.x - endPt.x) * (startPt.x - endPt.x) +
                                (startPt.y - endPt.y) * (startPt.y - endPt.y));
        if (dist < mergeThreshold) {
            // 闭合：将终点连接到起点
            selectedPoly.points.push_back(startPt);
            update();
            emit polylineModified();
            return;
        }
    }

    // 查找与选中线段端点距离小于阈值的线段
    Point2D selStart = selectedPoly.points.front();
    Point2D selEnd = selectedPoly.points.back();

    int bestIndex = -1;
    double bestDist = mergeThreshold;
    bool connectToEnd = false; // true=连接到选中线段的终点, false=连接到起点
    bool otherConnectStart = false; // true=连接到另一条线的起点, false=终点

    for (size_t i = 0; i < polylines_.size(); ++i) {
        if (static_cast<int>(i) == selectedPolylineIndex_) continue;
        const auto& poly = polylines_[i];
        if (poly.points.empty()) continue;

        // 检查选中线段起点与另一条线段终点的距离
        Point2D otherEnd = poly.points.back();
        double d1 = std::sqrt((selStart.x - otherEnd.x) * (selStart.x - otherEnd.x) +
                               (selStart.y - otherEnd.y) * (selStart.y - otherEnd.y));
        if (d1 < bestDist) {
            bestDist = d1;
            bestIndex = static_cast<int>(i);
            connectToEnd = false;  // 连接到选中线段的起点
            otherConnectStart = false; // 另一条线段的终点
        }

        // 检查选中线段起点与另一条线段起点的距离
        Point2D otherStart = poly.points.front();
        double d2 = std::sqrt((selStart.x - otherStart.x) * (selStart.x - otherStart.x) +
                               (selStart.y - otherStart.y) * (selStart.y - otherStart.y));
        if (d2 < bestDist) {
            bestDist = d2;
            bestIndex = static_cast<int>(i);
            connectToEnd = false;
            otherConnectStart = true;
        }

        // 检查选中线段终点与另一条线段起点的距离
        double d3 = std::sqrt((selEnd.x - otherStart.x) * (selEnd.x - otherStart.x) +
                               (selEnd.y - otherStart.y) * (selEnd.y - otherStart.y));
        if (d3 < bestDist) {
            bestDist = d3;
            bestIndex = static_cast<int>(i);
            connectToEnd = true;   // 连接到选中线段的终点
            otherConnectStart = true;
        }

        // 检查选中线段终点与另一条线段终点的距离
        double d4 = std::sqrt((selEnd.x - otherEnd.x) * (selEnd.x - otherEnd.x) +
                               (selEnd.y - otherEnd.y) * (selEnd.y - otherEnd.y));
        if (d4 < bestDist) {
            bestDist = d4;
            bestIndex = static_cast<int>(i);
            connectToEnd = true;
            otherConnectStart = false;
        }
    }

    if (bestIndex >= 0) {
        Polyline& otherPoly = polylines_[bestIndex];
        Polyline merged;
        merged.thickness = selectedPoly.thickness;

        if (connectToEnd && otherConnectStart) {
            // 选中线段终点 + 另一条线段起点
            merged.points = selectedPoly.points;
            for (const auto& pt : otherPoly.points) {
                merged.points.push_back(pt);
            }
        } else if (connectToEnd && !otherConnectStart) {
            // 选中线段终点 + 另一条线段终点（反转另一条）
            merged.points = selectedPoly.points;
            for (auto it = otherPoly.points.rbegin(); it != otherPoly.points.rend(); ++it) {
                merged.points.push_back(*it);
            }
        } else if (!connectToEnd && otherConnectStart) {
            // 选中线段起点 + 另一条线段起点（反转选中线段）
            for (auto it = selectedPoly.points.rbegin(); it != selectedPoly.points.rend(); ++it) {
                merged.points.push_back(*it);
            }
            for (const auto& pt : otherPoly.points) {
                merged.points.push_back(pt);
            }
        } else {
            // 选中线段起点 + 另一条线段终点
            for (const auto& pt : otherPoly.points) {
                merged.points.push_back(pt);
            }
            for (const auto& pt : selectedPoly.points) {
                merged.points.push_back(pt);
            }
        }

        // 替换：删除两条旧线段，插入合并后的
        int minIdx = std::min(selectedPolylineIndex_, bestIndex);
        int maxIdx = std::max(selectedPolylineIndex_, bestIndex);
        polylines_.erase(polylines_.begin() + maxIdx);
        polylines_.erase(polylines_.begin() + minIdx);
        polylines_.insert(polylines_.begin() + minIdx, merged);
        selectedPolylineIndex_ = minIdx;
    }

    update();
    emit polylineModified();
}

void SketchCanvas::smartMergeAll() {
    saveState();

    double mergeThreshold = 10.0 / zoom_;
    bool merged = true;

    while (merged) {
        merged = false;

        // 先检查自身闭合
        for (size_t i = 0; i < polylines_.size(); ++i) {
            auto& poly = polylines_[i];
            if (poly.points.size() >= 3) {
                Point2D& startPt = poly.points.front();
                Point2D& endPt = poly.points.back();
                double dist = std::sqrt((startPt.x - endPt.x) * (startPt.x - endPt.x) +
                                        (startPt.y - endPt.y) * (startPt.y - endPt.y));
                if (dist < mergeThreshold) {
                    poly.points.push_back(startPt);
                    merged = true;
                }
            }
        }

        // 遍历所有线段对
        for (size_t i = 0; i < polylines_.size() && !merged; ++i) {
            for (size_t j = i + 1; j < polylines_.size() && !merged; ++j) {
                const auto& polyA = polylines_[i];
                const auto& polyB = polylines_[j];
                if (polyA.points.empty() || polyB.points.empty()) continue;

                Point2D aStart = polyA.points.front();
                Point2D aEnd = polyA.points.back();
                Point2D bStart = polyB.points.front();
                Point2D bEnd = polyB.points.back();

                // 检查四种连接方式
                struct ConnInfo {
                    double dist;
                    bool reverseA, reverseB;
                };

                ConnInfo connections[] = {
                    {std::sqrt((aEnd.x - bStart.x) * (aEnd.x - bStart.x) +
                               (aEnd.y - bStart.y) * (aEnd.y - bStart.y)), false, false},  // A尾-B头
                    {std::sqrt((aEnd.x - bEnd.x) * (aEnd.x - bEnd.x) +
                               (aEnd.y - bEnd.y) * (aEnd.y - bEnd.y)), false, true},         // A尾-B尾
                    {std::sqrt((aStart.x - bStart.x) * (aStart.x - bStart.x) +
                               (aStart.y - bStart.y) * (aStart.y - bStart.y)), true, false},  // A头-B头
                    {std::sqrt((aStart.x - bEnd.x) * (aStart.x - bEnd.x) +
                               (aStart.y - bEnd.y) * (aStart.y - bEnd.y)), true, true}         // A头-B尾
                };

                int bestConn = -1;
                double bestDist = mergeThreshold;
                for (int c = 0; c < 4; ++c) {
                    if (connections[c].dist < bestDist) {
                        bestDist = connections[c].dist;
                        bestConn = c;
                    }
                }

                if (bestConn >= 0) {
                    Polyline mergedPoly;
                    mergedPoly.thickness = polyA.thickness;

                    auto getPoints = [](const Polyline& p, bool reverse) -> std::vector<Point2D> {
                        if (reverse) {
                            std::vector<Point2D> rev(p.points.rbegin(), p.points.rend());
                            return rev;
                        }
                        return p.points;
                    };

                    auto ptsA = getPoints(polyA, connections[bestConn].reverseA);
                    auto ptsB = getPoints(polyB, connections[bestConn].reverseB);

                    mergedPoly.points = ptsA;
                    mergedPoly.points.insert(mergedPoly.points.end(), ptsB.begin(), ptsB.end());

                    // 替换
                    polylines_.erase(polylines_.begin() + j);
                    polylines_.erase(polylines_.begin() + i);
                    polylines_.insert(polylines_.begin() + i, mergedPoly);
                    merged = true;
                    break;
                }
            }
        }
    }

    selectedPolylineIndex_ = -1;
    update();
    emit polylineModified();
}

// === 端点查找 ===

int SketchCanvas::findNearestEndpoint(const QPoint& pos, int polyIndex, double threshold) {
    if (polyIndex < 0 || polyIndex >= static_cast<int>(polylines_.size())) return -1;

    const auto& poly = polylines_[polyIndex];
    if (poly.points.empty()) return -1;

    QPoint startScreen = worldToScreen(poly.points.front());
    QPoint endScreen = worldToScreen(poly.points.back());

    double distToStart = std::sqrt(std::pow(pos.x() - startScreen.x(), 2) +
                                     std::pow(pos.y() - startScreen.y(), 2));
    double distToEnd = std::sqrt(std::pow(pos.x() - endScreen.x(), 2) +
                                   std::pow(pos.y() - endScreen.y(), 2));

    if (distToStart < threshold && distToStart <= distToEnd) return 0; // 起点
    if (distToEnd < threshold) return 1; // 终点
    return -1;
}

// === 右键菜单 ===

void SketchCanvas::showContextMenu(const QPoint& pos) {
    QMenu menu(this);
    menu.addAction("删除选中线段", this, [this]() {
        if (selectedPolylineIndex_ >= 0) {
            saveState();
            polylines_.erase(polylines_.begin() + selectedPolylineIndex_);
            selectedPolylineIndex_ = -1;
            update();
            emit polylineModified();
        }
    });
    menu.addAction("合并相近线段", this, [this]() {
        mergeSelectedWithNearby();
    });
    menu.exec(mapToGlobal(pos));
}

// === 切割逻辑 ===

bool SketchCanvas::segmentIntersection(const Point2D& a1, const Point2D& a2,
                                        const Point2D& b1, const Point2D& b2,
                                        Point2D& intersection) {
    double x1 = a1.x, y1 = a1.y;
    double x2 = a2.x, y2 = a2.y;
    double x3 = b1.x, y3 = b1.y;
    double x4 = b2.x, y4 = b2.y;

    double denom = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
    if (std::abs(denom) < 1e-10) return false; // 平行或共线

    double t = ((x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4)) / denom;
    double u = -((x1 - x2) * (y1 - y3) - (y1 - y2) * (x1 - x3)) / denom;

    if (t >= 0.0 && t <= 1.0 && u >= 0.0 && u <= 1.0) {
        intersection.x = x1 + t * (x2 - x1);
        intersection.y = y1 + t * (y2 - y1);
        return true;
    }
    return false;
}

void SketchCanvas::executeCut() {
    // 将屏幕坐标的切割线转换为世界坐标
    Point2D cutStart = screenToWorld(cutLineStart_);
    Point2D cutEnd = screenToWorld(cutLineEnd_);

    // 收集所有需要切割的信息：{polyIndex, segIndex, intersectionPoint}
    struct CutInfo {
        int polyIndex;
        int segIndex;
        Point2D intersection;
    };
    std::vector<CutInfo> cuts;

    for (size_t pi = 0; pi < polylines_.size(); ++pi) {
        const auto& poly = polylines_[pi];
        for (size_t si = 0; si + 1 < poly.points.size(); ++si) {
            Point2D intersection;
            if (segmentIntersection(cutStart, cutEnd,
                                     poly.points[si], poly.points[si + 1],
                                     intersection)) {
                cuts.push_back({static_cast<int>(pi), static_cast<int>(si), intersection});
            }
        }
    }

    if (cuts.empty()) return;

    saveState();

    // 按polyIndex和segIndex排序（从后往前处理，避免索引偏移）
    std::sort(cuts.begin(), cuts.end(), [](const CutInfo& a, const CutInfo& b) {
        if (a.polyIndex != b.polyIndex) return a.polyIndex > b.polyIndex;
        return a.segIndex > b.segIndex;
    });

    // 处理每条线段的切割
    // 注意：同一条polyline可能有多个切割点
    std::map<int, std::vector<std::pair<int, Point2D>>> cutsByPoly;
    for (const auto& c : cuts) {
        cutsByPoly[c.polyIndex].push_back({c.segIndex, c.intersection});
    }

    // 对每条被切割的polyline进行处理
    // 收集所有新的polyline，然后替换
    std::vector<Polyline> newPolylines;

    for (size_t pi = 0; pi < polylines_.size(); ++pi) {
        auto it = cutsByPoly.find(static_cast<int>(pi));
        if (it == cutsByPoly.end()) {
            // 没有被切割，保留原样
            newPolylines.push_back(polylines_[pi]);
            continue;
        }

        // 按segIndex排序
        auto& polyCuts = it->second;
        std::sort(polyCuts.begin(), polyCuts.end(),
                  [](const auto& a, const auto& b) { return a.first < b.first; });

        const auto& poly = polylines_[pi];
        int currentSeg = 0;
        Polyline currentPart;
        currentPart.thickness = poly.thickness;
        currentPart.points.push_back(poly.points[0]);

        for (const auto& [segIdx, interPt] : polyCuts) {
            // 添加从当前段到切割点的所有点
            for (int s = currentSeg; s < segIdx; ++s) {
                currentPart.points.push_back(poly.points[s + 1]);
            }
            // 添加交点
            currentPart.points.push_back(interPt);

            if (currentPart.points.size() >= 2) {
                newPolylines.push_back(currentPart);
            }

            // 开始新的一段
            currentPart = Polyline();
            currentPart.thickness = poly.thickness;
            currentPart.points.push_back(interPt);
            currentSeg = segIdx + 1;
        }

        // 添加剩余的点
        for (size_t s = currentSeg; s + 1 < poly.points.size(); ++s) {
            currentPart.points.push_back(poly.points[s + 1]);
        }
        if (currentPart.points.size() >= 2) {
            newPolylines.push_back(currentPart);
        }
    }

    polylines_ = newPolylines;
    selectedPolylineIndex_ = -1;
    update();
    emit polylineModified();
}

// === 绘制事件 ===

void SketchCanvas::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制背景
    drawBackground(painter);

    // 绘制线稿
    drawPolylines(painter);

    // 绘制选中高亮
    drawSelection(painter);

    // 绘制切割线
    if (canvasState_ == CanvasState::Cutting && !cutLineStart_.isNull() && !cutLineEnd_.isNull()) {
        drawCutLine(painter);
    }

    // 绘制当前正在手绘的路径
    if (canvasState_ == CanvasState::Drawing && isDrawing_ && currentPolyline_.points.size() > 1) {
        painter.save();
        painter.translate(offset_);
        painter.scale(zoom_, zoom_);
        painter.setPen(QPen(Qt::blue, 2.0 / zoom_, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        QPolygonF poly;
        for (const auto& pt : currentPolyline_.points) {
            poly << QPointF(pt.x, pt.y);
        }
        painter.drawPolyline(poly);
        painter.restore();
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

    // 特别标记起点和终点（用更大的圆）
    painter.setBrush(Qt::green);
    painter.drawEllipse(QPointF(poly.points.front().x, poly.points.front().y), 5.0, 5.0);
    painter.setBrush(Qt::blue);
    painter.drawEllipse(QPointF(poly.points.back().x, poly.points.back().y), 5.0, 5.0);

    painter.restore();
}

void SketchCanvas::drawCutLine(QPainter& painter) {
    painter.save();
    QPen cutPen(Qt::red, 2.0, Qt::DashLine);
    painter.setPen(cutPen);
    painter.drawLine(cutLineStart_, cutLineEnd_);

    // 绘制端点标记
    painter.setBrush(Qt::red);
    painter.drawEllipse(cutLineStart_, 4, 4);
    painter.drawEllipse(cutLineEnd_, 4, 4);

    painter.restore();
}

// === 坐标转换 ===

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

// === 查找方法 ===

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

// === 鼠标事件 ===

void SketchCanvas::mousePressEvent(QMouseEvent* event) {
    if (canvasState_ == CanvasState::Cutting) {
        if (event->button() == Qt::LeftButton) {
            // 左键按下：记录切割线起点
            cutLineStart_ = event->pos();
            cutLineEnd_ = event->pos();
            update();
        } else if (event->button() == Qt::RightButton) {
            // 右键按下：退出切割模式
            stopCutMode();
        }
        lastMousePos_ = event->pos();
        return;
    }

    if (canvasState_ == CanvasState::Drawing) {
        if (event->button() == Qt::LeftButton) {
            // 左键按下：开始手绘
            isDrawing_ = true;
            currentPolyline_ = Polyline();
            currentPolyline_.points.push_back(screenToWorld(event->pos()));
            update();
        }
        lastMousePos_ = event->pos();
        return;
    }

    // Idle 状态
    if (canvasState_ == CanvasState::Idle) {
        if (event->button() == Qt::LeftButton) {
            // 先检查是否点击了某条线段的端点
            int polyIdx = findNearestPolyline(event->pos());
            if (polyIdx >= 0) {
                int endpointIdx = findNearestEndpoint(event->pos(), polyIdx);
                if (endpointIdx >= 0) {
                    // 点击了端点，进入 DraggingPoint 状态
                    canvasState_ = CanvasState::DraggingPoint;
                    selectedPolylineIndex_ = polyIdx;
                    draggingPointIndex_ = endpointIdx;
                    dragStartPos_ = event->pos();
                    dragOriginalPoints_ = polylines_[polyIdx].points;
                    emit polylineSelected(polyIdx);
                    update();
                    lastMousePos_ = event->pos();
                    return;
                }

                // 点击了线段中间，进入 DraggingLine 状态
                canvasState_ = CanvasState::DraggingLine;
                selectedPolylineIndex_ = polyIdx;
                dragStartPos_ = event->pos();
                dragOriginalPoints_ = polylines_[polyIdx].points;
                emit polylineSelected(polyIdx);
                update();
                lastMousePos_ = event->pos();
                return;
            }

            // 没有点击任何线段，取消选中
            selectedPolylineIndex_ = -1;
            update();
        } else if (event->button() == Qt::RightButton) {
            // 右键弹出菜单
            if (selectedPolylineIndex_ >= 0) {
                showContextMenu(event->pos());
            }
        }
    }

    lastMousePos_ = event->pos();
}

void SketchCanvas::mouseMoveEvent(QMouseEvent* event) {
    if (canvasState_ == CanvasState::DraggingPoint) {
        // 拖拽端点
        if (selectedPolylineIndex_ >= 0 && selectedPolylineIndex_ < static_cast<int>(polylines_.size())) {
            Point2D newWorldPos = screenToWorld(event->pos());
            if (draggingPointIndex_ == 0) {
                polylines_[selectedPolylineIndex_].points.front() = newWorldPos;
            } else if (draggingPointIndex_ == 1) {
                polylines_[selectedPolylineIndex_].points.back() = newWorldPos;
            }
            update();
        }
    } else if (canvasState_ == CanvasState::DraggingLine) {
        // 拖拽整条线段
        if (selectedPolylineIndex_ >= 0 && selectedPolylineIndex_ < static_cast<int>(polylines_.size())) {
            // 计算鼠标偏移量（世界坐标）
            Point2D newWorldPos = screenToWorld(event->pos());
            Point2D oldWorldPos = screenToWorld(lastMousePos_);
            double dx = newWorldPos.x - oldWorldPos.x;
            double dy = newWorldPos.y - oldWorldPos.y;

            // 移动所有点
            for (auto& pt : polylines_[selectedPolylineIndex_].points) {
                pt.x += dx;
                pt.y += dy;
            }
            update();
        }
    } else if (canvasState_ == CanvasState::Cutting) {
        // 更新切割线终点
        if (event->buttons() & Qt::LeftButton) {
            cutLineEnd_ = event->pos();
            update();
        }
    } else if (canvasState_ == CanvasState::Drawing && isDrawing_) {
        // 手绘中，添加点
        currentPolyline_.points.push_back(screenToWorld(event->pos()));
        update();
    }

    lastMousePos_ = event->pos();
}

void SketchCanvas::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() != Qt::LeftButton) {
        lastMousePos_ = event->pos();
        return;
    }

    if (canvasState_ == CanvasState::DraggingPoint) {
        // 结束端点拖拽
        saveState();
        canvasState_ = CanvasState::Idle;
        draggingPointIndex_ = -1;
        update();
        emit polylineModified();
    } else if (canvasState_ == CanvasState::DraggingLine) {
        // 结束线段拖拽
        saveState();
        canvasState_ = CanvasState::Idle;
        update();
        emit polylineModified();
    } else if (canvasState_ == CanvasState::Cutting) {
        // 执行切割
        if (!cutLineStart_.isNull() && !cutLineEnd_.isNull()) {
            executeCut();
        }
        // 切割后保持切割模式，可以继续切割
        cutLineStart_ = QPoint();
        cutLineEnd_ = QPoint();
        update();
    } else if (canvasState_ == CanvasState::Drawing && isDrawing_) {
        // 结束手绘
        isDrawing_ = false;
        if (currentPolyline_.points.size() >= 2) {
            saveState();
            polylines_.push_back(currentPolyline_);
            emit polylineModified();
        }
        currentPolyline_ = Polyline();
        update();
    }

    lastMousePos_ = event->pos();
}

void SketchCanvas::wheelEvent(QWheelEvent* event) {
    double delta = event->angleDelta().y() / 120.0;
    double newZoom = zoom_ * std::pow(1.1, delta);
    setZoom(newZoom);
}

} // namespace SketchMaster
