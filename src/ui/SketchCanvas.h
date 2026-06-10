#pragma once

#include <QWidget>
#include <QImage>
#include <QPainter>
#include <QStack>
#include <vector>
#include "../core/PathSimplifier.h"

namespace SketchMaster {

// 编辑模式
enum class EditMode {
    View,       // 查看模式
    Select,     // 选择模式
    Draw,       // 手绘模式
    Erase,      // 擦除模式
    Cut,        // 切割模式
    Composite   // 综合模式（选择+切割+删除）
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

    // 设置编辑模式
    void setEditMode(EditMode mode) { editMode_ = mode; }
    EditMode getEditMode() const { return editMode_; }

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

    // 切割选中的路径（在最近点处一分为二）
    void cutSelectedPolyline(const QPoint& cutPos);

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

signals:
    void polylineSelected(int index);
    void polylineModified();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    QImage backgroundImage_;
    std::vector<Polyline> polylines_;
    EditMode editMode_;
    double backgroundOpacity_;
    QColor lineColor_;
    double zoom_;
    QPoint offset_;

    // 绘制状态
    bool isDrawing_;
    Polyline currentPolyline_;
    int selectedPolylineIndex_;
    QPoint lastMousePos_;

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

    // 坐标转换
    QPoint worldToScreen(const Point2D& pt) const;
    Point2D screenToWorld(const QPoint& pt) const;

    // 查找最近的路径
    int findNearestPolyline(const QPoint& pos, double threshold = 10.0);

    // 查找路径上最近的线段索引和参数t
    void findNearestSegment(const QPoint& pos, int polyIndex, int& segIndex, double& t);

    // 计算点到路径的距离
    double distanceToPolyline(const QPoint& pos, const Polyline& poly);
};

} // namespace SketchMaster
