#include "SVGImporter.h"
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <regex>

namespace SketchMaster {

SVGImporter::SVGImporter()
    : width_(0), height_(0) {}

SVGImporter::~SVGImporter() {}

bool SVGImporter::importFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        errorMessage_ = "无法打开文件: " + path;
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    
    return importFromString(buffer.str());
}

bool SVGImporter::importFromString(const std::string& svgContent) {
    polylines_.clear();
    width_ = 0;
    height_ = 0;
    errorMessage_.clear();
    
    return parseSVG(svgContent);
}

bool SVGImporter::parseSVG(const std::string& svgContent) {
    // 提取svg标签的属性
    std::regex svgRegex("<svg([^>]*)>");
    std::smatch svgMatch;
    if (std::regex_search(svgContent, svgMatch, svgRegex)) {
        std::string svgAttrs = svgMatch[1].str();
        
        // 提取width
        std::regex widthRegex("width=\"([^\"]+)\"");
        std::smatch widthMatch;
        if (std::regex_search(svgAttrs, widthMatch, widthRegex)) {
            width_ = static_cast<int>(std::stod(widthMatch[1].str()));
        }
        
        // 提取height
        std::regex heightRegex("height=\"([^\"]+)\"");
        std::smatch heightMatch;
        if (std::regex_search(svgAttrs, heightMatch, heightRegex)) {
            height_ = static_cast<int>(std::stod(heightMatch[1].str()));
        }
        
        // 提取viewBox
        if (width_ == 0 || height_ == 0) {
            std::regex viewBoxRegex("viewBox=\"([^\"]+)\"");
            std::smatch viewBoxMatch;
            if (std::regex_search(svgAttrs, viewBoxMatch, viewBoxRegex)) {
                std::string viewBox = viewBoxMatch[1].str();
                std::istringstream iss(viewBox);
                double x, y, w, h;
                iss >> x >> y >> w >> h;
                width_ = static_cast<int>(w);
                height_ = static_cast<int>(h);
            }
        }
    }
    
    // 解析path元素
    std::regex pathRegex("<path([^/]*)/?>");
    std::sregex_iterator pathBegin(svgContent.begin(), svgContent.end(), pathRegex);
    std::sregex_iterator pathEnd;
    
    for (auto it = pathBegin; it != pathEnd; ++it) {
        std::string pathAttrs = (*it)[1].str();
        
        // 提取d属性
        std::regex dRegex("d=\"([^\"]+)\"");
        std::smatch dMatch;
        if (std::regex_search(pathAttrs, dMatch, dRegex)) {
            std::string d = dMatch[1].str();
            auto points = parsePathData(d);
            
            if (points.size() >= 2) {
                Polyline poly;
                poly.points = points;
                
                // 提取stroke-width
                std::regex strokeWidthRegex("stroke-width=\"([^\"]+)\"");
                std::smatch strokeWidthMatch;
                if (std::regex_search(pathAttrs, strokeWidthMatch, strokeWidthRegex)) {
                    poly.thickness = std::stod(strokeWidthMatch[1].str());
                } else {
                    poly.thickness = 1.0;
                }
                
                polylines_.push_back(poly);
            }
        }
    }
    
    // 解析line元素
    std::regex lineRegex("<line([^/]*)/?>");
    std::sregex_iterator lineBegin(svgContent.begin(), svgContent.end(), lineRegex);
    std::sregex_iterator lineEnd;
    
    for (auto it = lineBegin; it != lineEnd; ++it) {
        std::string lineAttrs = (*it)[1].str();
        
        double x1 = 0, y1 = 0, x2 = 0, y2 = 0;
        std::regex x1Regex("x1=\"([^\"]+)\"");
        std::smatch x1Match;
        if (std::regex_search(lineAttrs, x1Match, x1Regex)) x1 = std::stod(x1Match[1].str());
        
        std::regex y1Regex("y1=\"([^\"]+)\"");
        std::smatch y1Match;
        if (std::regex_search(lineAttrs, y1Match, y1Regex)) y1 = std::stod(y1Match[1].str());
        
        std::regex x2Regex("x2=\"([^\"]+)\"");
        std::smatch x2Match;
        if (std::regex_search(lineAttrs, x2Match, x2Regex)) x2 = std::stod(x2Match[1].str());
        
        std::regex y2Regex("y2=\"([^\"]+)\"");
        std::smatch y2Match;
        if (std::regex_search(lineAttrs, y2Match, y2Regex)) y2 = std::stod(y2Match[1].str());
        
        Polyline poly;
        poly.points.emplace_back(x1, y1);
        poly.points.emplace_back(x2, y2);
        
        std::regex strokeWidthRegex("stroke-width=\"([^\"]+)\"");
        std::smatch strokeWidthMatch;
        if (std::regex_search(lineAttrs, strokeWidthMatch, strokeWidthRegex)) {
            poly.thickness = std::stod(strokeWidthMatch[1].str());
        } else {
            poly.thickness = 1.0;
        }
        
        polylines_.push_back(poly);
    }
    
    // 解析polyline元素
    std::regex polylineRegex("<polyline([^/]*)/?>");
    std::sregex_iterator polylineBegin(svgContent.begin(), svgContent.end(), polylineRegex);
    std::sregex_iterator polylineEnd;
    
    for (auto it = polylineBegin; it != polylineEnd; ++it) {
        std::string polylineAttrs = (*it)[1].str();
        
        std::regex pointsRegex("points=\"([^\"]+)\"");
        std::smatch pointsMatch;
        if (std::regex_search(polylineAttrs, pointsMatch, pointsRegex)) {
            std::string pointsStr = pointsMatch[1].str();
            std::istringstream iss(pointsStr);
            std::vector<Point2D> points;
            double x, y;
            while (iss >> x >> y) {
                points.emplace_back(x, y);
            }
            
            if (points.size() >= 2) {
                Polyline poly;
                poly.points = points;
                
                std::regex strokeWidthRegex("stroke-width=\"([^\"]+)\"");
                std::smatch strokeWidthMatch;
                if (std::regex_search(polylineAttrs, strokeWidthMatch, strokeWidthRegex)) {
                    poly.thickness = std::stod(strokeWidthMatch[1].str());
                } else {
                    poly.thickness = 1.0;
                }
                
                polylines_.push_back(poly);
            }
        }
    }
    
    return true;
}

std::vector<Point2D> SVGImporter::parsePathData(const std::string& d) {
    std::vector<Point2D> points;
    size_t pos = 0;
    double currentX = 0, currentY = 0;
    double startX = 0, startY = 0;
    bool hasStart = false;
    
    while (pos < d.length()) {
        skipWhitespace(d, pos);
        if (pos >= d.length()) break;
        
        char cmd = d[pos];
        bool isRelative = std::islower(cmd);
        pos++;
        
        switch (std::toupper(cmd)) {
            case 'M': { // 移动到
                skipWhitespace(d, pos);
                double x = parseNumber(d, pos);
                skipWhitespace(d, pos);
                double y = parseNumber(d, pos);
                
                if (isRelative) {
                    currentX += x;
                    currentY += y;
                } else {
                    currentX = x;
                    currentY = y;
                }
                
                points.emplace_back(currentX, currentY);
                startX = currentX;
                startY = currentY;
                hasStart = true;
                
                // 处理后续的隐式L命令
                while (pos < d.length() && (std::isdigit(d[pos]) || d[pos] == '-' || d[pos] == '.')) {
                    skipWhitespace(d, pos);
                    double lx = parseNumber(d, pos);
                    skipWhitespace(d, pos);
                    double ly = parseNumber(d, pos);
                    
                    if (isRelative) {
                        currentX += lx;
                        currentY += ly;
                    } else {
                        currentX = lx;
                        currentY = ly;
                    }
                    
                    points.emplace_back(currentX, currentY);
                }
                break;
            }
            
            case 'L': { // 直线到
                skipWhitespace(d, pos);
                double x = parseNumber(d, pos);
                skipWhitespace(d, pos);
                double y = parseNumber(d, pos);
                
                if (isRelative) {
                    currentX += x;
                    currentY += y;
                } else {
                    currentX = x;
                    currentY = y;
                }
                
                points.emplace_back(currentX, currentY);
                break;
            }
            
            case 'H': { // 水平线
                skipWhitespace(d, pos);
                double x = parseNumber(d, pos);
                
                if (isRelative) {
                    currentX += x;
                } else {
                    currentX = x;
                }
                
                points.emplace_back(currentX, currentY);
                break;
            }
            
            case 'V': { // 垂直线
                skipWhitespace(d, pos);
                double y = parseNumber(d, pos);
                
                if (isRelative) {
                    currentY += y;
                } else {
                    currentY = y;
                }
                
                points.emplace_back(currentX, currentY);
                break;
            }
            
            case 'C': { // 三次贝塞尔曲线 - 简化为起点和终点
                skipWhitespace(d, pos);
                double x1 = parseNumber(d, pos);
                skipWhitespace(d, pos);
                double y1 = parseNumber(d, pos);
                skipWhitespace(d, pos);
                double x2 = parseNumber(d, pos);
                skipWhitespace(d, pos);
                double y2 = parseNumber(d, pos);
                skipWhitespace(d, pos);
                double x = parseNumber(d, pos);
                skipWhitespace(d, pos);
                double y = parseNumber(d, pos);
                
                if (isRelative) {
                    currentX += x;
                    currentY += y;
                } else {
                    currentX = x;
                    currentY = y;
                }
                
                points.emplace_back(currentX, currentY);
                break;
            }
            
            case 'Q': { // 二次贝塞尔曲线 - 简化为起点和终点
                skipWhitespace(d, pos);
                double x1 = parseNumber(d, pos);
                skipWhitespace(d, pos);
                double y1 = parseNumber(d, pos);
                skipWhitespace(d, pos);
                double x = parseNumber(d, pos);
                skipWhitespace(d, pos);
                double y = parseNumber(d, pos);
                
                if (isRelative) {
                    currentX += x;
                    currentY += y;
                } else {
                    currentX = x;
                    currentY = y;
                }
                
                points.emplace_back(currentX, currentY);
                break;
            }
            
            case 'Z':
            case 'z': { // 闭合路径
                if (hasStart) {
                    currentX = startX;
                    currentY = startY;
                    points.emplace_back(currentX, currentY);
                }
                break;
            }
            
            default:
                // 跳过未知命令
                break;
        }
    }
    
    return points;
}

double SVGImporter::parseNumber(const std::string& str, size_t& pos) {
    skipWhitespace(str, pos);
    
    size_t start = pos;
    
    // 处理符号
    if (pos < str.length() && (str[pos] == '-' || str[pos] == '+')) {
        pos++;
    }
    
    // 处理整数部分
    while (pos < str.length() && std::isdigit(str[pos])) {
        pos++;
    }
    
    // 处理小数部分
    if (pos < str.length() && str[pos] == '.') {
        pos++;
        while (pos < str.length() && std::isdigit(str[pos])) {
            pos++;
        }
    }
    
    // 处理科学计数法
    if (pos < str.length() && (str[pos] == 'e' || str[pos] == 'E')) {
        pos++;
        if (pos < str.length() && (str[pos] == '-' || str[pos] == '+')) {
            pos++;
        }
        while (pos < str.length() && std::isdigit(str[pos])) {
            pos++;
        }
    }
    
    if (start == pos) {
        return 0;
    }
    
    return std::stod(str.substr(start, pos - start));
}

void SVGImporter::skipWhitespace(const std::string& str, size_t& pos) {
    while (pos < str.length() && (std::isspace(str[pos]) || str[pos] == ',')) {
        pos++;
    }
}

std::string SVGImporter::extractAttribute(const std::string& element, const std::string& attrName) {
    std::regex attrRegex(attrName + "=\"([^\"]+)\"");
    std::smatch match;
    if (std::regex_search(element, match, attrRegex)) {
        return match[1].str();
    }
    return "";
}

} // namespace SketchMaster
