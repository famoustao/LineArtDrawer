# MLSD 模型文件

MLSD (Mobile Line Segment Detection) 使用 ONNX 预训练模型。

## 所需文件

1. `mlsd_large.onnx` - MLSD Large 版本模型（推荐）
2. 或 `mlsd_tiny.onnx` - MLSD Tiny 版本模型（更小更快）

程序会优先加载 `mlsd_large.onnx`，如果不存在则尝试加载 `mlsd_tiny.onnx`。

## 下载方式

### 方式一：手动下载

1. 访问 HuggingFace 仓库：
   ```
   https://huggingface.co/lllyasviel/ControlNet
   ```

2. 下载 `mlsd_large.onnx`（推荐）或 `mlsd_tiny.onnx`

3. 将文件放到本目录（`models/mlsd/`）下。

### 方式二：使用下载脚本

在项目根目录运行：
```bash
# Linux/macOS
bash models/download_models.sh

# Windows
models\download_models.bat
```

## 文件大小

| 文件 | 大小 |
|------|------|
| mlsd_large.onnx | ~70MB |
| mlsd_tiny.onnx | ~20MB |

## 注意事项

- 模型文件因体积较大，不包含在 Git 仓库中。
- `.gitignore` 已配置忽略 `*.caffemodel` 和 `*.onnx` 文件。
- 程序启动时会自动在以下路径搜索模型文件：
  1. `models/mlsd/`（相对于当前工作目录）
  2. `models/mlsd/`（相对于可执行文件所在目录）
  3. `../models/mlsd/`（上级目录）
