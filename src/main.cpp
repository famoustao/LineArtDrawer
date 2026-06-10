#include <QApplication>
#include "ui/MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    
    // 设置应用程序信息
    app.setApplicationName("SketchMaster");
    app.setOrganizationName("SketchMaster");
    app.setApplicationDisplayName("图片转矢量图自动绘图工具，涛2026年6月编译");
    
    // 创建主窗口
    SketchMaster::MainWindow window;
    window.show();
    
    return app.exec();
}
