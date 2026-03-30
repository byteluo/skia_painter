# Skia Painter

一个基于 `C++ + V8 + Skia` 的 Canvas 后端引擎。它的目标不是完整实现浏览器 DOM，而是提供一套足够驱动 `ECharts` canvas 渲染链路的宿主运行时，并把结果稳定导出为 PNG。

当前仓库已经具备这几类能力：

- 通过 `V8` 执行 JavaScript
- 通过 `Skia` 完成 2D 栅格绘制
- 提供 `Canvas` / `CanvasRenderingContext2D` / `Image` 宿主对象
- 直接加载 `ECharts` UMD 产物并完成 canvas 渲染
- 默认导出高清 PNG
- 提供基础冒烟测试和渲染回归测试

## 当前状态

这套工程现在已经不是“骨架”，而是一套可运行的最小 Canvas 渲染后端，重点覆盖了 `ECharts` 真正会走到的那部分 API。

目前已经稳定支持：

- 本地脚本加载：`loadScript(path)`
- 本地图片解码：`new Image()` + `image.src = "..."`
- 2D 路径、文本、阴影、虚线、渐变、pattern、像素读写
- `requestAnimationFrame` / `setTimeout` 的最小任务队列语义
- 默认高清导出

当前默认渲染策略：

- 内部 backing store 默认使用 `DPR = 3`
- `canvas.saveToPng(path)` 默认直接导出 backing store 分辨率
- 例如逻辑尺寸 `960x540`，导出 PNG 默认是 `2880x1620`

路径语义：

- `loadScript(path)`、`image.src = path`、`canvas.saveToPng(path)` 的相对路径都按“当前脚本文件所在目录”解析
- 不是按进程启动目录解析

## 已实现 API

### 宿主对象

- `new Canvas(width, height)`
- `canvas.width`
- `canvas.height`
- `canvas.getContext("2d")`
- `canvas.saveToPng(path)`
- `canvas.setAttribute(name, value)`
- `canvas.addEventListener(...)`
- `canvas.removeEventListener(...)`

- `new Image()`
- `image.width`
- `image.height`
- `image.src`
- `image.complete`
- `image.onload`
- `image.onerror`

### 2D Context

- `ctx.fillStyle`
- `ctx.strokeStyle`
- `ctx.lineWidth`
- `ctx.font`
- `ctx.textAlign`
- `ctx.textBaseline`
- `ctx.globalAlpha`
- `ctx.globalCompositeOperation`
- `ctx.lineCap`
- `ctx.lineJoin`
- `ctx.miterLimit`
- `ctx.shadowBlur`
- `ctx.shadowColor`
- `ctx.shadowOffsetX`
- `ctx.shadowOffsetY`
- `ctx.lineDashOffset`

- `ctx.save()`
- `ctx.restore()`
- `ctx.translate(x, y)`
- `ctx.scale(x, y)`
- `ctx.rotate(radians)`
- `ctx.transform(a, b, c, d, e, f)`
- `ctx.setTransform(a, b, c, d, e, f)`
- `ctx.resetTransform()`

- `ctx.clearRect(x, y, w, h)`
- `ctx.fillRect(x, y, w, h)`
- `ctx.strokeRect(x, y, w, h)`

- `ctx.beginPath()`
- `ctx.moveTo(x, y)`
- `ctx.lineTo(x, y)`
- `ctx.quadraticCurveTo(cpx, cpy, x, y)`
- `ctx.bezierCurveTo(cp1x, cp1y, cp2x, cp2y, x, y)`
- `ctx.rect(x, y, w, h)`
- `ctx.arc(x, y, radius, startAngle, endAngle, counterClockwise = false)`
- `ctx.arcTo(x1, y1, x2, y2, radius)`
- `ctx.ellipse(x, y, rx, ry, rotation, startAngle, endAngle, counterClockwise = false)`
- `ctx.closePath()`
- `ctx.clip()`
- `ctx.fill()`
- `ctx.stroke()`

- `ctx.setLineDash(segments)`
- `ctx.getLineDash()`

- `ctx.createLinearGradient(x0, y0, x1, y1)`
- `ctx.createRadialGradient(x0, y0, r0, x1, y1, r1)`
- `ctx.createPattern(imageOrCanvas, repeat?)`

- `ctx.measureText(text)`
- `ctx.fillText(text, x, y)`
- `ctx.strokeText(text, x, y)`

- `ctx.drawImage(imageOrCanvas, ...)`
- `ctx.getImageData(x, y, w, h)`
- `ctx.putImageData(imageData, dx, dy)`

### 全局能力

- `print(...)`
- `console.log(...)`
- `console.warn(...)`
- `console.error(...)`
- `loadScript(path)`
- `setTimeout(...)`
- `clearTimeout(...)`
- `requestAnimationFrame(...)`
- `cancelAnimationFrame(...)`
- `performance.now()`

## 已验证的 ECharts 图表类型

下面这些示例都已经在当前仓库里实际跑通过，并能导出 PNG：

### 基础图表

- `bar`
- `line`
- `pie`
- `scatter`
- `candlestick`
- `funnel`
- `gauge`
- `radar`
- `heatmap`
- `calendar heatmap`

### 图关系与流向

- `graph`
- `sankey`
- `lines`
- `parallel`
- `themeRiver`

### 层级与布局

- `tree`
- `treemap`
- `sunburst`

### 自定义与组件能力

- `custom series`
- `pictorialBar`
- `timeline`
- `dataZoom + markArea`

### 图片 / 富文本 / 图形混合

- `image scatter`
- `rich graphic`
- `pattern bar`
- `geo heatmap`

## 示例索引

### 冒烟与基础

- [examples/smoke.js](/Users/treecat/Desktop/skia-painter/examples/smoke.js)
- [examples/demo.js](/Users/treecat/Desktop/skia-painter/examples/demo.js)
- [examples/image_demo.js](/Users/treecat/Desktop/skia-painter/examples/image_demo.js)

### ECharts 示例

- [examples/echarts_bar.js](/Users/treecat/Desktop/skia-painter/examples/echarts_bar.js)
- [examples/echarts_line.js](/Users/treecat/Desktop/skia-painter/examples/echarts_line.js)
- [examples/echarts_pie.js](/Users/treecat/Desktop/skia-painter/examples/echarts_pie.js)
- [examples/echarts_scatter.js](/Users/treecat/Desktop/skia-painter/examples/echarts_scatter.js)
- [examples/echarts_heatmap.js](/Users/treecat/Desktop/skia-painter/examples/echarts_heatmap.js)
- [examples/echarts_calendar_heatmap.js](/Users/treecat/Desktop/skia-painter/examples/echarts_calendar_heatmap.js)
- [examples/echarts_candlestick.js](/Users/treecat/Desktop/skia-painter/examples/echarts_candlestick.js)
- [examples/echarts_funnel.js](/Users/treecat/Desktop/skia-painter/examples/echarts_funnel.js)
- [examples/echarts_gauge.js](/Users/treecat/Desktop/skia-painter/examples/echarts_gauge.js)
- [examples/echarts_radar.js](/Users/treecat/Desktop/skia-painter/examples/echarts_radar.js)
- [examples/echarts_graph.js](/Users/treecat/Desktop/skia-painter/examples/echarts_graph.js)
- [examples/echarts_sankey.js](/Users/treecat/Desktop/skia-painter/examples/echarts_sankey.js)
- [examples/echarts_lines.js](/Users/treecat/Desktop/skia-painter/examples/echarts_lines.js)
- [examples/echarts_parallel.js](/Users/treecat/Desktop/skia-painter/examples/echarts_parallel.js)
- [examples/echarts_theme_river.js](/Users/treecat/Desktop/skia-painter/examples/echarts_theme_river.js)
- [examples/echarts_tree.js](/Users/treecat/Desktop/skia-painter/examples/echarts_tree.js)
- [examples/echarts_treemap.js](/Users/treecat/Desktop/skia-painter/examples/echarts_treemap.js)
- [examples/echarts_sunburst.js](/Users/treecat/Desktop/skia-painter/examples/echarts_sunburst.js)
- [examples/echarts_custom_series.js](/Users/treecat/Desktop/skia-painter/examples/echarts_custom_series.js)
- [examples/echarts_pictorial_bar.js](/Users/treecat/Desktop/skia-painter/examples/echarts_pictorial_bar.js)
- [examples/echarts_timeline_bar.js](/Users/treecat/Desktop/skia-painter/examples/echarts_timeline_bar.js)
- [examples/echarts_datazoom_markarea.js](/Users/treecat/Desktop/skia-painter/examples/echarts_datazoom_markarea.js)
- [examples/echarts_image_scatter.js](/Users/treecat/Desktop/skia-painter/examples/echarts_image_scatter.js)
- [examples/echarts_rich_graphic.js](/Users/treecat/Desktop/skia-painter/examples/echarts_rich_graphic.js)
- [examples/echarts_pattern_bar.js](/Users/treecat/Desktop/skia-painter/examples/echarts_pattern_bar.js)
- [examples/echarts_geo_heatmap.js](/Users/treecat/Desktop/skia-painter/examples/echarts_geo_heatmap.js)

## 构建与依赖

### 本机前提

当前仓库的默认 bootstrap 目标是 macOS + Homebrew。脚本会自动处理这批依赖：

- `cmake`
- `ninja`
- `v8`
- `libpng`
- `freetype`
- `zlib`
- `pkgconf`
- `openjdk`
- `bazel@8`

### 一键安装与构建

如果你想从零拉起这套工程，直接运行：

```bash
./scripts/bootstrap.sh
```

如果要构建 release：

```bash
./scripts/bootstrap.sh release
```

这个脚本会完成：

- 安装 Homebrew 依赖
- 配置 `JAVA_HOME`
- 初始化 `third_party/skia`
- 构建当前工程需要的最小 Skia 产物
- 配置并编译主工程
- 运行测试

如果你要强制重建 Skia：

```bash
FORCE_SKIA_BUILD=1 ./scripts/bootstrap.sh
```

### 直接使用 CMake Presets

开发构建：

```bash
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
```

Release 构建：

```bash
cmake --preset release
cmake --build --preset release
ctest --preset release
```

预设输出目录：

- `build/dev`
- `build/release`

### V8 / Skia 自动探测

在 macOS + Homebrew `v8` 场景下，`CMakeLists.txt` 会自动探测：

- `/opt/homebrew/opt/v8/include`
- `/opt/homebrew/opt/v8/lib`
- `/opt/homebrew/opt/v8/libexec`
- Homebrew V8 对应的 compile definitions

如果仓库内已经存在可用的本地 Skia 产物，也会自动探测：

- `third_party/skia`
- `third_party/skia/bazel-bin/src/core/libcore.a`
- `third_party/skia/bazel-bin/src/base/libbase.a`
- `third_party/skia/bazel-bin/modules/skcms/_objs/skcms_public/skcms.o`
- `third_party/skia/bazel-bin/modules/skcms/_objs/skcms_TransformBaseline/skcms_TransformBaseline.o`

如果你换成了自己的 V8 / Skia 构建产物，再手动覆盖这些变量：

- `V8_INCLUDE_DIRS`
- `V8_LINK_DIRECTORIES`
- `V8_LIBRARIES`
- `V8_COMPILE_DEFINITIONS`
- `SKIA_INCLUDE_DIRS`
- `SKIA_LINK_DIRECTORIES`
- `SKIA_LIBRARIES`

## 测试

当前有两类测试：

- 冒烟测试：验证 V8 宿主和脚本执行链路
- 渲染回归测试：验证高清导出、图片绘制、曲线、椭圆、`arcTo`、`ImageData`、任务队列冲刷语义没有回退

直接运行：

```bash
ctest --test-dir build --output-on-failure
```

或者使用 preset：

```bash
ctest --preset dev
```

## 运行示例

先准备输出目录：

```bash
mkdir -p output
```

开发构建的示例运行方式：

```bash
./build/dev/canvas_engine examples/echarts_bar.js
```

如果你是直接构建到根目录 `build/` 下，也可以这样跑：

```bash
./build/canvas_engine examples/echarts_bar.js
```

运行基础 demo：

```bash
./scripts/run_demo.sh
```

## ECharts 集成方式

当前推荐的接入方式是直接加载 `ECharts` 的 UMD 构建，并通过宿主提供的 `Canvas` 作为渲染目标：

```js
loadScript("../node_modules/echarts/dist/echarts.js");

const width = 960;
const height = 540;
const canvas = new Canvas(width, height);
canvas.style = {};

echarts.setPlatformAPI({
  createCanvas() {
    const next = new Canvas(width, height);
    next.style = {};
    return next;
  },
  loadImage(src, onload) {
    const image = new Image();
    if (typeof onload === "function") {
      image.onload = onload;
    }
    image.src = src;
    return image;
  }
});

const chart = echarts.init(canvas, null, {
  renderer: "canvas",
  width,
  height
});
```

渲染完成后导出：

```js
chart.getZr().refreshImmediately();
canvas.saveToPng("../output/chart.png");
```

## 目录结构

```text
.
├── CMakeLists.txt
├── CMakePresets.json
├── README.md
├── examples
├── include
│   └── canvas_engine
│       ├── canvas
│       └── runtime
├── scripts
├── src
│   ├── canvas
│   └── runtime
└── third_party
    └── skia
```

核心实现位置：

- 2D 绘制实现: [src/canvas/Canvas2DContext.cc](/Users/treecat/Desktop/skia-painter/src/canvas/Canvas2DContext.cc)
- Canvas surface / PNG 导出: [src/canvas/CanvasSurface.cc](/Users/treecat/Desktop/skia-painter/src/canvas/CanvasSurface.cc)
- 图片解码: [src/canvas/ImageAsset.cc](/Users/treecat/Desktop/skia-painter/src/canvas/ImageAsset.cc)
- V8 绑定与宿主运行时: [src/runtime/ScriptEngine.cc](/Users/treecat/Desktop/skia-painter/src/runtime/ScriptEngine.cc)

## 已知限制

这套运行时目前仍然是“面向渲染导出”的最小宿主，不是浏览器环境，也不是完整 `node-canvas` 替代品。

当前明确限制：

- 没有 DOM、CSSOM、布局系统
- `Image.src` 目前只支持本地文件路径，不支持 `http(s)` URL
- 没有完整事件系统，`addEventListener` 主要用于兼容占位
- 没有 `Path2D`
- 没有 `isPointInPath` / `isPointInStroke`
- 没有 `createImageData`
- 没有完整浏览器级异步事件循环

关于定时器语义：

- 现在的 `requestAnimationFrame` / `setTimeout` 已经不是“立即同步执行”
- 它们会进入最小任务队列
- 队列会在脚本执行末尾和 `saveToPng()` 前统一 drain
- 这足以支撑 `ECharts` / `zrender` 的刷新链，但仍然不是通用浏览器事件循环实现

## 下一步

如果继续扩展，优先级建议是：

- 补 `Path2D`
- 补更完整的像素 API，比如 `createImageData`
- 补命中测试 API
- 补更接近浏览器的定时器 / 事件循环语义
- 把当前 ECharts 兼容矩阵继续系统化成自动化回归
