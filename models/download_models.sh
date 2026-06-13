#!/bin/bash
# SketchMaster 模型下载脚本
# 用法: bash models/download_models.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

echo "========================================="
echo "  SketchMaster 模型下载脚本"
echo "========================================="
echo ""

# 检查 curl 或 wget
if command -v curl &> /dev/null; then
    DOWNLOAD_CMD="curl -L -o"
elif command -v wget &> /dev/null; then
    DOWNLOAD_CMD="wget -O"
else
    echo "错误: 需要 curl 或 wget 来下载文件"
    exit 1
fi

# ---- HED 模型 ----
echo "[1/3] 下载 HED 模型文件..."
mkdir -p "$PROJECT_DIR/models/hed"

if [ ! -f "$PROJECT_DIR/models/hed/deploy.prototxt" ]; then
    echo "  下载 deploy.prototxt..."
    $DOWNLOAD_CMD "$PROJECT_DIR/models/hed/deploy.prototxt" \
        https://raw.githubusercontent.com/s9xie/hed/master/deploy.prototxt
    echo "  deploy.prototxt 下载完成"
else
    echo "  deploy.prototxt 已存在，跳过"
fi

if [ ! -f "$PROJECT_DIR/models/hed/hed_pretrained_bsds.caffemodel" ]; then
    echo ""
    echo "  hed_pretrained_bsds.caffemodel 需要手动下载："
    echo "  1. 访问 https://github.com/s9xie/hed"
    echo "  2. 下载 hed_pretrained_bsds.caffemodel"
    echo "  3. 放到 models/hed/ 目录下"
    echo ""
else
    echo "  hed_pretrained_bsds.caffemodel 已存在，跳过"
fi

# ---- MLSD 模型 ----
echo ""
echo "[2/3] MLSD 模型需要手动下载："
echo "  1. 访问 https://huggingface.co/lllyasviel/ControlNet"
echo "  2. 下载 mlsd_large.onnx（推荐）或 mlsd_tiny.onnx"
echo "  3. 放到 models/mlsd/ 目录下"
mkdir -p "$PROJECT_DIR/models/mlsd"

# ---- 完成 ----
echo ""
echo "[3/3] 下载完成"
echo ""
echo "========================================="
echo "  模型文件状态："
echo "========================================="
for f in "$PROJECT_DIR/models/hed/deploy.prototxt" \
         "$PROJECT_DIR/models/hed/hed_pretrained_bsds.caffemodel" \
         "$PROJECT_DIR/models/mlsd/mlsd_large.onnx" \
         "$PROJECT_DIR/models/mlsd/mlsd_tiny.onnx"; do
    if [ -f "$f" ]; then
        SIZE=$(du -h "$f" | cut -f1)
        echo "  [OK] $(basename $f) ($SIZE)"
    else
        echo "  [--] $(basename $f) (未找到)"
    fi
done
echo ""
echo "请确保所有必需的模型文件都已下载。"
