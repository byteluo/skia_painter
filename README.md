# Skia Painter

一个基于 `C++ + V8 + Skia` 的 Canvas 后端引擎骨架。它现在已经能直接承载 `ECharts` 的 canvas 渲染链路，并导出 PNG。它提供：

- 通过 `V8` 执行 JavaScript
- 通过 `Skia` 完成 2D 栅格绘制
- 一个可运行 `ECharts` 的 `Canvas` / `CanvasRenderingContext2D` 运行时
- 将渲染结果导出为 PNG

当前实现的 API 范围依然不是完整 HTML Canvas 规范，但已经覆盖了一批足以驱动 `ECharts` 实际绘图的宿主和 2D 接口。

## 仓库约定

- 顶层仓库用于承载 `canvas_engine` 工程代码
- `third_party/skia` 按子模块路径管理
- 本地构建目录、产物目录默认不提交
- 默认开发构建走 `CMakePresets.json`

## 当前安装状态

我已经在这台机器上安装了：

- `cmake`
- `ninja`
- Homebrew `v8`
- `libpng`
- `freetype`
- `zlib`
- `pkgconf`
- `openjdk`
- `bazel@8`

其中 Homebrew `v8` 的默认路径是：

- include: `/opt/homebrew/opt/v8/include`
- libs: `/opt/homebrew/opt/v8/lib`
- runtime: `/opt/homebrew/opt/v8/libexec`

`Skia` 已经在仓库内以源码方式放在 `third_party/skia`，并通过 `Bazel` 构建出了这套工程当前需要的最小产物：

- `third_party/skia/bazel-bin/src/core/libcore.a`
- `third_party/skia/bazel-bin/src/base/libbase.a`
- `third_party/skia/bazel-bin/modules/skcms/_objs/skcms_public/skcms.o`
- `third_party/skia/bazel-bin/modules/skcms/_objs/skcms_TransformBaseline/skcms_TransformBaseline.o`

当前机器对 `github.com`、`chromium.googlesource.com`、`skia.googlesource.com` 的 TLS 请求不稳定，所以这里采用的是“本地最小可运行构建”，不是一套完整的官方 Skia 全量构建。

这台机器上已经验证通过：

- `./build/dev/canvas_engine examples/smoke.js`
- `./build/dev/canvas_engine examples/demo.js`
- `./build/dev/canvas_engine examples/echarts_bar.js`
- 成功生成 `output/demo.png`
- 成功生成 `output/echarts_bar.png`

## 已实现能力

- `new Canvas(width, height)`
- `canvas.getContext("2d")`
- `canvas.saveToPng(path)`
- `canvas.setAttribute(name, value)`
- `canvas.addEventListener(...)`
- `canvas.removeEventListener(...)`
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
- `ctx.bezierCurveTo(cp1x, cp1y, cp2x, cp2y, x, y)`
- `ctx.rect(x, y, w, h)`
- `ctx.arc(x, y, radius, startAngle, endAngle, counterClockwise = false)`
- `ctx.closePath()`
- `ctx.clip()`
- `ctx.fill()`
- `ctx.stroke()`
- `ctx.measureText(text)`
- `ctx.fillText(text, x, y)`
- `ctx.strokeText(text, x, y)`
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

## 目录结构

```text
.
├── CMakeLists.txt
├── README.md
├── examples
│   ├── demo.js
│   ├── echarts_bar.js
│   ├── echarts_line.js
│   └── smoke.js
├── include
│   └── canvas_engine
│       ├── canvas
│       │   ├── Canvas2DContext.h
│       │   ├── CanvasSurface.h
│       │   └── ColorParser.h
│       └── runtime
│           └── ScriptEngine.h
└── src
    ├── canvas
    │   ├── Canvas2DContext.cc
    │   ├── CanvasSurface.cc
    │   └── ColorParser.cc
    ├── main.cc
    └── runtime
        └── ScriptEngine.cc
```

## 依赖准备

默认情况下，你不需要再手动传 `V8_*` 参数。

在 macOS + Homebrew `v8` 的场景下，`CMakeLists.txt` 会默认探测：

- `/opt/homebrew/opt/v8/include`
- `/opt/homebrew/opt/v8/lib`
- `/opt/homebrew/opt/v8/libexec`
- Homebrew V8 需要的 compile definitions

如果仓库内已经存在本地 Bazel 构建的 Skia 产物，`CMakeLists.txt` 也会默认探测：

- `third_party/skia`
- `third_party/skia/bazel-bin/src/core/libcore.a`
- `third_party/skia/bazel-bin/src/base/libbase.a`
- `third_party/skia/bazel-bin/modules/skcms/_objs/skcms_public/skcms.o`
- `third_party/skia/bazel-bin/modules/skcms/_objs/skcms_TransformBaseline/skcms_TransformBaseline.o`

只有在你换成其他 V8/Skia 构建产物时，才需要手动覆盖 `V8_*` 或 `SKIA_*` 参数。

手动指定时，最小配置参数是：

- `SKIA_INCLUDE_DIRS`
- `SKIA_LIBRARIES`

示例：

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DSKIA_INCLUDE_DIRS="/path/to/skia" \
  -DSKIA_LIBRARIES="/path/to/libskia.a;/path/to/libskia_base.a;/path/to/skcms.o"
```

不同平台和不同构建产物的库名可能不同。比如一些 V8/Skia 构建方式会要求额外链接 `icu*`、`absl_*`、`harfbuzz`、`png`、`zlib`、`freetype` 等库，这部分需要按你的本地产物补齐。当前仓库内的最小 Bazel 产物已经把这套工程需要的链接项稳定下来。

## 构建

如果你需要跑 `ECharts` 示例，先安装 JS 依赖：

```bash
npm install
```

一键安装并完成最小可运行构建：

```bash
./scripts/bootstrap.sh
```

如果你想直接构建 release：

```bash
./scripts/bootstrap.sh release
```

这个脚本会完成：

- 安装 Homebrew 依赖
- 配置 Bazel 所需的 `JAVA_HOME`
- 初始化 `third_party/skia` 子模块
- 构建最小 Skia 产物
- 配置并编译主工程
- 运行冒烟测试

如果仓库里已经存在可用的 Skia 产物，脚本会直接复用；如果你要强制重新构建 Skia：

```bash
FORCE_SKIA_BUILD=1 ./scripts/bootstrap.sh
```

单独构建时也可以用下面的命令：

```bash
cmake --preset dev
cmake --build --preset dev
```

或者直接使用脚本：

```bash
./scripts/build.sh
```

Release 构建：

```bash
cmake --preset release
cmake --build --preset release
```

## 测试

目前提供一个最小冒烟测试，验证 V8 宿主和脚本执行链路。

```bash
ctest --preset dev
```

## 运行示例

先准备输出目录：

```bash
mkdir -p output
```

运行：

```bash
./build/dev/canvas_engine examples/demo.js
```

脚本会输出：

```text
wrote output/demo.png
```

也可以直接运行：

```bash
./scripts/run_demo.sh
```

运行 `ECharts` 示例：

```bash
./build/dev/canvas_engine examples/echarts_bar.js
```

脚本会输出：

```text
wrote output/echarts_bar.png
```

折线图示例：

```bash
./build/dev/canvas_engine examples/echarts_line.js
```

脚本会输出：

```text
wrote output/echarts_line.png
```

## ECharts 集成方式

当前推荐的接入方式是直接加载 `ECharts` 的 UMD 构建，并通过宿主提供的 `Canvas` 对象作为渲染目标：

```js
loadScript("../node_modules/echarts/dist/echarts.js");

const canvas = new Canvas(960, 540);

echarts.setPlatformAPI({
  createCanvas() {
    return new Canvas(960, 540);
  }
});

const chart = echarts.init(canvas, null, {
  renderer: "canvas",
  width: 960,
  height: 540
});
```

这条链路依赖三层能力已经接通：

- `loadScript(path)` 负责把 UMD 包加载进当前 V8 上下文
- `Canvas` / `CanvasRenderingContext2D` 提供 zrender 实际调用的 2D 接口
- `canvas.saveToPng(path)` 把最终结果导出为 PNG

## 示例脚本

见 [examples/demo.js](/Users/treecat/Desktop/skia-painter/examples/demo.js)。

最小冒烟脚本见 [examples/smoke.js](/Users/treecat/Desktop/skia-painter/examples/smoke.js)。

`ECharts` 柱状图示例见 [examples/echarts_bar.js](/Users/treecat/Desktop/skia-painter/examples/echarts_bar.js)。

`ECharts` 折线图示例见 [examples/echarts_line.js](/Users/treecat/Desktop/skia-painter/examples/echarts_line.js)。

## Git 提交建议

如果这是第一次拉取顶层仓库，建议带子模块：

```bash
git clone --recurse-submodules <your-repo-url>
```

如果已经 clone 过，再初始化子模块：

```bash
git submodule update --init --recursive
```

## 后续扩展建议

- 增加 `Path2D`
- 增加图片解码、`Image` 与 `drawImage`
- 增加渐变、pattern 和更多路径语义
- 增加 `ImageData` 和像素级读写
- 把定时器从“立即执行宿主 stub”升级为真正事件循环
- 继续补齐更接近浏览器 / node-canvas 的兼容层
