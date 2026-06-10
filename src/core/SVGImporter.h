#pragma once

#include "PathSimplifier.h"
#include <vector>
#include <string>

namespace SketchMaster {

// SVG导入类
class SVGImporter {
public:
    SVGImporter();
    ~SVGImporter();

    // 从文件导入SVG
    bool importFromFile(const std::string& path);
    
    // 从字符串导入SVG
    bool importFromString(const std::string& svgContent);
    
    // 获取导入的路径
    std::vector<Polyline> getPolylines() const { return polylines_; }
    
    // 获取SVG尺寸
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }
    
    // 获取解析错误信息
    std::string getError() const { return errorMessage_; }

private:
    std::vector<Polyline> polylines_;
    int width_;
    int height_;
    std::string errorMessage_;
    
    // 解析SVG内容
    bool parseSVG(const std::string& svgContent);
    
    // 解析path元素的d属性
    std::vector<Point2D> parsePathData(const std::string& d);
    
    // 解析数字
    double parseNumber(const std::string& str, size_t& pos);
    
    // 跳过空白字符
    void skipWhitespace(const std::string& str, size_t& pos);
    
    // 提取属性值
    std::string extractAttribute(const std::string& element, const std::string& attrName);
};

} // namespace SketchMaster
