cmake_minimum_required(VERSION 3.16)
project(SketchMaster VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找依赖包
find_package(OpenCV REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui)

# 启用Qt自动处理
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 源文件
set(SOURCES
    src/main.cpp
    src/core/ImageProcessor.cpp
    src/core/LineArtGenerator.cpp
    src/core/SVGExporter.cpp
    src/core/SVGImporter.cpp
    src/core/PathSimplifier.cpp
    src/core/ImageTracer.cpp
    src/ui/MainWindow.cpp
    src/ui/SketchCanvas.cpp
    src/ui/ControlPanel.cpp
    src/mouse/MouseController.cpp
    src/mouse/MousePainter.cpp
    src/mouse/DrawingAreaSelector.cpp
    src/input/HotkeyManager.cpp
)

set(HEADERS
    src/core/ImageProcessor.h
    src/core/LineArtGenerator.h
    src/core/SVGExporter.h
    src/core/SVGImporter.h
    src/core/PathSimplifier.h
    src/core/ImageTracer.h
    src/ui/MainWindow.h
    src/ui/SketchCanvas.h
    src/ui/ControlPanel.h
    src/mouse/MouseController.h
    src/mouse/MousePainter.h
    src/mouse/DrawingAreaSelector.h
    src/input/HotkeyManager.h
)

# 创建可执行文件
add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS})

# 跨平台链接库
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Gui
        ${OpenCV_LIBS}
    )
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Gui
        ${OpenCV_LIBS}
        X11
        Xtst
    )
endif()

# 包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    src
)

# Windows: 将resources目录复制到输出目录
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/resources
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources
    )
endif()

# 安装目标
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
