const browserChartElement = document.getElementById("browser-chart");
const engineImageElement = document.getElementById("engine-image");
const enginePlaceholderElement = document.getElementById("engine-placeholder");
const caseListElement = document.getElementById("case-list");
const caseTitleElement = document.getElementById("case-title");
const caseDescriptionElement = document.getElementById("case-description");
const exampleScriptElement = document.getElementById("example-script");
const outputFileElement = document.getElementById("output-file");
const statusTextElement = document.getElementById("status-text");
const renderBrowserButton = document.getElementById("render-browser");
const renderEngineButton = document.getElementById("render-engine");

let compareCases = [];
let currentCaseId = null;
let chart = null;

function setStatus(message) {
  statusTextElement.textContent = message;
}

async function loadCases() {
  const response = await fetch("/api/cases");
  if (!response.ok) {
    throw new Error(`failed to load cases: ${response.status}`);
  }
  compareCases = await response.json();
}

function findCase(caseId) {
  return compareCases.find((item) => item.id === caseId);
}

function makeImageScatterMarker() {
  const marker = document.createElement("canvas");
  marker.width = 96;
  marker.height = 96;

  const ctx = marker.getContext("2d");
  ctx.fillStyle = "#f8fafc";
  ctx.beginPath();
  ctx.arc(48, 48, 42, 0, Math.PI * 2);
  ctx.fill();

  ctx.strokeStyle = "#0f172a";
  ctx.lineWidth = 6;
  ctx.stroke();

  ctx.fillStyle = "#2563eb";
  ctx.beginPath();
  ctx.arc(48, 40, 16, 0, Math.PI * 2);
  ctx.fill();

  ctx.fillStyle = "#f97316";
  ctx.beginPath();
  ctx.arc(48, 72, 18, Math.PI, Math.PI * 2);
  ctx.fill();

  return marker.toDataURL("image/png");
}

function createSunburstOption() {
  const sunburstData = [
    {
      name: "Traffic",
      value: 68,
      itemStyle: { color: "#2563eb" },
      children: [
        {
          name: "Search",
          value: 42,
          itemStyle: { color: "#3b82f6" },
          children: [
            { name: "Organic", value: 24, itemStyle: { color: "#60a5fa" } },
            { name: "Ads", value: 18, itemStyle: { color: "#93c5fd" } }
          ]
        },
        {
          name: "Social",
          value: 26,
          itemStyle: { color: "#1d4ed8" },
          children: [
            { name: "Video", value: 12, itemStyle: { color: "#38bdf8" } },
            { name: "Community", value: 14, itemStyle: { color: "#0ea5e9" } }
          ]
        }
      ]
    },
    {
      name: "Sales",
      value: 56,
      itemStyle: { color: "#f97316" },
      children: [
        {
          name: "Direct",
          value: 34,
          itemStyle: { color: "#fb923c" },
          children: [
            { name: "Store", value: 15, itemStyle: { color: "#fdba74" } },
            { name: "Partners", value: 19, itemStyle: { color: "#fed7aa" } }
          ]
        },
        {
          name: "Campaigns",
          value: 22,
          itemStyle: { color: "#ea580c" },
          children: [
            { name: "Launch", value: 10, itemStyle: { color: "#fb7185" } },
            { name: "Seasonal", value: 12, itemStyle: { color: "#f43f5e" } }
          ]
        }
      ]
    },
    {
      name: "Retention",
      value: 46,
      itemStyle: { color: "#14b8a6" },
      children: [
        {
          name: "Product",
          value: 28,
          itemStyle: { color: "#2dd4bf" },
          children: [
            { name: "Quality", value: 16, itemStyle: { color: "#5eead4" } },
            { name: "Pricing", value: 12, itemStyle: { color: "#99f6e4" } }
          ]
        },
        {
          name: "Support",
          value: 18,
          itemStyle: { color: "#0f766e" },
          children: [
            { name: "Docs", value: 8, itemStyle: { color: "#34d399" } },
            { name: "Success", value: 10, itemStyle: { color: "#10b981" } }
          ]
        }
      ]
    }
  ];

  return {
    animation: false,
    backgroundColor: "#fffaf0",
    title: {
      text: "Sunburst Compatibility",
      subtext: "Sector layout + rotated labels",
      left: "center",
      top: 18
    },
    series: [
      {
        type: "sunburst",
        radius: ["16%", "78%"],
        center: ["50%", "57%"],
        sort: null,
        nodeClick: false,
        itemStyle: {
          borderColor: "#fffaf0",
          borderWidth: 3
        },
        label: {
          rotate: "radial",
          color: "#111827",
          fontSize: 12
        },
        levels: [
          {},
          { r0: "16%", r: "36%", label: { rotate: 0, fontWeight: "bold" } },
          { r0: "36%", r: "58%" },
          { r0: "58%", r: "78%", label: { fontSize: 10 } }
        ],
        data: sunburstData
      }
    ]
  };
}

function createTimelineOption() {
  return {
    baseOption: {
      animation: false,
      backgroundColor: "#f8fafc",
      timeline: {
        axisType: "category",
        autoPlay: false,
        top: 18,
        left: 80,
        right: 80,
        data: ["Q1", "Q2", "Q3"],
        currentIndex: 2
      },
      title: {
        text: "Timeline Compatibility",
        subtext: "Timeline component + option switching",
        left: "center",
        top: 64
      },
      grid: {
        left: 80,
        right: 56,
        top: 150,
        bottom: 60
      },
      xAxis: {
        type: "value"
      },
      yAxis: {
        type: "category"
      },
      series: [
        {
          type: "bar",
          label: {
            show: true,
            position: "right"
          },
          itemStyle: {
            borderRadius: [0, 8, 8, 0]
          }
        }
      ]
    },
    options: [
      {
        yAxis: { data: ["Search", "Direct", "Ads", "Email"] },
        series: [{ data: [120, 90, 66, 40], itemStyle: { color: "#2563eb" } }]
      },
      {
        yAxis: { data: ["Search", "Direct", "Ads", "Email"] },
        series: [{ data: [138, 102, 72, 52], itemStyle: { color: "#14b8a6" } }]
      },
      {
        yAxis: { data: ["Search", "Direct", "Ads", "Email"] },
        series: [{ data: [154, 118, 88, 60], itemStyle: { color: "#f97316" } }]
      }
    ]
  };
}

function createCustomSeriesOption() {
  const data = [
    ["Alpha", 12, 26, "#2563eb"],
    ["Beta", 28, 18, "#14b8a6"],
    ["Gamma", 44, 34, "#f97316"],
    ["Delta", 60, 22, "#8b5cf6"],
    ["Epsilon", 76, 30, "#e11d48"]
  ];

  return {
    animation: false,
    backgroundColor: "#f8fafc",
    title: {
      text: "Custom Series Compatibility",
      subtext: "renderItem group + shapes + text",
      left: "center",
      top: 18
    },
    grid: {
      left: 80,
      right: 56,
      top: 96,
      bottom: 60
    },
    xAxis: {
      min: 0,
      max: 90,
      axisLabel: {
        color: "#334155"
      },
      splitLine: {
        lineStyle: {
          color: "#dbe4f0",
          type: "dashed"
        }
      }
    },
    yAxis: {
      min: 0,
      max: 40,
      axisLabel: {
        color: "#334155"
      },
      splitLine: {
        lineStyle: {
          color: "#dbe4f0",
          type: "dashed"
        }
      }
    },
    series: [
      {
        type: "custom",
        renderItem(params, api) {
          const category = api.value(0);
          const x = api.value(1);
          const y = api.value(2);
          const color = api.value(3);
          const point = api.coord([x, y]);

          return {
            type: "group",
            children: [
              {
                type: "circle",
                shape: {
                  cx: point[0],
                  cy: point[1],
                  r: 24
                },
                style: {
                  fill: color,
                  stroke: "#ffffff",
                  lineWidth: 3,
                  shadowBlur: 16,
                  shadowColor: "rgba(15, 23, 42, 0.14)"
                }
              },
              {
                type: "text",
                style: {
                  x: point[0],
                  y: point[1] + 2,
                  text: category,
                  textAlign: "center",
                  textVerticalAlign: "middle",
                  fill: "#ffffff",
                  font: "bold 13px sans-serif"
                }
              }
            ]
          };
        },
        data
      }
    ]
  };
}

function createPictorialBarOption() {
  const values = [9, 14, 18, 22, 26];

  return {
    animation: false,
    backgroundColor: "#fffaf0",
    title: {
      text: "Pictorial Bar Compatibility",
      subtext: "Repeated symbols + clipping",
      left: "center",
      top: 18
    },
    grid: {
      left: 80,
      right: 60,
      top: 112,
      bottom: 56
    },
    xAxis: {
      type: "category",
      data: ["Mon", "Tue", "Wed", "Thu", "Fri"],
      axisLabel: {
        color: "#334155"
      }
    },
    yAxis: {
      type: "value",
      min: 0,
      max: 28,
      splitLine: {
        lineStyle: {
          color: "#e5e7eb",
          type: "dashed"
        }
      },
      axisLabel: {
        color: "#334155"
      }
    },
    series: [
      {
        type: "pictorialBar",
        symbol: "roundRect",
        symbolSize: [28, 12],
        symbolRepeat: true,
        symbolMargin: 3,
        symbolClip: true,
        itemStyle: {
          color: "#2563eb",
          borderRadius: 6
        },
        data: values,
        z: 10
      },
      {
        type: "pictorialBar",
        symbol: "roundRect",
        symbolSize: [28, 12],
        symbolRepeat: true,
        symbolMargin: 3,
        symbolBoundingData: 28,
        itemStyle: {
          color: "rgba(37, 99, 235, 0.12)",
          borderRadius: 6
        },
        data: new Array(values.length).fill(28),
        z: 1
      }
    ]
  };
}

function createGaugeOption() {
  return {
    animation: false,
    backgroundColor: "#f8fafc",
    title: {
      text: "Gauge Compatibility",
      subtext: "Axis arcs + pointer + detail text",
      left: "center",
      top: 18
    },
    series: [
      {
        type: "gauge",
        center: ["50%", "58%"],
        radius: "72%",
        startAngle: 210,
        endAngle: -30,
        min: 0,
        max: 100,
        splitNumber: 5,
        axisLine: {
          roundCap: true,
          lineStyle: {
            width: 24,
            color: [
              [0.35, "#38bdf8"],
              [0.7, "#22c55e"],
              [1, "#f97316"]
            ]
          }
        },
        progress: {
          show: true,
          roundCap: true,
          width: 24,
          itemStyle: {
            color: "#2563eb"
          }
        },
        pointer: {
          show: true,
          length: "62%",
          width: 8,
          itemStyle: {
            color: "#0f172a"
          }
        },
        anchor: {
          show: true,
          size: 18,
          itemStyle: {
            color: "#0f172a"
          }
        },
        axisTick: {
          distance: -32,
          splitNumber: 5,
          lineStyle: {
            width: 2,
            color: "#cbd5e1"
          }
        },
        splitLine: {
          distance: -34,
          length: 18,
          lineStyle: {
            width: 4,
            color: "#94a3b8"
          }
        },
        axisLabel: {
          distance: -54,
          color: "#334155",
          fontSize: 14
        },
        detail: {
          valueAnimation: false,
          offsetCenter: [0, "48%"],
          formatter(value) {
            return `{value|${value}}\n{label|Canvas score}`;
          },
          rich: {
            value: {
              fontSize: 46,
              fontWeight: "bold",
              color: "#0f172a",
              lineHeight: 54
            },
            label: {
              fontSize: 15,
              color: "#64748b",
              lineHeight: 20
            }
          }
        },
        title: {
          offsetCenter: [0, "22%"],
          color: "#334155",
          fontSize: 18
        },
        data: [
          {
            value: 82,
            name: "Stability"
          }
        ]
      }
    ]
  };
}

function createImageScatterOption() {
  return {
    animation: false,
    backgroundColor: "#f8fafc",
    title: {
      text: "Image Marker Scatter",
      subtext: "drawImage compatibility",
      left: "center",
      top: 24
    },
    grid: {
      left: 72,
      right: 48,
      top: 96,
      bottom: 64
    },
    tooltip: {
      trigger: "item"
    },
    xAxis: {
      type: "category",
      data: ["A", "B", "C", "D", "E"],
      axisLabel: {
        color: "#334155"
      }
    },
    yAxis: {
      type: "value",
      min: 0,
      max: 100,
      axisLabel: {
        color: "#334155"
      },
      splitLine: {
        lineStyle: {
          color: "#dbe4f0",
          type: "dashed"
        }
      }
    },
    series: [
      {
        type: "scatter",
        symbol: `image://${makeImageScatterMarker()}`,
        symbolSize: 44,
        data: [
          ["A", 24],
          ["B", 52],
          ["C", 38],
          ["D", 76],
          ["E", 61]
        ]
      }
    ]
  };
}

function createOption(caseId) {
  switch (caseId) {
    case "sunburst":
      return createSunburstOption();
    case "timeline":
      return createTimelineOption();
    case "custom_series":
      return createCustomSeriesOption();
    case "pictorial_bar":
      return createPictorialBarOption();
    case "gauge":
      return createGaugeOption();
    case "image_scatter":
      return createImageScatterOption();
    default:
      throw new Error(`unknown case: ${caseId}`);
  }
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
    button.addEventListener("click", () => selectCase(item.id));
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

function renderBrowserCase() {
  const caseInfo = findCase(currentCaseId);
  if (!caseInfo) {
    return;
  }

  const currentChart = ensureChart();
  currentChart.clear();
  currentChart.setOption(createOption(caseInfo.id), true);
  setStatus(`浏览器侧已重绘: ${caseInfo.title}`);
}

async function renderEngineCase() {
  const caseInfo = findCase(currentCaseId);
  if (!caseInfo) {
    return;
  }

  setStatus(`后端渲染中: ${caseInfo.title}`);

  const response = await fetch(`/api/render/${caseInfo.id}`, {
    method: "POST"
  });
  const payload = await response.json();

  if (!response.ok || !payload.ok) {
    throw new Error(payload.error || `render failed: ${response.status}`);
  }

  engineImageElement.src = payload.imageUrl;
  engineImageElement.style.display = "block";
  enginePlaceholderElement.style.display = "none";
  setStatus(`后端 PNG 已更新: ${caseInfo.title}`);
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
