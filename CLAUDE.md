# Skia Painter

基于 C++ + V8 + Skia 的 Canvas 渲染引擎，执行 JavaScript 并输出高质量 PNG。专为 ECharts 图表服务端渲染优化。

## 快速命令

```bash
# macOS 构建
cmake --preset dev && cmake --build --preset dev
cmake --preset release && cmake --build --preset release

# 运行示例
./build/dev/canvas_engine examples/echarts_bar.js

# 测试
ctest --preset dev --output-on-failure

# 批量生成所有 PNG
for f in examples/*.js; do ./build/release/canvas_engine "$f"; done

# 开发服务器 (Compare Lab, port 8787)
node scripts/dev_compare_server.js

# Docker Linux 构建
docker build -t skia-painter-linux .
docker cp "$(docker create skia-painter-linux)":/usr/local/bin/canvas_engine ./canvas_engine-linux

# Docker 直接运行
docker run --rm -v "$PWD":/work skia-painter-linux /work/examples/smoke.js
```

## 架构概览

```
┌─────────────────────────────────────────────────────────┐
│  examples/*.js  (ECharts 脚本, 53个)                      │
│    loadScript("echarts.js")                              │
│    new Canvas(960,540) → getContext("2d") → bindbindDraw │
│    bindbindchart bindbindbindSaveToPng()              │
└──────────────┬──────────────────────────────────────────┘
               │ V8 执行
┌──────────────▼──────────────────────────────────────────┐
│  ScriptEngine  (src/runtime/ScriptEngine.cc)            │
│  - V8 isolate/context 管理                                │
│  - JS↔C++ 绑定: Canvas, Image, Context2D, Gradient,     │
│    Pattern, console, setTimeout, requestAnimationFrame   │
│  - 任务队列 (PendingTask) + DrainPendingTasks()           │
│  - 路径解析: 相对于脚本目录                                  │
│  - V8 数据目录自动检测 (macOS Homebrew / Linux /usr/local) │
└──────────────┬──────────────────────────────────────────┘
               │
┌──────────────▼──────────────────────────────────────────┐
│  Canvas 层 (src/canvas/)                                 │
│                                                          │
│  CanvasSurface        Canvas2DContext       ImageAsset   │
│  - Skia 光栅表面       - 2D 绘图状态机        - 图片加载解码 │
│  - 逻辑尺寸×DPR=物理   - fill/stroke/text    - macOS:     │
│    尺寸(默认3x)        - path/transform/clip   ImageIO   │
│  - SavePng():          - gradient/pattern    - Linux:    │
│    macOS=ImageIO       - getImageData/         libpng    │
│    Linux=SkPngEncoder    putImageData        - → SkImage │
└──────────────┬──────────────────────────────────────────┘
               │
┌──────────────▼──────────────────────────────────────────┐
│  Skia (third_party/skia, Bazel 构建)                     │
│  - SkSurface (RGBA8888 raster)                           │
│  - SkCanvas, SkPaint, SkPath, SkFont, SkShader           │
│  - 仅 CPU 光栅化，无 GPU                                   │
└─────────────────────────────────────────────────────────┘
```

## 平台支持

| 功能 | macOS (原生) | Linux (Docker) |
|------|-------------|----------------|
| 图片解码 | CoreGraphics + ImageIO | libpng |
| PNG 导出 | ImageIO | SkPngEncoder |
| 字体渲染 | CoreText | Skia (SkFont) |
| 默认字体 | Helvetica / Times New Roman / Menlo | DejaVu Sans / Serif / Sans Mono |
| 文字测量 | CoreText (CTLine) | Skia (SkFont) |
| V8 来源 | Homebrew | 源码编译 (monolith) |
| 构建方式 | CMake 原生 | Docker 多阶段构建 |

## 目录结构

```
src/
├── main.cc                          # 入口: 接收脚本路径参数
├── canvas/
│   ├── Canvas2DContext.cc/h         # 2D 上下文 (1800行, 核心)
│   ├── CanvasSurface.cc/h           # 光栅表面 + PNG 导出
│   ├── ColorParser.cc/h             # CSS 颜色解析
│   └── ImageAsset.cc/h              # 图片加载 (macOS: ImageIO, Linux: libpng)
└── runtime/
    └── ScriptEngine.cc/h            # V8 引擎 + 全部 JS 绑定 (1000行)

include/canvas_engine/               # 头文件 (与 src/ 镜像)

examples/                            # JS 脚本 (53个)
├── smoke.js / demo.js               # 基础测试
├── image_demo.js                    # 图片功能
└── echarts_*.js                     # ECharts 图表 (50个)

web/compare/                         # Compare Lab (GitHub Pages 部署)
├── index.html                       # 分屏对比 UI
├── app.js                           # 前端逻辑 (浏览器渲染 vs 引擎 PNG)
├── cases.json                       # 55 个测试用例元数据
└── styles.css

scripts/
├── bootstrap.sh                     # macOS 环境初始化 (Homebrew + Skia + CMake)
├── dev_compare_server.js            # 开发服务器 (port 8787)
└── verify_*.py                      # 验证脚本

output/                              # 生成的 PNG (已提交, 用于 GitHub Pages)
Dockerfile                           # Linux 多阶段构建
.github/workflows/
├── deploy.yml                       # GitHub Pages 部署 (轻量, 用提交的 PNG)
└── release.yml                      # Release 构建 (macOS + Linux 二进制)
```

## 核心数据流

```
./canvas_engine examples/echarts_bar.js
  │
  ├─ ScriptEngine::RunScriptFile()
  │    ├─ ReadFile() 加载 JS
  │    ├─ V8 执行脚本
  │    │    ├─ loadScript("echarts.js")  → 加载并 eval
  │    │    ├─ new Canvas(960,540)       → CanvasSurface::CreateRaster(960,540,3.0)
  │    │    │                              → Skia 分配 2880×1620 RGBA 光栅
  │    │    ├─ echarts.init(canvas)      → zrender 绑定到 canvas
  │    │    ├─ chart.setOption({...})    → 触发大量 ctx draw 调用
  │    │    │                              → Skia SkCanvas 执行绘制
  │    │    ├─ refreshImmediately()      → 刷新渲染
  │    │    └─ canvas.saveToPng(path)    → CanvasSurface::SavePng()
  │    └─ DrainPendingTasks()           → 执行队列中的 RAF/setTimeout
  │
  └─ 输出: output/echarts_bar.png (2880×1620)
```

## 关键类说明

### ScriptEngine (`src/runtime/ScriptEngine.cc`)
- V8 isolate + context 生命周期管理
- 内部结构: `CanvasHandle`, `ImageHandle`, `GradientHandle`, `PatternHandle`, `PendingTask`
- 约 60 个静态回调函数绑定 JS API
- `DrainPendingTasks()`: 按到期时间排序执行回调
- `DetectV8DataDir()`: 自动查找 V8 ICU 数据, 支持 `V8_DATA_DIR` 环境变量

### Canvas2DContext (`src/canvas/Canvas2DContext.cc`)
- `State` 结构体: fill/stroke style, line props, font, transform, clip, shadow, blend mode
- save/restore 状态栈
- `MakeFillPaint()` / `MakeStrokePaint()` → 构建 SkPaint
- `MakeFont()` → 解析 CSS font, 创建 SkFont
- `ParseFontFamily()` → 平台相关字体映射 (macOS: Helvetica, Linux: DejaVu Sans)
- 支持 100+ Canvas2D API 方法

### CanvasSurface (`src/canvas/CanvasSurface.cc`)
- 逻辑尺寸 × pixel_ratio = 物理尺寸 (默认 DPR=3.0, 可通过 `CANVAS_ENGINE_DPR` 环境变量调整)
- `SavePng()`: macOS 用 ImageIO, Linux 用 SkPngEncoder

### ImageAsset (`src/canvas/ImageAsset.cc`)
- macOS: CFURLRef → CGImageSource → CGBitmapContext → SkImage
- Linux: libpng 解码 PNG → 预乘 alpha → SkImage
- 仅支持本地文件，不支持 HTTP

## Docker 构建 (Linux)

`Dockerfile` 使用 4 阶段构建:

1. **v8-builder**: 用 depot_tools 从源码构建 V8 monolith 静态库 (首次慢, Docker 缓存后快)
2. **skia-builder**: 用 Bazel 构建 Skia (core, base, skcms)
3. **project-builder**: CMake 构建 canvas_engine, 链接 V8 monolith + Skia + libpng
4. **runtime**: 最小运行时镜像 (仅二进制 + 字体 + echarts)

```bash
# 构建完整 Linux 镜像
docker build -t skia-painter-linux .

# 提取二进制
docker cp "$(docker create skia-painter-linux)":/usr/local/bin/canvas_engine ./canvas_engine-linux

# 直接运行示例
docker run --rm -v "$PWD":/work skia-painter-linux /work/examples/smoke.js
```

## CI/CD

### GitHub Pages (`deploy.yml`)
- 轻量级: ubuntu-latest, 只装 Node.js
- 直接用仓库中提交的 `output/*.png`
- 组装 `_site/` 并部署

### Release (`release.yml`)
- 触发: push tag `v*` 或手动 dispatch
- **macOS job**: 原生编译 (macos-15), 生成 `canvas_engine-macos-arm64.tar.gz`
- **Linux job**: Docker 构建, 生成 `canvas_engine-linux-x64.tar.gz`
- 两个二进制都上传到 GitHub Release
- Docker 层缓存到 GitHub Actions cache

发布流程:
```bash
git tag v0.1.0
git push origin v0.1.0
# → 自动触发 release workflow → 构建双平台 → 创建 GitHub Release
```

## 构建依赖

### macOS (原生)
- **C++20**, CMake 3.20+, Ninja
- **V8** (Homebrew, `/opt/homebrew/opt/v8/`)
- **Skia** (submodule, Bazel 构建, 仅 core/base/skcms)
- **macOS 框架**: CoreFoundation, CoreGraphics, ImageIO, CoreText
- **npm**: echarts@6.0.0

### Linux (Docker)
- **C++20**, CMake, Ninja
- **V8** (源码编译 monolith, `/usr/local/lib/libv8_monolith.a`)
- **Skia** (同 macOS, Bazel 构建)
- **libpng**, zlib, freetype
- **fonts-dejavu-core** (DejaVu 字体族)
- **npm**: echarts@6.0.0

## 注意事项

- 路径解析基于脚本文件所在目录，不是进程 cwd
- JS 侧查询 devicePixelRatio 返回 1.0，但内部用 3x 渲染
- 无 DOM/CSSOM，仅 Canvas API
- Linux 图片解码目前仅支持 PNG 格式 (通过 libpng)
- macOS 图片解码支持 PNG/JPEG/GIF 等 (通过 ImageIO)
- 无 Path2D、无 Hit Testing、无 createImageData
- V8 ICU 数据目录可通过 `V8_DATA_DIR` 环境变量指定
