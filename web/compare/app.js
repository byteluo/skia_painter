import { createExampleOptions } from "./cases.generated.js";

const browserChartElement = document.getElementById("browser-chart");
const engineSurfaceElement = document.getElementById("engine-surface");
const engineImageElement = document.getElementById("engine-image");
const enginePlaceholderElement = document.getElementById("engine-placeholder");
const engineLoadingElement = document.getElementById("engine-loading");
const engineLoadingTextElement = document.getElementById("engine-loading-text");
const caseListElement = document.getElementById("case-list");
const caseTitleElement = document.getElementById("case-title");
const caseDescriptionElement = document.getElementById("case-description");
const exampleScriptElement = document.getElementById("example-script");
const outputFileElement = document.getElementById("output-file");
const statusTextElement = document.getElementById("status-text");
const renderBrowserButton = document.getElementById("render-browser");
const renderEngineButton = document.getElementById("render-engine");

let compareCases = [];
let exampleOptions = {};
let currentCaseId = null;
let chart = null;
let engineRenderRequestId = 0;

function setStatus(message) {
  statusTextElement.textContent = message;
}

function setEngineLoading(isLoading, message = "后端 PNG 渲染中…") {
  engineSurfaceElement.classList.toggle("is-loading", isLoading);
  engineLoadingElement.setAttribute("aria-hidden", isLoading ? "false" : "true");
  engineLoadingTextElement.textContent = message;
}

function preloadImage(imageUrl) {
  return new Promise((resolve, reject) => {
    const image = new Image();
    image.onload = () => resolve();
    image.onerror = () => reject(new Error(`image load failed: ${imageUrl}`));
    image.src = imageUrl;
  });
}

async function loadCases() {
  const response = await fetch("cases.json");
  if (!response.ok) {
    throw new Error(`failed to load cases: ${response.status}`);
  }
  compareCases = await response.json();
  exampleOptions = createExampleOptions(window.echarts);
}

function findCase(caseId) {
  return compareCases.find((item) => item.id === caseId);
}

function renderCaseList() {
  caseListElement.innerHTML = "";

  compareCases.forEach((item) => {
    const button = document.createElement("button");
    button.type = "button";
    button.className = "case-card";
    button.dataset.caseId = item.id;
    button.innerHTML = `
      <span class="case-card__title">${item.title}</span>
      <span class="case-card__description">${item.description}</span>
    `;
    button.addEventListener("click", async () => {
      try {
        await selectCase(item.id);
      } catch (error) {
        setStatus(`切换案例失败: ${error.message}`);
      }
    });
    caseListElement.appendChild(button);
  });
}

function syncCaseSelection() {
  caseListElement.querySelectorAll(".case-card").forEach((element) => {
    element.classList.toggle("is-active", element.dataset.caseId === currentCaseId);
  });
}

function ensureChart() {
  if (!chart) {
    chart = echarts.init(browserChartElement, null, {
      renderer: "canvas"
    });
  }

  chart.resize({
    width: browserChartElement.clientWidth,
    height: browserChartElement.clientHeight
  });

  return chart;
}

function updateMeta(caseInfo) {
  caseTitleElement.textContent = caseInfo.title;
  caseDescriptionElement.textContent = caseInfo.description;
  exampleScriptElement.textContent = caseInfo.exampleScript;
  outputFileElement.textContent = caseInfo.outputFile;
}

function applyCaseActions(caseId, currentChart) {
  if (caseId === "timeline") {
    currentChart.dispatchAction({ type: "timelineChange", currentIndex: 2 });
  }

  if (caseId === "brush") {
    currentChart.dispatchAction({
      type: "brush",
      areas: [
        {
          brushType: "rect",
          coordRange: [[20, 62], [28, 72]],
          xAxisIndex: 0,
          yAxisIndex: 0
        }
      ]
    });
  }

  if (caseId === "axis_pointer") {
    currentChart.dispatchAction({
      type: "updateAxisPointer",
      currTrigger: "mousemove",
      x: 650,
      y: 250
    });
  }

  if (caseId === "axis_pointer_multigrid") {
    currentChart.dispatchAction({
      type: "updateAxisPointer",
      currTrigger: "mousemove",
      x: 650,
      y: 220
    });
  }

  if (caseId === "datazoom_markarea") {
    currentChart.dispatchAction({
      type: "dataZoom",
      start: 30,
      end: 90
    });
  }
}

function renderBrowserCase() {
  const caseInfo = findCase(currentCaseId);
  if (!caseInfo) {
    return;
  }

  const currentChart = ensureChart();
  currentChart.clear();
  const exampleCase = exampleOptions[caseInfo.id];
  currentChart.setOption(exampleCase.option, true);
  applyCaseActions(caseInfo.id, currentChart);
  setStatus(`浏览器侧已重绘: ${caseInfo.title}`);
}

async function renderEngineCase() {
  const caseInfo = findCase(currentCaseId);
  if (!caseInfo) {
    return;
  }

  const requestId = ++engineRenderRequestId;
  const imageUrl = `${caseInfo.outputFile}?t=${Date.now()}`;
  setEngineLoading(true, `加载后端 PNG: ${caseInfo.title}`);
  setStatus(`加载后端 PNG: ${caseInfo.title}`);

  try {
    await preloadImage(imageUrl);

    if (requestId !== engineRenderRequestId) {
      return;
    }

    engineImageElement.src = imageUrl;
    engineImageElement.style.display = "block";
    enginePlaceholderElement.style.display = "none";
    setStatus(`后端 PNG 已加载: ${caseInfo.title}`);
  } catch (error) {
    if (requestId === engineRenderRequestId) {
      enginePlaceholderElement.textContent = `预生成 PNG 未找到: ${caseInfo.outputFile}`;
      enginePlaceholderElement.style.display = "block";
      engineImageElement.style.display = "none";
      setStatus(`PNG 未找到: ${caseInfo.title}`);
    }
  } finally {
    if (requestId === engineRenderRequestId) {
      setEngineLoading(false);
    }
  }
}

async function selectCase(caseId) {
  currentCaseId = caseId;
  syncCaseSelection();

  const caseInfo = findCase(caseId);
  updateMeta(caseInfo);
  renderBrowserCase();
  await renderEngineCase();
}

window.addEventListener("resize", () => {
  if (chart) {
    chart.resize({
      width: browserChartElement.clientWidth,
      height: browserChartElement.clientHeight
    });
  }
});

renderBrowserButton.addEventListener("click", () => {
  try {
    renderBrowserCase();
  } catch (error) {
    setStatus(`浏览器重绘失败: ${error.message}`);
  }
});

renderEngineButton.addEventListener("click", async () => {
  try {
    await renderEngineCase();
  } catch (error) {
    setStatus(`后端渲染失败: ${error.message}`);
  }
});

async function main() {
  await loadCases();
  renderCaseList();
  await selectCase(compareCases[0].id);
}

main().catch((error) => {
  setStatus(`初始化失败: ${error.message}`);
});
