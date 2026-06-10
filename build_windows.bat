@echo off
chcp 65001 >nul
echo ============================================
echo   SketchMaster Windows 编译脚本
echo ============================================
echo.

:: 检查依赖
where cmake >nul 2>nul
if %errorlevel% neq 0 (
    echo [错误] 未找到 cmake，请先安装 CMake 并添加到 PATH
    echo 下载地址: https://cmake.org/download/
    goto :end
)

where g++ >nul 2>nul || where cl >nul 2>nul
if %errorlevel% neq 0 (
    echo [错误] 未找到 C++ 编译器
    echo.
    echo 请选择以下方式之一安装编译环境：
    echo.
    echo 方式1 (推荐): 安装 Qt6 在线安装器
    echo   https://www.qt.io/download-qt-installer
    echo   安装时勾选 "MinGW" 编译器组件
    echo.
    echo 方式2: 安装 Visual Studio 2022 Community (免费)
    echo   https://visualstudio.microsoft.com/zh-hans/vs/community/
    echo   安装时勾选 "使用C++的桌面开发"
    echo.
    goto :end
)

:: 检查 Qt6
echo [检查] 正在检查 Qt6...
cmake --find-package -DNAME=Qt6 -DCOMPILER_ID=GNU -DLANGUAGE=CXX -DMODE=COMPILE >nul 2>nul
if %errorlevel% neq 0 (
    echo [警告] CMake 未自动找到 Qt6
    echo.
    echo 如果已安装 Qt6，请设置环境变量：
    echo   set CMAKE_PREFIX_PATH=C:\Qt\6.x.x\mingw_64
    echo.
    echo 或者修改本脚本，在 cmake 命令中添加：
    echo   -DCMAKE_PREFIX_PATH=C:\Qt\6.x.x\mingw_64
    echo.
)

:: 检查 OpenCV
echo [检查] 正在检查 OpenCV...
cmake --find-package -DNAME=OpenCV -DCOMPILER_ID=GNU -DLANGUAGE=CXX -DMODE=COMPILE >nul 2>nul
if %errorlevel% neq 0 (
    echo [警告] CMake 未自动找到 OpenCV
    echo.
    echo 如果已安装 OpenCV，请设置环境变量：
    echo   set OpenCV_DIR=C:\opencv\build
    echo.
)

echo.
echo [开始] 创建构建目录...
if not exist build mkdir build
cd build

echo [编译] 正在配置项目...
cmake .. -G "MinGW Makefiles" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_PREFIX_PATH=%CMAKE_PREFIX_PATH% ^
    -DOpenCV_DIR=%OpenCV_DIR%

if %errorlevel% neq 0 (
    echo.
    echo [错误] CMake 配置失败！
    echo.
    echo 常见原因：
    echo 1. Qt6 未安装或路径不正确
    echo 2. OpenCV 未安装或路径不正确
    echo 3. 编译器未安装
    echo.
    echo 请根据上方提示安装依赖后重试。
    goto :end
)

echo.
echo [编译] 正在编译项目...
cmake --build . --config Release -j%NUMBER_OF_PROCESSORS%

if %errorlevel% neq 0 (
    echo.
    echo [错误] 编译失败！请检查编译错误信息。
    goto :end
)

echo.
echo ============================================
echo   编译成功！
echo ============================================
echo.
echo 可执行文件位置: build\Release\SketchMaster.exe
echo 或: build\SketchMaster.exe
echo.

:: 复制资源文件
if exist ..\resources (
    echo [复制] 正在复制资源文件...
    if not exist Release mkdir Release 2>nul
    xcopy /Y /E ..\resources Release\resources\ >nul
    xcopy /Y /E ..\resources .\resources\ >nul
)

echo.
echo 运行程序: build\SketchMaster.exe
echo.

:end
pause
