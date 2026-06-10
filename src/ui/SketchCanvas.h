#pragma once

#include <QWidget>
#include <QImage>
#include <QPainter>
#include <QStack>
#include <QMenu>
#include <vector>
#include "../core/PathSimplifier.h"

namespace SketchMaster {

// 画布编辑状态
enum class CanvasState {
    Idle,           // 空闲（左键点击选中线段，右键弹出菜单）
    DraggingPoint,  // 拖拽线段端点（延长/缩短）
    DraggingLine,   // 拖拽整条线段
    Cutting,        // 切割模式（画直线）
    Drawing         // 手绘模式
};

// 线稿画布：支持可视化编辑
class SketchCanvas : public QWidget {
    Q_OBJECT

public:
    explicit SketchCanvas(QWidget* parent = nullptr);
    ~SketchCanvas();

    // 设置底图
    void setBackgroundImage(const QImage& image);
    void setBackgroundImage(const cv::Mat& mat);

    // 设置线稿路径
    void setPolylines(const std::vector<Polyline>& polylines);
    std::vector<Polyline> getPolylines() const { return polylines_; }

    // 设置底图透明度
    void setBackgroundOpacity(double opacity);

    // 设置线稿颜色
    void setLineColor(const QColor& color) { lineColor_ = color; update(); }

    // 缩放控制
    void setZoom(double zoom);
    double getZoom() const { return zoom_; }
    void resetZoom();

    // 添加新路径
    void addPolyline(const Polyline& poly);

    // 删除选中的路径
    void deleteSelectedPolyline();

    // 清空所有路径
    void clearPolylines();

    // 撤销功能：保存当前状态到撤销栈
    void saveState();

    // 撤销上一步操作
    void undo();

    // 重做（前进）
    void redo();

    // 查询撤销栈是否为空
    bool canUndo() const { return !undoStack_.isEmpty(); }

    // 查询重做栈是否为空
    bool canRedo() const { return !redoStack_.isEmpty(); }

    // 进入切割模式
    void startCutMode();

    // 退出切割模式
    void stopCutMode();

    // 进入手绘模式
    void startDrawMode();

    // 退出手绘模式
    void stopDrawMode();

    // 合并选中线段与相近线段
    void mergeSelectedWithNearby();

    // 智能合并所有相近线段
    void smartMergeAll();

    // 获取当前画布状态
    CanvasState getCanvasState() const { return canvasState_; }

signals:
    void polylineSelected(int index);
    void polylineModified();
    void cutModeChanged(bool active);
    void drawModeChanged(bool active);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    QImage backgroundImage_;
    std::vector<Polyline> polylines_;
    CanvasState canvasState_;
    double backgroundOpacity_;
    QColor lineColor_;
    double zoom_;
    QPoint offset_;

    // 绘制状态
    bool isDrawing_;
    Polyline currentPolyline_;
    int selectedPolylineIndex_;
    QPoint lastMousePos_;

    // 拖拽状态
    int draggingPointIndex_;  // 正在拖拽的端点索引（0=起点，1=终点）
    QPoint dragStartPos_;     // 拖拽开始时的鼠标位置
    std::vector<Point2D> dragOriginalPoints_; // 拖拽开始时的原始点集

    // 切割状态
    QPoint cutLineStart_;
    QPoint cutLineEnd_;

    // 撤销栈（最大20步）
    QStack<QVector<Polyline>> undoStack_;
    QStack<QVector<Polyline>> redoStack_;
    static const int MAX_UNDO_STEPS = 20;

    // 绘制背景
    void drawBackground(QPainter& painter);

    // 绘制线稿
    void drawPolylines(QPainter& painter);

    // 绘制选中高亮
    void drawSelection(QPainter& painter);

    // 绘制切割线
    void drawCutLine(QPainter& painter);

    // 坐标转换
    QPoint worldToScreen(const Point2D& pt) const;
    Point2D screenToWorld(const QPoint& pt) const;

    // 查找最近的路径
    int findNearestPolyline(const QPoint& pos, double threshold = 10.0);

    // 查找路径上最近的线段索引和参数t
    void findNearestSegment(const QPoint& pos, int polyIndex, int& segIndex, double& t);

    // 计算点到路径的距离
    double distanceToPolyline(const QPoint& pos, const Polyline& poly);

    // 查找最近的端点
    int findNearestEndpoint(const QPoint& pos, int polyIndex, double threshold = 10.0);
    // 返回值: 0=起点, 1=终点, -1=没找到

    // 显示右键菜单
    void showContextMenu(const QPoint& pos);

    // 执行切割
    void executeCut();

    // 线段相交检测（世界坐标）
    // 返回true表示两线段相交，intersection为交点
    static bool segmentIntersection(const Point2D& a1, const Point2D& a2,
                                     const Point2D& b1, const Point2D& b2,
                                     Point2D& intersection);
};

} // namespace SketchMaster
