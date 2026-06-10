#include "SVGExporter.h"
#include <fstream>
#include <sstream>
#include <iomanip>

namespace SketchMaster {

SVGExporter::SVGExporter()
    : strokeColor_("#000000"), backgroundColor_("#FFFFFF"), defaultStrokeWidth_(1.0) {}

SVGExporter::~SVGExporter() {}

bool SVGExporter::exportToFile(const std::string& path,
                               const std::vector<Polyline>& polylines,
                               int width, int height) {
    std::string svgContent = exportToString(polylines, width, height);
    
    std::ofstream file(path);
    if (!file.is_open()) {
        return false;
    }
    
    file << svgContent;
    file.close();
    
    return true;
}

std::string SVGExporter::exportToString(const std::vector<Polyline>& polylines,
                                        int width, int height) {
    std::ostringstream svg;
    
    // SVG头部
    svg << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n";
    svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" ";
    svg << "width=\"" << width << "\" height=\"" << height << "\" ";
    svg << "viewBox=\"0 0 " << width << " " << height << "\">\n";
    
    // 背景
    svg << "  <rect width=\"100%\" height=\"100%\" fill=\"" << backgroundColor_ << "\"/>\n";
    
    // 绘制每条路径
    for (const auto& poly : polylines) {
        if (poly.points.size() < 2) continue;
        
        std::string pathData = generatePathData(poly);
        double strokeWidth = poly.thickness > 0 ? poly.thickness : defaultStrokeWidth_;
        
        svg << "  <path d=\"" << pathData << "\" ";
        svg << "stroke=\"" << strokeColor_ << "\" ";
        svg << "stroke-width=\"" << strokeWidth << "\" ";
        svg << "fill=\"none\" ";
        svg << "stroke-linecap=\"round\" ";
        svg << "stroke-linejoin=\"round\"/>\n";
    }
    
    svg << "</svg>\n";
    
    return svg.str();
}

std::string SVGExporter::generatePathData(const Polyline& poly) {
    if (poly.points.empty()) return "";
    
    std::ostringstream pathData;
    pathData << std::fixed << std::setprecision(2);
    
    // 移动到第一个点
    pathData << "M " << poly.points[0].x << " " << poly.points[0].y;
    
    // 绘制线段
    for (size_t i = 1; i < poly.points.size(); ++i) {
        pathData << " L " << poly.points[i].x << " " << poly.points[i].y;
    }
    
    return pathData.str();
}

} // namespace SketchMaster
