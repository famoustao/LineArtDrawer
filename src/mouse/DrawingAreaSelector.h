#pragma once

#include <QObject>
#include <QPoint>
#include <atomic>

namespace SketchMaster {

// 绘制区域选择器
class DrawingAreaSelector : public QObject {
    Q_OBJECT

public:
    explicit DrawingAreaSelector(QObject* parent = nullptr);
    ~DrawingAreaSelector();

    // 设置左上角
    void setTopLeft(const QPoint& point);
    
    // 设置右下角
    void setBottomRight(const QPoint& point);
    
    // 获取左上角
    QPoint getTopLeft() const { return topLeft_; }
    
    // 获取右下角
    QPoint getBottomRight() const { return bottomRight_; }
    
    // 获取区域尺寸
    int getWidth() const;
    int getHeight() const;
    
    // 检查区域是否有效
    bool isValid() const;
    
    // 重置区域
    void reset();
    
    // 将线稿坐标映射到绘制区域
    QPoint mapPoint(double x, double y, int lineArtWidth, int lineArtHeight) const;

signals:
    void topLeftSet(const QPoint& point);
    void bottomRightSet(const QPoint& point);
    void areaChanged();

private:
    QPoint topLeft_;
    QPoint bottomRight_;
    bool hasTopLeft_;
    bool hasBottomRight_;
};

} // namespace SketchMaster
