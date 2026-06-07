# Benchmarks / 性能对比

[English](#english) · [中文](#中文)

> **Honesty note.** The harness in [`bench/benchmark.mjs`](../bench/benchmark.mjs) is fully reproducible. Any backend whose dependency is missing is **skipped, never faked**. The numbers below are illustrative of one machine — always re-run on your own hardware before quoting them.

---

## English

### What is being compared

Server-side ECharts rendering is usually done one of these ways:

| Backend | Render target | Output | Browser? | DOM? |
| --- | --- | --- | --- | --- |
| **Skia Painter** (this project) | Skia (native 2D) | PNG (raster, HiDPI) | No | No |
| **Headless Chrome** (Puppeteer/Playwright) | Browser compositor | PNG / screenshot | Yes (~150–300 MB) | Yes |
| **node-canvas + ECharts** | Cairo | PNG (raster) | No | No |
| **ECharts SVG SSR** | — | SVG (vector string) | No | No |

These are **not** identical workloads — a raster PNG and an SVG string are different artifacts, and headless Chrome is the visual *reference* implementation. Read the table as "cost of producing one rendered chart with each approach", not as a single apples-to-apples score.

### Methodology

- The harness renders the **same five ECharts options** (`bar`, `line`, `pie`, `scatter` with 1k points, `candlestick`) through every available backend.
- 1 warmup pass, then `ITERATIONS` (default 20) timed passes per chart; we report the **median ms per chart** (robust to outliers) and the overall median.
- Skia Painter is timed **including process spawn**, because that is how it is used in production (one binary invocation per chart). The in-process Node backends are timed around `setOption` + export only — so the comparison is, if anything, conservative toward Skia Painter.
- Headless Chrome reuses a single warm browser/page across renders (best case for it; excludes the multi-hundred-MB cold start).

### How to run

```bash
npm install                      # echarts (already a dependency)
npm i -D canvas puppeteer        # optional competitors
./scripts/bootstrap.sh           # build ./build/dev/canvas_engine

node bench/benchmark.mjs                 # 20 iterations
ITERATIONS=50 node bench/benchmark.mjs   # more samples
CANVAS_ENGINE_BIN=/path node bench/benchmark.mjs   # custom binary
```

The harness prints a Markdown table you can paste straight back into this file.

**No local toolchain?** Trigger the [`Benchmark` GitHub Actions workflow](../.github/workflows/benchmark.yml) (`workflow_dispatch`). It builds Skia + V8 + the engine on a clean Ubuntu runner, installs the competitors, runs the harness and prints the table to the job summary — so you get the engine's own numbers without building anything locally.

### Sample results

> Real numbers from one run (50 iterations, median ms per chart, lower is
> better). `—` means that backend was not measured in this run; build the
> binary / install the dep and re-run to fill it in. Re-measure on your own
> hardware before quoting anything.

```
# Benchmark — 50 iterations, 960x540 logical px

| Backend                         | bar  | line | pie  | scatter | candlestick | median |
| ------------------------------- | ---- | ---- | ---- | ------- | ----------- | ------ |
| Skia Painter (this project)     |  —   |  —   |  —   |   —     |     —       |   —    |  (build the binary)
| node-canvas + ECharts           | 13.0 | 16.4 | 13.3 |  23.8   |    13.8     |  14.2  |
| ECharts SVG SSR                 |  1.7 |  2.6 |  1.3 |  18.6   |     2.6     |   2.5  |
| Headless Chrome (Puppeteer)     |  —   |  —   |  —   |   —     |     —       |   —    |  (Chromium not available here)

Environment: Node v22.22.3, linux/x64, i5-12600K.
```

### How to read it

- **SVG SSR** is the cheapest because it never rasterizes — but it emits a vector string, not a pixel buffer, and downstream you still pay to rasterize it if you need an image.
- **Headless Chrome** is the fidelity reference, but carries the heaviest footprint (a full browser process, hundreds of MB RSS, and a slow cold start that this table deliberately excludes).
- **Skia Painter** trades the browser away for a single native binary: no Chromium download, no display server, no DOM — at the cost of being a render-and-export host rather than a full browser environment (see [Known limitations](../README.md#known-limitations)).

---

## 中文

### 对比对象

服务端渲染 ECharts 通常有这几条路：

| 方案 | 渲染目标 | 输出 | 需要浏览器？ | 需要 DOM？ |
| --- | --- | --- | --- | --- |
| **Skia Painter**（本项目） | Skia（原生 2D） | PNG（栅格，高清） | 否 | 否 |
| **无头 Chrome**（Puppeteer/Playwright） | 浏览器合成器 | PNG / 截图 | 是（约 150–300 MB） | 是 |
| **node-canvas + ECharts** | Cairo | PNG（栅格） | 否 | 否 |
| **ECharts SVG SSR** | — | SVG（矢量字符串） | 否 | 否 |

这些**并非完全等价的负载**——栅格 PNG 和 SVG 字符串是不同的产物，而无头 Chrome 是视觉上的*参考实现*。这张表应理解为“用每种方案产出一张图的成本”，而不是一个严格同口径的单一分数。

### 方法学

- harness 把**同一组 5 个 ECharts option**（`bar`、`line`、`pie`、1000 点 `scatter`、`candlestick`）喂给每个可用后端。
- 先 1 次预热，再对每张图跑 `ITERATIONS`（默认 20）次计时，报告**每张图的中位耗时**（抗离群）以及总体中位数。
- Skia Painter 的计时**包含进程启动开销**，因为它在生产里就是这样用的（每张图一次二进制调用）。Node 内的后端只计 `setOption` + 导出这一段——所以这个对比即使有偏，也是偏向不利于 Skia Painter 的（更保守）。
- 无头 Chrome 复用同一个已预热的浏览器/页面（对它最有利，排除了几百 MB 的冷启动）。

### 如何运行

```bash
npm install                      # echarts（已是依赖）
npm i -D canvas puppeteer        # 可选的对比方案
./scripts/bootstrap.sh           # 构建 ./build/dev/canvas_engine

node bench/benchmark.mjs                 # 20 次迭代
ITERATIONS=50 node bench/benchmark.mjs   # 更多采样
CANVAS_ENGINE_BIN=/path node bench/benchmark.mjs   # 自定义二进制
```

harness 会打印一张 Markdown 表，可直接粘回本文件。

**本地没有工具链？** 手动触发 [`Benchmark` GitHub Actions workflow](../.github/workflows/benchmark.yml)（`workflow_dispatch`）。它会在干净的 Ubuntu runner 上构建 Skia + V8 + 引擎、安装对比方案、跑 harness 并把表格输出到作业摘要——无需在本地构建任何东西就能拿到本引擎自己的数字。

### 示例结果

> 某次真实运行的结果（50 次迭代，每张图中位耗时，越低越好）。`—` 表示该后端
> 本次未测量；构建二进制 / 安装对应依赖后重跑即可补齐。引用前请在你自己的
> 硬件上重新测量。

```
# Benchmark — 50 iterations, 960x540 logical px

| Backend                         | bar  | line | pie  | scatter | candlestick | median |
| ------------------------------- | ---- | ---- | ---- | ------- | ----------- | ------ |
| Skia Painter (this project)     |  —   |  —   |  —   |   —     |     —       |   —    |  (需先构建二进制)
| node-canvas + ECharts           | 13.0 | 16.4 | 13.3 |  23.8   |    13.8     |  14.2  |
| ECharts SVG SSR                 |  1.7 |  2.6 |  1.3 |  18.6   |     2.6     |   2.5  |
| Headless Chrome (Puppeteer)     |  —   |  —   |  —   |   —     |     —       |   —    |  (此环境无 Chromium)

Environment: Node v22.22.3, linux/x64, i5-12600K.
```

### 怎么读这张表

- **SVG SSR** 最便宜，因为它根本不栅格化——但它产出的是矢量字符串而非像素缓冲，下游若需要图片仍要再付一次栅格化成本。
- **无头 Chrome** 是保真度参考，但代价最重（完整浏览器进程、几百 MB RSS、以及本表刻意排除掉的缓慢冷启动）。
- **Skia Painter** 用单个原生二进制换掉浏览器：不下载 Chromium、不需要显示服务器、没有 DOM——代价是它只是一个“渲染导出宿主”，而非完整浏览器环境（见 [已知限制](../README.md#已知限制)）。
