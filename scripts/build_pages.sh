#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
DIST_DIR="$ROOT_DIR/dist"

rm -rf "$DIST_DIR"
mkdir -p "$DIST_DIR"

# Copy static assets
cp "$ROOT_DIR/web/compare/styles.css" "$DIST_DIR/styles.css"
cp "$ROOT_DIR/web/compare/cases.json"  "$DIST_DIR/cases.json"
cp "$ROOT_DIR/web/compare/app.js"      "$DIST_DIR/app.js"

# Create index.html with relative paths and CDN echarts
cat > "$DIST_DIR/index.html" << 'HTMLEOF'
<!doctype html>
<html lang="zh-CN">
  <head>
    <meta charset="utf-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <title>Skia Painter Compare</title>
    <link rel="stylesheet" href="styles.css" />
  </head>
  <body>
    <div class="shell">
      <aside class="sidebar">
        <div class="sidebar__head">
          <p class="eyebrow">Skia Painter</p>
          <h1>Canvas Compare Lab</h1>
          <p class="sidebar__copy">
            上面用浏览器里的 ECharts 直接渲染，下面用当前 C++/V8/Skia 引擎导出的 PNG，对比视觉结果。
          </p>
        </div>
        <div id="case-list" class="case-list"></div>
      </aside>

      <main class="main">
        <section class="toolbar">
          <div class="toolbar__info">
            <p class="eyebrow">Current Case</p>
            <h2 id="case-title">Loading…</h2>
            <p id="case-description" class="muted"></p>
          </div>
          <div class="toolbar__actions">
            <button id="render-browser" class="button button--secondary" type="button">
              重绘浏览器侧
            </button>
            <button id="render-engine" class="button" type="button" disabled title="GitHub Pages 静态部署不支持后端渲染">
              渲染后端 PNG
            </button>
          </div>
        </section>

        <section class="meta">
          <div class="meta__item">
            <span class="meta__label">Example</span>
            <code id="example-script"></code>
          </div>
          <div class="meta__item">
            <span class="meta__label">PNG</span>
            <code id="output-file"></code>
          </div>
          <div class="meta__item">
            <span class="meta__label">Status</span>
            <span id="status-text">Idle</span>
          </div>
          <div class="meta__item">
            <span class="meta__label">Preview Size</span>
            <span>统一按 16:9 视口缩放展示</span>
          </div>
        </section>

        <section class="compare-stack">
          <article class="panel">
            <header class="panel__head">
              <p class="eyebrow">Browser</p>
              <h3>ECharts Runtime</h3>
            </header>
            <div class="viewport">
              <div class="stage">
                <div id="browser-chart" class="viewport__surface viewport__surface--chart"></div>
              </div>
            </div>
          </article>

          <article class="panel">
            <header class="panel__head">
              <p class="eyebrow">Backend</p>
              <h3>Skia PNG Export</h3>
            </header>
            <div class="viewport">
              <div class="stage">
                <div id="engine-surface" class="viewport__surface viewport__surface--image">
                  <img id="engine-image" alt="Engine render output" />
                  <div id="engine-placeholder" class="placeholder">
                    后端渲染功能仅在本地开发服务器可用
                  </div>
                  <div id="engine-loading" class="loading-overlay" aria-live="polite">
                    <div class="loading-spinner" aria-hidden="true"></div>
                    <div id="engine-loading-text" class="loading-text">后端 PNG 渲染中…</div>
                  </div>
                </div>
              </div>
            </div>
          </article>
        </section>
      </main>
    </div>

    <script src="https://cdn.jsdelivr.net/npm/echarts@6.0.0/dist/echarts.min.js"></script>
    <script type="module" src="app.js"></script>
  </body>
</html>
HTMLEOF

# Patch app.js: replace /api/cases fetch with relative cases.json
sed -i 's|fetch("/api/cases")|fetch("cases.json")|g' "$DIST_DIR/app.js"

echo "Build complete: $DIST_DIR"
ls -la "$DIST_DIR"
