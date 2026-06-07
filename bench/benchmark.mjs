#!/usr/bin/env node
// Reproducible benchmark harness for server-side ECharts rendering.
//
// It renders the SAME set of ECharts options through several backends and
// reports the wall-clock time per chart, so the numbers are comparable.
//
// Engines are probed at runtime; any whose dependency is missing is SKIPPED
// (never faked). Install what you want to compare:
//
//   npm i -D puppeteer canvas        # node-canvas + headless Chrome
//   ./scripts/bootstrap.sh           # build ./build/dev/canvas_engine
//
// Usage:
//   node bench/benchmark.mjs                 # default 20 iterations
//   ITERATIONS=50 node bench/benchmark.mjs
//   CANVAS_ENGINE_BIN=/path/to/canvas_engine node bench/benchmark.mjs
//
// Output is a Markdown table you can paste into docs/benchmarks.md.

import { createRequire } from "node:module";
import { spawnSync } from "node:child_process";
import { mkdtempSync, writeFileSync, rmSync, existsSync } from "node:fs";
import { tmpdir } from "node:os";
import { join, dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";
import { performance } from "node:perf_hooks";

const require = createRequire(import.meta.url);
const ROOT = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const ITERATIONS = Number(process.env.ITERATIONS || 20);
const WIDTH = 960;
const HEIGHT = 540;

// ---------------------------------------------------------------------------
// Shared chart set — plain JSON-serializable ECharts options so every backend
// renders the exact same input.
// ---------------------------------------------------------------------------
const CHARTS = {
  bar: {
    xAxis: { type: "category", data: ["Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"] },
    yAxis: { type: "value" },
    series: [{ type: "bar", data: [120, 200, 150, 80, 70, 110, 130] }],
  },
  line: {
    xAxis: { type: "category", data: Array.from({ length: 50 }, (_, i) => `P${i}`) },
    yAxis: { type: "value" },
    series: [{
      type: "line",
      smooth: true,
      areaStyle: {},
      data: Array.from({ length: 50 }, (_, i) => Math.round(100 + 60 * Math.sin(i / 3))),
    }],
  },
  pie: {
    series: [{
      type: "pie",
      radius: ["35%", "70%"],
      data: [
        { value: 1048, name: "A" }, { value: 735, name: "B" },
        { value: 580, name: "C" }, { value: 484, name: "D" },
        { value: 300, name: "E" },
      ],
    }],
  },
  scatter: {
    xAxis: {}, yAxis: {},
    series: [{
      type: "scatter",
      symbolSize: 8,
      data: Array.from({ length: 1000 }, (_, i) => [i % 100, (i * 37) % 100]),
    }],
  },
  candlestick: {
    xAxis: { type: "category", data: Array.from({ length: 60 }, (_, i) => `${i}`) },
    yAxis: {},
    series: [{
      type: "candlestick",
      data: Array.from({ length: 60 }, (_, i) => {
        const o = 50 + (i % 7) * 3;
        return [o, o + 5, o - 4, o + 9];
      }),
    }],
  },
};
const CHART_NAMES = Object.keys(CHARTS);

const ECHARTS_PATH = join(ROOT, "node_modules", "echarts", "dist", "echarts.js");

function stats(samples) {
  const s = [...samples].sort((a, b) => a - b);
  const median = s[Math.floor(s.length / 2)];
  return { median, min: s[0], max: s[s.length - 1] };
}

// ---------------------------------------------------------------------------
// Engine: Skia Painter (this project) — spawns the native binary per render,
// which is how it is actually used in production.
// ---------------------------------------------------------------------------
const skiaPainter = {
  id: "skia_painter",
  label: "Skia Painter (this project)",
  bin: process.env.CANVAS_ENGINE_BIN
    || [join(ROOT, "build/dev/canvas_engine"), join(ROOT, "build/canvas_engine"), join(ROOT, "build/release/canvas_engine")].find(existsSync),
  available() {
    return Boolean(this.bin && existsSync(this.bin) && existsSync(ECHARTS_PATH));
  },
  reason() {
    if (!existsSync(ECHARTS_PATH)) return "echarts not installed (npm install)";
    return "canvas_engine binary not built (./scripts/bootstrap.sh)";
  },
  renderOnce(name, dir) {
    const out = join(dir, `${name}.png`);
    const script = `
loadScript(${JSON.stringify(ECHARTS_PATH)});
const W=${WIDTH},H=${HEIGHT};
const canvas=new Canvas(W,H);canvas.style={};
echarts.setPlatformAPI({
  createCanvas(){const c=new Canvas(W,H);c.style={};return c;},
  loadImage(src,onload){const im=new Image();if(typeof onload==='function')im.onload=onload;im.src=src;return im;}
});
const chart=echarts.init(canvas,null,{renderer:'canvas',width:W,height:H});
chart.setOption(${JSON.stringify(CHARTS[name])});
chart.getZr().refreshImmediately();
canvas.saveToPng(${JSON.stringify(out)});
`;
    const scriptPath = join(dir, `${name}.js`);
    writeFileSync(scriptPath, script);
    const t0 = performance.now();
    const r = spawnSync(this.bin, [scriptPath], { stdio: "ignore" });
    const dt = performance.now() - t0;
    if (r.status !== 0) throw new Error(`canvas_engine exited ${r.status}`);
    return dt;
  },
};

// ---------------------------------------------------------------------------
// Engine: node-canvas + ECharts (Cairo-backed raster)
// ---------------------------------------------------------------------------
const nodeCanvas = {
  id: "node_canvas",
  label: "node-canvas + ECharts",
  available() {
    try { require.resolve("canvas"); require.resolve("echarts"); return true; }
    catch { return false; }
  },
  reason() { return "npm i -D canvas"; },
  async setup() {
    const { createCanvas } = require("canvas");
    this.echarts = require("echarts");
    this.createCanvas = createCanvas;
  },
  renderOnce(name) {
    const t0 = performance.now();
    const canvas = this.createCanvas(WIDTH, HEIGHT);
    const chart = this.echarts.init(canvas);
    chart.setOption(CHARTS[name]);
    const buf = canvas.toBuffer("image/png");
    chart.dispose();
    void buf;
    return performance.now() - t0;
  },
};

// ---------------------------------------------------------------------------
// Engine: ECharts SVG server-side rendering (vector string, no raster)
// ---------------------------------------------------------------------------
const echartsSvg = {
  id: "echarts_svg",
  label: "ECharts SVG SSR",
  available() {
    try { require.resolve("echarts"); return true; } catch { return false; }
  },
  reason() { return "npm install"; },
  async setup() { this.echarts = require("echarts"); },
  renderOnce(name) {
    const t0 = performance.now();
    const chart = this.echarts.init(null, null, {
      renderer: "svg", ssr: true, width: WIDTH, height: HEIGHT,
    });
    chart.setOption(CHARTS[name]);
    const svg = chart.renderToSVGString();
    chart.dispose();
    void svg;
    return performance.now() - t0;
  },
};

// ---------------------------------------------------------------------------
// Engine: Headless Chrome via Puppeteer (the reference, native browser canvas)
// ---------------------------------------------------------------------------
const puppeteerEngine = {
  id: "puppeteer",
  label: "Headless Chrome (Puppeteer)",
  available() {
    try { require.resolve("puppeteer"); return existsSync(ECHARTS_PATH); }
    catch { return false; }
  },
  reason() { return "npm i -D puppeteer"; },
  async setup() {
    const puppeteer = require("puppeteer");
    this.browser = await puppeteer.launch({ args: ["--no-sandbox"] });
    this.page = await this.browser.newPage();
    const echartsSource = require("node:fs").readFileSync(ECHARTS_PATH, "utf8");
    await this.page.setContent(`<!doctype html><body><div id="c" style="width:${WIDTH}px;height:${HEIGHT}px"></div></body>`);
    await this.page.addScriptTag({ content: echartsSource });
  },
  async renderOnce(name) {
    return await this.page.evaluate(async (option, w, h) => {
      const t0 = performance.now();
      const el = document.getElementById("c");
      const chart = window.echarts.init(el, null, { renderer: "canvas", width: w, height: h });
      chart.setOption(option);
      const url = chart.getDataURL({ type: "png" });
      chart.dispose();
      void url;
      return performance.now() - t0;
    }, CHARTS[name], WIDTH, HEIGHT);
  },
  async teardown() { if (this.browser) await this.browser.close(); },
};

const ENGINES = [skiaPainter, nodeCanvas, echartsSvg, puppeteerEngine];

async function run() {
  console.log(`# Benchmark — ${ITERATIONS} iterations, ${WIDTH}x${HEIGHT} logical px\n`);
  const dir = mkdtempSync(join(tmpdir(), "skia-bench-"));
  const rows = [];

  for (const engine of ENGINES) {
    if (!engine.available()) {
      console.log(`- SKIP ${engine.label} — ${engine.reason()}`);
      continue;
    }
    try {
      if (engine.setup) await engine.setup();
      // warmup
      for (const name of CHART_NAMES) await engine.renderOnce(name, dir);

      const perChart = {};
      let allSamples = [];
      for (const name of CHART_NAMES) {
        const samples = [];
        for (let i = 0; i < ITERATIONS; i++) samples.push(await engine.renderOnce(name, dir));
        perChart[name] = stats(samples).median;
        allSamples = allSamples.concat(samples);
      }
      const overall = stats(allSamples).median;
      rows.push({ label: engine.label, perChart, overall });
      console.log(`- OK   ${engine.label}`);
    } catch (err) {
      console.log(`- FAIL ${engine.label} — ${err.message}`);
    } finally {
      if (engine.teardown) await engine.teardown();
    }
  }

  rmSync(dir, { recursive: true, force: true });

  if (!rows.length) {
    console.log("\nNo engines available. Install at least one and re-run.");
    return;
  }

  console.log(`\n## Results (median ms/chart, lower is better)\n`);
  const header = ["Backend", ...CHART_NAMES, "**median**"];
  console.log(`| ${header.join(" | ")} |`);
  console.log(`| ${header.map(() => "---").join(" | ")} |`);
  for (const r of rows) {
    const cells = CHART_NAMES.map((n) => r.perChart[n].toFixed(1));
    console.log(`| ${r.label} | ${cells.join(" | ")} | **${r.overall.toFixed(1)}** |`);
  }
  console.log(`\n_Environment: Node ${process.version}, ${process.platform}/${process.arch}. Numbers vary by machine — run locally._`);
}

run().catch((e) => { console.error(e); process.exit(1); });
