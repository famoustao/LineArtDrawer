#pragma once

#include "PathSimplifier.h"
#include <vector>
#include <string>

namespace SketchMaster {

// SVG导出类
class SVGExporter {
public:
    SVGExporter();
    ~SVGExporter();

    // 导出路径为SVG文件
    bool exportToFile(const std::string& path, 
                      const std::vector<Polyline>& polylines,
                      int width, int height);
    
    // 导出为SVG字符串
    std::string exportToString(const std::vector<Polyline>& polylines,
                               int width, int height);
    
    // 设置描边颜色
    void setStrokeColor(const std::string& color) { strokeColor_ = color; }
    
    // 设置背景颜色
    void setBackgroundColor(const std::string& color) { backgroundColor_ = color; }
    
    // 设置默认线宽
    void setDefaultStrokeWidth(double width) { defaultStrokeWidth_ = width; }

private:
    std::string strokeColor_;
    std::string backgroundColor_;
    double defaultStrokeWidth_;
    
    // 生成SVG路径数据
    std::string generatePathData(const Polyline& poly);
};

} // namespace SketchMaster
