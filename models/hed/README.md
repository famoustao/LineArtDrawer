[README.md](https://github.com/user-attachments/files/28905505/README.md)
# HED 模型文件

HED (Holistically-Nested Edge Detection) 使用 Caffe 预训练模型。

## 所需文件

1. `deploy.prototxt` - 网络结构定义文件（约 5KB）
2. `hed_pretrained_bsds.caffemodel` - 预训练权重文件（约 57MB）

## 下载方式

### 方式一：手动下载

1. 下载 `deploy.prototxt`：
   ```
   https://raw.githubusercontent.com/s9xie/hed/master/deploy.prototxt
   ```

2. 下载 `hed_pretrained_bsds.caffemodel`：
   - 访问 HED 官方仓库：https://github.com/s9xie/hed
   - 或从以下地址下载：
     ```
     https://drive.google.com/file/d/0B-y7nFm6uJwsZHlFNUVnLVJCbWs/view
     ```

3. 将两个文件放到本目录（`models/hed/`）下。

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
| deploy.prototxt | ~5KB |
| hed_pretrained_bsds.caffemodel | ~57MB |

## 注意事项

- 模型文件因体积较大，不包含在 Git 仓库中。
- `.gitignore` 已配置忽略 `*.caffemodel` 和 `*.onnx` 文件。
- 程序启动时会自动在以下路径搜索模型文件：
  1. `models/hed/`（相对于当前工作目录）
  2. `models/hed/`（相对于可执行文件所在目录）
  3. `../models/hed/`（上级目录）
