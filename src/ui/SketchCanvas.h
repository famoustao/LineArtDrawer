#pragma once

#include <QWidget>
#include <QImage>
#include <QPainter>
#include <vector>
#include "../core/PathSimplifier.h"

namespace SketchMaster {

// 编辑模式
enum class EditMode {
    View,       // 查看模式
    Select,     // 选择模式
    Draw,       // 手绘模式
    Erase       // 擦除模式
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
    
    // 清空所有路径
    void clearPolylines();

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
    
    // 计算点到路径的距离
    double distanceToPolyline(const QPoint& pos, const Polyline& poly);
};

} // namespace SketchMaster
