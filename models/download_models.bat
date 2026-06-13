@echo off
chcp 65001 >nul
echo =========================================
echo   SketchMaster 模型下载脚本
echo =========================================
echo.

REM 创建模型目录
if not exist "hed" mkdir hed
if not exist "mlsd" mkdir mlsd

REM ---- HED 模型 ----
echo [1/3] 下载 HED 模型文件...

if not exist "hed\deploy.prototxt" (
    echo   下载 deploy.prototxt...
    curl -L -o hed\deploy.prototxt https://raw.githubusercontent.com/s9xie/hed/master/deploy.prototxt
    echo   deploy.prototxt 下载完成
) else (
    echo   deploy.prototxt 已存在，跳过
)

if not exist "hed\hed_pretrained_bsds.caffemodel" (
    echo.
    echo   hed_pretrained_bsds.caffemodel 需要手动下载：
    echo   1. 访问 https://github.com/s9xie/hed
    echo   2. 下载 hed_pretrained_bsds.caffemodel
    echo   3. 放到 models\hed\ 目录下
    echo.
) else (
    echo   hed_pretrained_bsds.caffemodel 已存在，跳过
)

REM ---- MLSD 模型 ----
echo.
echo [2/3] MLSD 模型需要手动下载：
echo   1. 访问 https://huggingface.co/lllyasviel/ControlNet
echo   2. 下载 mlsd_large.onnx（推荐）或 mlsd_tiny.onnx
echo   3. 放到 models\mlsd\ 目录下

REM ---- 完成 ----
echo.
echo [3/3] 下载完成
echo.
echo =========================================
echo   模型文件状态：
echo =========================================
if exist "hed\deploy.prototxt" (echo   [OK] deploy.prototxt) else (echo   [--] deploy.prototxt (未找到))
if exist "hed\hed_pretrained_bsds.caffemodel" (echo   [OK] hed_pretrained_bsds.caffemodel) else (echo   [--] hed_pretrained_bsds.caffemodel (未找到))
if exist "mlsd\mlsd_large.onnx" (echo   [OK] mlsd_large.onnx) else (echo   [--] mlsd_large.onnx (未找到))
if exist "mlsd\mlsd_tiny.onnx" (echo   [OK] mlsd_tiny.onnx) else (echo   [--] mlsd_tiny.onnx (未找到))
echo.
echo 请确保所有必需的模型文件都已下载。
pause
