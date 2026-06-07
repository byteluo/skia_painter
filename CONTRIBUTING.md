# Contributing to Skia Painter / 贡献指南

Thanks for your interest in improving Skia Painter! 感谢你愿意改进 Skia Painter！

[English](#english) · [中文](#中文)

---

## English

### Development setup

```bash
git clone --recursive https://github.com/byteluo/skia_painter.git
cd skia_painter
./scripts/bootstrap.sh         # installs deps, builds Skia + the engine, runs tests
```

If you already have the toolchain, build with presets:

```bash
cmake --preset dev && cmake --build --preset dev && ctest --preset dev
```

### Adding a new ECharts example

1. Add `examples/echarts_<name>.js`. Follow the pattern in existing examples: load the ECharts UMD build, build an option, `refreshImmediately()`, then `saveToPng("../output/echarts_<name>.png")`.
2. Wire the case into the compare page (`web/compare/cases.json` + the browser-side `createOption`).
3. Regenerate the PNG and run the coverage check:
   ```bash
   ./build/dev/canvas_engine examples/echarts_<name>.js
   python3 scripts/verify_compare_coverage.py
   ```
4. Verify visually with `npm run dev:compare`.

### Before opening a PR

- Run the full test suite: `ctest --preset dev --output-on-failure`
- Keep new code consistent with the style of the surrounding files.
- Commit messages: use a short, conventional prefix (`fix:`, `feat:`, `ci:`, `docs:` …).
- One logical change per PR where possible; describe **what** and **why**.

### Reporting bugs

Open an issue with: the example/script that reproduces it, the command you ran, the expected vs. actual PNG (attach images if you can), and your OS / toolchain versions.

---

## 中文

### 开发环境

```bash
git clone --recursive https://github.com/byteluo/skia_painter.git
cd skia_painter
./scripts/bootstrap.sh         # 安装依赖、构建 Skia 与引擎、运行测试
```

如果已有工具链，可直接用 preset 构建：

```bash
cmake --preset dev && cmake --build --preset dev && ctest --preset dev
```

### 新增一个 ECharts 示例

1. 添加 `examples/echarts_<name>.js`，参考现有示例：加载 ECharts UMD 产物、构造 option、`refreshImmediately()`，再 `saveToPng("../output/echarts_<name>.png")`。
2. 把用例接入对比页面（`web/compare/cases.json` 以及浏览器侧的 `createOption`）。
3. 重新生成 PNG 并跑覆盖校验：
   ```bash
   ./build/dev/canvas_engine examples/echarts_<name>.js
   python3 scripts/verify_compare_coverage.py
   ```
4. 用 `npm run dev:compare` 做视觉确认。

### 提交 PR 之前

- 跑通完整测试：`ctest --preset dev --output-on-failure`
- 新代码保持与周边文件一致的风格。
- 提交信息使用简短的约定式前缀（`fix:`、`feat:`、`ci:`、`docs:` 等）。
- 尽量一个 PR 只做一件事，说明**做了什么**以及**为什么**。

### 报告 Bug

提交 issue 时请附上：可复现的示例/脚本、运行的命令、期望与实际 PNG（尽量附图）、以及你的操作系统 / 工具链版本。
