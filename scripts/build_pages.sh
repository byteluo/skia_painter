#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
DIST_DIR="$ROOT_DIR/dist"
ENGINE="${CANVAS_ENGINE_BIN:-$ROOT_DIR/build/release/canvas_engine}"

if [[ ! -x "$ENGINE" ]]; then
  printf 'canvas_engine is not executable: %s\n' "$ENGINE" >&2
  printf 'Build it first or set CANVAS_ENGINE_BIN.\n' >&2
  exit 1
fi
if [[ ! -f "$ROOT_DIR/node_modules/echarts/dist/echarts.js" ]]; then
  printf 'ECharts is missing; run npm ci first.\n' >&2
  exit 1
fi

rm -rf "$DIST_DIR"
mkdir -p "$DIST_DIR"

cp "$ROOT_DIR/web/compare/index.html" "$DIST_DIR/index.html"
cp "$ROOT_DIR/web/compare/styles.css" "$DIST_DIR/styles.css"
cp "$ROOT_DIR/web/compare/app.js" "$DIST_DIR/app.js"
cp "$ROOT_DIR/web/compare/cases.json" "$DIST_DIR/cases.json"
cp "$ROOT_DIR/node_modules/echarts/dist/echarts.js" "$DIST_DIR/echarts.js"

python3 "$ROOT_DIR/scripts/generate_compare_cases.py" \
  --output "$DIST_DIR/cases.generated.js"
python3 "$ROOT_DIR/scripts/render_pages_examples.py" \
  --engine "$ENGINE" \
  --output-dir "$DIST_DIR/output" \
  --work-dir "$ROOT_DIR/build/pages-render"

printf 'Build complete: %s\n' "$DIST_DIR"
find "$DIST_DIR" -maxdepth 2 -type f -printf '%P\n' | sort
