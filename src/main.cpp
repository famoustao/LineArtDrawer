#include <QApplication>
#include "ui/MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    
    // 设置应用程序信息
    app.setApplicationName("SketchMaster");
    app.setOrganizationName("SketchMaster");
    app.setApplicationDisplayName("SketchMaster - 图片转线稿与自动绘制工具");
    
    // 创建主窗口
    SketchMaster::MainWindow window;
    window.show();
    
    return app.exec();
}
