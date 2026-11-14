<div align="center">

# 🎨 Skia Painter

**A headless Canvas backend that renders [Apache ECharts](https://echarts.apache.org/) to high-resolution PNG — powered by C++, V8 and Skia.**

[![Deploy Pages](https://github.com/byteluo/skia_painter/actions/workflows/deploy-pages.yml/badge.svg)](https://github.com/byteluo/skia_painter/actions/workflows/deploy-pages.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C.svg?logo=cplusplus)](CMakeLists.txt)
[![Skia](https://img.shields.io/badge/Skia-2D%20Graphics-EA4335.svg)](https://skia.org/)
[![V8](https://img.shields.io/badge/V8-15.0-4B8BF5.svg)](https://v8.dev/)
[![ECharts](https://img.shields.io/badge/ECharts-6.0-AA344D.svg)](https://echarts.apache.org/)
[![Live Demo](https://img.shields.io/badge/Live-Demo-success.svg)](https://byteluo.github.io/skia_painter/)

**English** · [中文](README.zh-CN.md)

</div>

---

<div align="center">

<img src="output/echarts_bar.png" width="32%" alt="bar"/>
<img src="output/echarts_line.png" width="32%" alt="line"/>
<img src="output/echarts_pie.png" width="32%" alt="pie"/>
<img src="output/echarts_gauge.png" width="32%" alt="gauge"/>
<img src="output/echarts_sankey.png" width="32%" alt="sankey"/>
<img src="output/echarts_sunburst.png" width="32%" alt="sunburst"/>

<sub>A few samples from the demo gallery — see the <a href="https://byteluo.github.io/skia_painter/">live comparison demo</a> for all 50+ charts.</sub>

</div>

---

## Overview

**Skia Painter** is *not* a full browser DOM implementation. It is a minimal host runtime designed to drive the part of the Canvas API that **ECharts** actually exercises, and to export the result as a deterministic high-resolution PNG.

It can:

- Execute JavaScript through **V8**
- Rasterize 2D graphics through **Skia**
- Provide `Canvas` / `CanvasRenderingContext2D` / `Image` host objects
- Load the **ECharts** UMD build directly and render its canvas pipeline
- Export crisp, high-DPI PNGs by default
- Run smoke tests and rendering regression tests

## Why?

Server-side chart rendering usually means running a headless browser (Puppeteer/Playwright) or `node-canvas`. Skia Painter takes a leaner path: a single native binary that boots V8, hands ECharts a Skia-backed canvas, and writes a PNG — no browser, no display server, no DOM.

## Status

This is a **runnable minimal Canvas rendering backend**, not a skeleton. It stably supports:

- Local script loading: `loadScript(path)`
- Local image decoding: `new Image()` + `image.src = "..."`
- 2D paths, text, shadows, dashed lines, gradients, patterns, pixel read/write
- A minimal task-queue semantics for `requestAnimationFrame` / `setTimeout`
- High-DPI export by default

**Default rendering policy:**

- The internal backing store uses `DPR = 3` by default
- `canvas.saveToPng(path)` exports at backing-store resolution
- e.g. a logical size of `960×540` produces a `2880×1620` PNG

**Path resolution:** relative paths in `loadScript(path)`, `image.src = path` and `canvas.saveToPng(path)` are resolved against **the directory of the current script file**, not the process working directory.

## Quick Start

> The default bootstrap target is **macOS + Homebrew**. A Linux bootstrap script (`scripts/bootstrap_linux.sh`) is also provided.

```bash
# Clone with submodules
git clone --recursive https://github.com/byteluo/skia_painter.git
cd skia_painter

# One-shot install + build + test
./scripts/bootstrap.sh          # debug
./scripts/bootstrap.sh release  # release
```

The bootstrap script installs Homebrew dependencies, configures `JAVA_HOME`, initializes `third_party/skia`, builds the minimal Skia artifacts, then configures, compiles and tests the main project.

Force a Skia rebuild:

```bash
FORCE_SKIA_BUILD=1 ./scripts/bootstrap.sh
```

## Run an example

```bash
mkdir -p output
./build/dev/canvas_engine examples/echarts_bar.js
# or, if you built into ./build directly:
./build/canvas_engine examples/echarts_bar.js
# or the basic demo:
./scripts/run_demo.sh
```

## Compare against the browser

Spin up a local dev server that renders the **native browser ECharts canvas** on top and the **PNG exported by this backend** below, so you can compare them visually:

```bash
npm run dev:compare          # serves http://127.0.0.1:8787
```

The compare page covers every `examples/echarts_*.js` in the repo. Point at a custom binary with:

```bash
CANVAS_ENGINE_BIN=/absolute/path/to/canvas_engine npm run dev:compare
```

## Build with CMake Presets

```bash
cmake --preset dev      && cmake --build --preset dev      && ctest --preset dev
cmake --preset release  && cmake --build --preset release  && ctest --preset release
# output dirs: build/dev, build/release
```

## Integrating ECharts

The recommended path is to load the ECharts UMD build and use the host `Canvas` as the render target:

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
    if (typeof onload === "function") image.onload = onload;
    image.src = src;
    return image;
  }
});

const chart = echarts.init(canvas, null, { renderer: "canvas", width, height });

// ... setOption ...

chart.getZr().refreshImmediately();
canvas.saveToPng("../output/chart.png");
```

## Verified ECharts charts

All 50+ examples below have actually been rendered to PNG in this repo (based on `echarts@6.0.0`). Every built-in core chart series now has example coverage.

| Category | Charts |
| --- | --- |
| **Basic** | bar · boxplot · line · pie · scatter · effectScatter · candlestick · funnel · gauge · radar · heatmap · calendar heatmap |
| **Graph & flow** | graph · map · sankey · lines · parallel · themeRiver |
| **Hierarchy & layout** | tree · treemap · sunburst |
| **Custom & components** | custom series · pictorialBar · timeline · dataZoom+markArea · markPoint+markLine · toolbox · brush · dataset+transform · legend scroll · axisPointer · visualMap · piecewise visualMap · multi-grid axisPointer · toolbox magicType / dataZoom · legend selected |
| **Coordinates & containers** | polar · singleAxis · calendar · geo · geo+effectScatter · geo+lines · map+piecewise visualMap · map+scatter+visualMap |
| **Image / rich / mixed** | image scatter · rich graphic · pattern bar · geo heatmap |

The full example index lives in [`examples/`](examples/).

## Implemented API

<details>
<summary><b>Host objects</b></summary>

`new Canvas(width, height)` · `canvas.width` · `canvas.height` · `canvas.getContext("2d")` · `canvas.saveToPng(path)` · `canvas.setAttribute(name, value)` · `canvas.addEventListener(...)` · `canvas.removeEventListener(...)`

`new Image()` · `image.width` · `image.height` · `image.src` · `image.complete` · `image.onload` · `image.onerror`
</details>

<details>
<summary><b>2D context</b></summary>

**State:** `fillStyle` · `strokeStyle` · `lineWidth` · `font` · `textAlign` · `textBaseline` · `globalAlpha` · `globalCompositeOperation` · `lineCap` · `lineJoin` · `miterLimit` · `shadowBlur` · `shadowColor` · `shadowOffsetX` · `shadowOffsetY` · `lineDashOffset`

**Transform:** `save` · `restore` · `translate` · `scale` · `rotate` · `transform` · `setTransform` · `resetTransform`

**Rects:** `clearRect` · `fillRect` · `strokeRect`

**Paths:** `beginPath` · `moveTo` · `lineTo` · `quadraticCurveTo` · `bezierCurveTo` · `rect` · `arc` · `arcTo` · `ellipse` · `closePath` · `clip` · `fill` · `stroke`

**Dash / style factories:** `setLineDash` · `getLineDash` · `createLinearGradient` · `createRadialGradient` · `createPattern`

**Text:** `measureText` · `fillText` · `strokeText`

**Images / pixels:** `drawImage` · `getImageData` · `putImageData`
</details>

<details>
<summary><b>Globals</b></summary>

`print(...)` · `console.log/warn/error` · `loadScript(path)` · `setTimeout` · `clearTimeout` · `requestAnimationFrame` · `cancelAnimationFrame` · `performance.now()`
</details>

## Testing

```bash
ctest --test-dir build --output-on-failure   # all tests
ctest --preset dev                            # via preset
python3 scripts/verify_compare_coverage.py    # compare-coverage check only
```

Three test classes: **smoke** (V8 host + script execution), **rendering regression** (HiDPI export, image draw, curves, ellipse, `arcTo`, `ImageData`, task-queue flush), and **compare coverage** (every `examples/echarts_*.js` wired into `web/compare/cases.json`).

## Benchmarks

How does this compare to the usual ways of rendering ECharts on a server? Architecturally:

| Backend | Render target | Output | Browser? | DOM? |
| --- | --- | --- | --- | --- |
| **Skia Painter** (this project) | Skia (native 2D) | PNG (raster, HiDPI) | No | No |
| **Headless Chrome** (Puppeteer/Playwright) | Browser compositor | PNG / screenshot | Yes (~150–300 MB) | Yes |
| **node-canvas + ECharts** | Cairo | PNG (raster) | No | No |
| **ECharts SVG SSR** | — | SVG (vector string) | No | No |

There is a **reproducible harness** in [`bench/benchmark.mjs`](bench/benchmark.mjs) that renders the *same* ECharts options through every available backend and prints a Markdown table. Any backend whose dependency is missing is skipped, never faked:

```bash
npm i -D canvas puppeteer        # optional competitors
node bench/benchmark.mjs         # ITERATIONS=50 for more samples
```

No local toolchain? The [`Benchmark` workflow](.github/workflows/benchmark.yml) builds the engine on a clean Ubuntu runner and runs the harness in the cloud (manual trigger). See [`docs/benchmarks.md`](docs/benchmarks.md) for the methodology, caveats (PNG vs SVG are different artifacts; headless Chrome is the visual *reference*), and sample numbers. **Run it on your own hardware before quoting any figure.**

## Known limitations

This runtime is a **render-and-export host**, not a browser and not a drop-in `node-canvas` replacement:

- No DOM, CSSOM or layout system
- `Image.src` only supports local file paths — no `http(s)` URLs
- No full event system (`addEventListener` is mostly a compatibility stub)
- No `Path2D`, no `isPointInPath` / `isPointInStroke`, no `createImageData`
- No full browser-grade async event loop

**Timer semantics:** `requestAnimationFrame` / `setTimeout` are no longer synchronous — they enter a minimal task queue that drains at the end of script execution and before `saveToPng()`. Enough for the ECharts/zrender refresh chain, but not a general event loop.

## Roadmap

- `Path2D`
- Fuller pixel API (e.g. `createImageData`)
- Hit-testing API
- Closer-to-browser timer / event-loop semantics
- Systematize the ECharts compatibility matrix into automated regression

## Project layout

```text
.
├── CMakeLists.txt / CMakePresets.json
├── examples/              # ECharts + smoke examples
├── include/canvas_engine/ # public headers (canvas/, runtime/)
├── scripts/               # bootstrap, build, dev server, verifiers
├── src/                   # canvas/ (2D, surface, image), runtime/ (V8 bindings)
├── web/                   # browser-vs-backend compare page
└── third_party/skia       # Skia submodule
```

Core implementation:

- 2D drawing — [`src/canvas/Canvas2DContext.cc`](src/canvas/Canvas2DContext.cc)
- Surface / PNG export — [`src/canvas/CanvasSurface.cc`](src/canvas/CanvasSurface.cc)
- Image decoding — [`src/canvas/ImageAsset.cc`](src/canvas/ImageAsset.cc)
- V8 bindings & host runtime — [`src/runtime/ScriptEngine.cc`](src/runtime/ScriptEngine.cc)

Pinned third-party versions — Skia: `31521f8508c712615b3d35e8e4554ccb5bf568e1`, V8: `15.0.39`.

## Contributing

Contributions are welcome! See [CONTRIBUTING.md](CONTRIBUTING.md) for the dev setup, the compare workflow, and how to add a new ECharts example. By participating you agree to our [Code of Conduct](CODE_OF_CONDUCT.md).

## License

Released under the [MIT License](LICENSE).
