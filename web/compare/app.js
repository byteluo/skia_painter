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

function createBarOption() {
  return {
    animation: false,
    backgroundColor: "#f8fafc",
    title: {
      text: "Skia + V8 + ECharts",
      left: "center",
      textStyle: {
        color: "#0f172a",
        fontSize: 24,
        fontWeight: "bold"
      }
    },
    grid: {
      left: 80,
      right: 40,
      top: 90,
      bottom: 70
    },
    tooltip: {},
    xAxis: {
      type: "category",
      data: ["Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"],
      axisLine: {
        lineStyle: {
          color: "#475569"
        }
      },
      axisLabel: {
        color: "#334155"
      }
    },
    yAxis: {
      type: "value",
      axisLine: {
        show: true,
        lineStyle: {
          color: "#475569"
        }
      },
      splitLine: {
        lineStyle: {
          color: "#cbd5e1"
        }
      },
      axisLabel: {
        color: "#334155"
      }
    },
    series: [
      {
        name: "Visits",
        type: "bar",
        barWidth: 42,
        data: [150, 230, 224, 218, 135, 147, 260],
        itemStyle: {
          color: "#2563eb"
        }
      }
    ]
  };
}

function createLineOption() {
  return {
    animation: false,
    backgroundColor: "#f8fafc",
    title: {
      text: "Weekly Traffic Trend",
      left: "center",
      textStyle: {
        color: "#0f172a",
        fontSize: 24,
        fontWeight: "bold"
      }
    },
    grid: {
      left: 72,
      right: 48,
      top: 96,
      bottom: 64
    },
    xAxis: {
      type: "category",
      boundaryGap: false,
      data: ["Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"],
      axisLine: {
        lineStyle: {
          color: "#64748b",
          type: "dashed"
        }
      },
      splitLine: {
        show: true,
        lineStyle: {
          color: "#cbd5e1",
          type: "dashed"
        }
      },
      axisLabel: {
        color: "#334155"
      }
    },
    yAxis: {
      type: "value",
      axisLine: {
        show: true,
        lineStyle: {
          color: "#64748b"
        }
      },
      splitLine: {
        lineStyle: {
          color: "#cbd5e1",
          type: [6, 4]
        }
      },
      axisLabel: {
        color: "#334155"
      }
    },
    series: [
      {
        name: "Visits",
        type: "line",
        smooth: true,
        symbol: "circle",
        symbolSize: 10,
        lineStyle: {
          color: "#2563eb",
          width: 4,
          type: "dashed"
        },
        itemStyle: {
          color: "#1d4ed8"
        },
        areaStyle: {
          color: new echarts.graphic.LinearGradient(0, 0, 0, 1, [
            { offset: 0, color: "rgba(37, 99, 235, 0.35)" },
            { offset: 1, color: "rgba(37, 99, 235, 0.04)" }
          ])
        },
        data: [180, 232, 201, 234, 290, 330, 310]
      }
    ]
  };
}

function createPieOption() {
  return {
    animation: false,
    backgroundColor: "#fffaf0",
    title: {
      text: "Traffic Sources",
      subtext: "Pie compatibility",
      left: "center",
      top: 24,
      textStyle: {
        color: "#1f2937",
        fontSize: 24,
        fontWeight: "bold"
      },
      subtextStyle: {
        color: "#6b7280",
        fontSize: 12
      }
    },
    tooltip: {
      trigger: "item"
    },
    legend: {
      orient: "vertical",
      right: 32,
      top: "middle",
      textStyle: {
        color: "#374151",
        fontSize: 14
      }
    },
    series: [
      {
        name: "Sources",
        type: "pie",
        radius: ["34%", "68%"],
        center: ["40%", "56%"],
        padAngle: 1.2,
        avoidLabelOverlap: true,
        itemStyle: {
          borderColor: "#fffaf0",
          borderWidth: 4,
          borderRadius: 8
        },
        label: {
          color: "#111827",
          fontSize: 13,
          formatter: "{b}\n{d}%"
        },
        labelLine: {
          length: 18,
          length2: 12,
          lineStyle: {
            color: "#94a3b8",
            width: 2
          }
        },
        emphasis: {
          itemStyle: {
            shadowBlur: 24,
            shadowColor: "rgba(15, 23, 42, 0.18)"
          }
        },
        data: [
          { value: 1048, name: "Search", itemStyle: { color: "#2563eb" } },
          { value: 735, name: "Direct", itemStyle: { color: "#f97316" } },
          { value: 580, name: "Email", itemStyle: { color: "#14b8a6" } },
          { value: 484, name: "Union Ads", itemStyle: { color: "#8b5cf6" } },
          { value: 300, name: "Video", itemStyle: { color: "#e11d48" } }
        ]
      }
    ]
  };
}

function createScatterOption() {
  const points = [
    [12, 82, 38], [18, 91, 52], [24, 76, 43], [32, 88, 65], [40, 69, 34],
    [48, 95, 73], [55, 78, 46], [62, 84, 58], [70, 92, 80], [78, 74, 40]
  ];

  return {
    animation: false,
    backgroundColor: "#f8fafc",
    title: {
      text: "Conversion Scatter",
      left: "center",
      top: 24,
      textStyle: {
        color: "#0f172a",
        fontSize: 24,
        fontWeight: "bold"
      }
    },
    grid: {
      left: 72,
      right: 48,
      top: 96,
      bottom: 68
    },
    tooltip: {
      trigger: "item",
      formatter(params) {
        return `Spend: ${params.value[0]}k<br/>ROI: ${params.value[1]}<br/>Volume: ${params.value[2]}`;
      }
    },
    xAxis: {
      name: "Ad Spend (k)",
      nameLocation: "middle",
      nameGap: 36,
      min: 0,
      max: 90,
      splitLine: {
        lineStyle: {
          color: "#dbe4f0",
          type: "dashed"
        }
      },
      axisLabel: {
        color: "#334155"
      }
    },
    yAxis: {
      name: "ROI",
      nameLocation: "middle",
      nameGap: 44,
      min: 60,
      max: 100,
      splitLine: {
        lineStyle: {
          color: "#dbe4f0",
          type: "dashed"
        }
      },
      axisLabel: {
        color: "#334155"
      }
    },
    visualMap: {
      min: 30,
      max: 80,
      dimension: 2,
      orient: "horizontal",
      left: "center",
      bottom: 16,
      textStyle: {
        color: "#475569"
      },
      inRange: {
        color: ["#93c5fd", "#2563eb", "#0f172a"]
      }
    },
    series: [
      {
        type: "scatter",
        data: points,
        symbolSize(value) {
          return value[2] * 0.72;
        },
        itemStyle: {
          shadowBlur: 18,
          shadowColor: "rgba(37, 99, 235, 0.18)"
        }
      }
    ]
  };
}

function createHeatmapOption() {
  const hours = ["12a", "3a", "6a", "9a", "12p", "3p", "6p", "9p"];
  const days = ["Sat", "Fri", "Thu", "Wed", "Tue", "Mon", "Sun"];
  const data = [
    [0, 0, 2], [1, 0, 4], [2, 0, 8], [3, 0, 12], [4, 0, 8], [5, 0, 6], [6, 0, 4], [7, 0, 3],
    [0, 1, 3], [1, 1, 6], [2, 1, 12], [3, 1, 18], [4, 1, 11], [5, 1, 8], [6, 1, 6], [7, 1, 4],
    [0, 2, 2], [1, 2, 5], [2, 2, 9], [3, 2, 14], [4, 2, 10], [5, 2, 7], [6, 2, 4], [7, 2, 2],
    [0, 3, 1], [1, 3, 3], [2, 3, 7], [3, 3, 11], [4, 3, 15], [5, 3, 12], [6, 3, 9], [7, 3, 5],
    [0, 4, 1], [1, 4, 2], [2, 4, 6], [3, 4, 10], [4, 4, 16], [5, 4, 18], [6, 4, 13], [7, 4, 8],
    [0, 5, 2], [1, 5, 4], [2, 5, 7], [3, 5, 12], [4, 5, 19], [5, 5, 21], [6, 5, 14], [7, 5, 9],
    [0, 6, 1], [1, 6, 2], [2, 6, 5], [3, 6, 8], [4, 6, 13], [5, 6, 12], [6, 6, 8], [7, 6, 4]
  ];

  return {
    animation: false,
    backgroundColor: "#f8fafc",
    title: {
      text: "Heatmap Compatibility",
      left: "center",
      top: 24,
      textStyle: {
        color: "#0f172a",
        fontSize: 24,
        fontWeight: "bold"
      }
    },
    tooltip: {
      position: "top"
    },
    grid: {
      left: 90,
      right: 50,
      top: 96,
      bottom: 60
    },
    xAxis: {
      type: "category",
      data: hours,
      splitArea: {
        show: true
      },
      axisLabel: {
        color: "#334155"
      }
    },
    yAxis: {
      type: "category",
      data: days,
      splitArea: {
        show: true
      },
      axisLabel: {
        color: "#334155"
      }
    },
    visualMap: {
      min: 0,
      max: 24,
      calculable: true,
      orient: "horizontal",
      left: "center",
      bottom: 12,
      inRange: {
        color: ["#dbeafe", "#60a5fa", "#2563eb", "#0f172a"]
      }
    },
    series: [
      {
        type: "heatmap",
        data,
        label: {
          show: true,
          color: "#ffffff"
        },
        emphasis: {
          itemStyle: {
            shadowBlur: 10,
            shadowColor: "rgba(15, 23, 42, 0.25)"
          }
        }
      }
    ]
  };
}

function createCandlestickOption() {
  const days = ["Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"];
  const data = [
    [220, 232, 210, 240],
    [232, 218, 208, 238],
    [218, 256, 214, 268],
    [256, 246, 236, 262],
    [246, 278, 242, 286],
    [278, 294, 270, 304],
    [294, 286, 280, 308]
  ];

  return {
    animation: false,
    backgroundColor: "#f8fafc",
    title: {
      text: "Candlestick Compatibility",
      subtext: "Body + wick + category axis",
      left: "center",
      top: 18,
      textStyle: {
        color: "#0f172a",
        fontSize: 24,
        fontWeight: "bold"
      },
      subtextStyle: {
        color: "#64748b"
      }
    },
    grid: {
      left: 80,
      right: 50,
      top: 100,
      bottom: 56
    },
    xAxis: {
      type: "category",
      data: days,
      axisLabel: {
        color: "#334155"
      }
    },
    yAxis: {
      scale: true,
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
        type: "candlestick",
        data,
        itemStyle: {
          color: "#f97316",
          color0: "#2563eb",
          borderColor: "#ea580c",
          borderColor0: "#1d4ed8",
          borderWidth: 2
        }
      }
    ]
  };
}

function createFunnelOption() {
  return {
    animation: false,
    backgroundColor: "#fffaf0",
    title: {
      text: "Funnel Compatibility",
      subtext: "Polygon slices + inside labels",
      left: "center",
      top: 18,
      textStyle: {
        color: "#1f2937",
        fontSize: 24,
        fontWeight: "bold"
      },
      subtextStyle: {
        color: "#6b7280"
      }
    },
    tooltip: {
      trigger: "item"
    },
    series: [
      {
        type: "funnel",
        top: 90,
        left: "18%",
        width: "64%",
        height: "72%",
        min: 0,
        max: 100,
        sort: "descending",
        gap: 4,
        label: {
          show: true,
          position: "inside",
          color: "#ffffff",
          fontSize: 14
        },
        labelLine: {
          show: false
        },
        itemStyle: {
          borderColor: "#fffaf0",
          borderWidth: 3
        },
        data: [
          { value: 100, name: "Exposure", itemStyle: { color: "#2563eb" } },
          { value: 80, name: "Clicks", itemStyle: { color: "#14b8a6" } },
          { value: 58, name: "Visits", itemStyle: { color: "#f97316" } },
          { value: 36, name: "Leads", itemStyle: { color: "#8b5cf6" } },
          { value: 22, name: "Orders", itemStyle: { color: "#e11d48" } }
        ]
      }
    ]
  };
}

function createRadarOption() {
  return {
    animation: false,
    backgroundColor: "#f8fafc",
    title: {
      text: "Radar Compatibility",
      subtext: "Polygon grid + area fill",
      left: "center",
      top: 18,
      textStyle: {
        color: "#0f172a",
        fontSize: 24,
        fontWeight: "bold"
      },
      subtextStyle: {
        color: "#64748b"
      }
    },
    legend: {
      top: 64,
      textStyle: {
        color: "#334155"
      }
    },
    radar: {
      center: ["50%", "58%"],
      radius: "58%",
      splitNumber: 5,
      shape: "polygon",
      axisName: {
        color: "#0f172a",
        fontSize: 14
      },
      splitLine: {
        lineStyle: {
          color: ["#e2e8f0", "#cbd5e1", "#94a3b8", "#64748b", "#334155"]
        }
      },
      splitArea: {
        areaStyle: {
          color: ["rgba(226, 232, 240, 0.25)", "rgba(255, 255, 255, 0.7)"]
        }
      },
      axisLine: {
        lineStyle: {
          color: "#cbd5e1"
        }
      },
      indicator: [
        { name: "Rendering", max: 100 },
        { name: "Text", max: 100 },
        { name: "Paths", max: 100 },
        { name: "Images", max: 100 },
        { name: "Gradients", max: 100 },
        { name: "Transforms", max: 100 }
      ]
    },
    series: [
      {
        type: "radar",
        symbol: "circle",
        symbolSize: 10,
        lineStyle: {
          width: 3
        },
        data: [
          {
            name: "Current",
            value: [92, 88, 90, 84, 86, 82],
            lineStyle: {
              color: "#2563eb"
            },
            areaStyle: {
              color: "rgba(37, 99, 235, 0.22)"
            },
            itemStyle: {
              color: "#1d4ed8"
            }
          },
          {
            name: "Target",
            value: [96, 94, 95, 92, 93, 91],
            lineStyle: {
              color: "#f97316"
            },
            areaStyle: {
              color: "rgba(249, 115, 22, 0.16)"
            },
            itemStyle: {
              color: "#ea580c"
            }
          }
        ]
      }
    ]
  };
}

function createSankeyOption() {
  return {
    animation: false,
    backgroundColor: "#f8fafc",
    title: {
      text: "Sankey Compatibility",
      subtext: "Layout + flowing edges",
      left: "center",
      top: 18,
      textStyle: {
        color: "#0f172a",
        fontSize: 24,
        fontWeight: "bold"
      },
      subtextStyle: {
        color: "#64748b"
      }
    },
    tooltip: {
      trigger: "item"
    },
    series: [
      {
        type: "sankey",
        left: 40,
        right: 40,
        top: 90,
        bottom: 30,
        emphasis: {
          focus: "adjacency"
        },
        lineStyle: {
          color: "source",
          curveness: 0.5,
          opacity: 0.35
        },
        label: {
          color: "#0f172a",
          fontSize: 13
        },
        data: [
          { name: "Traffic", itemStyle: { color: "#2563eb" } },
          { name: "Search", itemStyle: { color: "#14b8a6" } },
          { name: "Social", itemStyle: { color: "#f97316" } },
          { name: "Email", itemStyle: { color: "#8b5cf6" } },
          { name: "Checkout", itemStyle: { color: "#e11d48" } },
          { name: "Subscribed", itemStyle: { color: "#0f172a" } }
        ],
        links: [
          { source: "Traffic", target: "Search", value: 42 },
          { source: "Traffic", target: "Social", value: 24 },
          { source: "Traffic", target: "Email", value: 14 },
          { source: "Search", target: "Checkout", value: 22 },
          { source: "Social", target: "Checkout", value: 16 },
          { source: "Email", target: "Subscribed", value: 12 },
          { source: "Checkout", target: "Subscribed", value: 18 }
        ]
      }
    ]
  };
}

function createTreemapOption() {
  return {
    animation: false,
    backgroundColor: "#f8fafc",
    title: {
      text: "Treemap Compatibility",
      subtext: "Nested rectangles + labels",
      left: "center",
      top: 18,
      textStyle: {
        color: "#0f172a",
        fontSize: 24,
        fontWeight: "bold"
      },
      subtextStyle: {
        color: "#64748b"
      }
    },
    series: [
      {
        type: "treemap",
        roam: false,
        breadcrumb: {
          show: false
        },
        width: "86%",
        height: "72%",
        left: "7%",
        top: 88,
        label: {
          show: true,
          formatter: "{b}",
          color: "#ffffff",
          fontSize: 14
        },
        upperLabel: {
          show: true,
          height: 28,
          color: "#0f172a",
          fontSize: 14
        },
        itemStyle: {
          borderColor: "#ffffff",
          borderWidth: 3,
          gapWidth: 3
        },
        levels: [
          {
            itemStyle: {
              borderColor: "#e2e8f0",
              borderWidth: 0,
              gapWidth: 3
            }
          },
          {
            color: ["#2563eb", "#f97316", "#14b8a6", "#8b5cf6"],
            colorSaturation: [0.35, 0.75],
            itemStyle: {
              gapWidth: 3,
              borderColorSaturation: 0.6
            }
          }
        ],
        data: [
          {
            name: "Frontend",
            value: 38,
            children: [
              { name: "Charts", value: 14 },
              { name: "Editor", value: 9 },
              { name: "Design System", value: 8 },
              { name: "Runtime", value: 7 }
            ]
          },
          {
            name: "Backend",
            value: 31,
            children: [
              { name: "Canvas API", value: 11 },
              { name: "Raster Export", value: 8 },
              { name: "Text Layout", value: 7 },
              { name: "Images", value: 5 }
            ]
          },
          {
            name: "QA",
            value: 19,
            children: [
              { name: "Smoke", value: 6 },
              { name: "Regression", value: 7 },
              { name: "Examples", value: 6 }
            ]
          },
          {
            name: "Docs",
            value: 12,
            children: [
              { name: "README", value: 5 },
              { name: "Install", value: 4 },
              { name: "API", value: 3 }
            ]
          }
        ]
      }
    ]
  };
}

function createParallelOption() {
  return {
    animation: false,
    backgroundColor: "#f8fafc",
    title: {
      text: "Parallel Compatibility",
      subtext: "Multi-axis polylines + labels",
      left: "center",
      top: 18,
      textStyle: {
        color: "#0f172a",
        fontSize: 24,
        fontWeight: "bold"
      },
      subtextStyle: {
        color: "#64748b"
      }
    },
    parallelAxis: [
      { dim: 0, name: "CPU", max: 100, nameTextStyle: { color: "#334155" } },
      { dim: 1, name: "Memory", max: 100, nameTextStyle: { color: "#334155" } },
      { dim: 2, name: "IO", max: 100, nameTextStyle: { color: "#334155" } },
      { dim: 3, name: "Network", max: 100, nameTextStyle: { color: "#334155" } },
      { dim: 4, name: "FPS", max: 100, nameTextStyle: { color: "#334155" } }
    ],
    parallel: {
      left: 88,
      right: 72,
      top: 110,
      bottom: 60,
      parallelAxisDefault: {
        axisLine: {
          lineStyle: {
            color: "#94a3b8",
            width: 2
          }
        },
        axisTick: {
          lineStyle: {
            color: "#cbd5e1"
          }
        },
        axisLabel: {
          color: "#475569"
        },
        splitLine: {
          show: false
        }
      }
    },
    series: [
      {
        type: "parallel",
        smooth: true,
        lineStyle: {
          width: 3,
          opacity: 0.55
        },
        data: [
          { value: [82, 74, 66, 58, 90], lineStyle: { color: "#2563eb" } },
          { value: [64, 52, 78, 70, 72], lineStyle: { color: "#14b8a6" } },
          { value: [48, 68, 58, 84, 64], lineStyle: { color: "#f97316" } },
          { value: [90, 80, 72, 62, 96], lineStyle: { color: "#8b5cf6" } }
        ]
      }
    ]
  };
}

function createGraphOption() {
  return {
    animation: false,
    backgroundColor: "#f8fafc",
    title: {
      text: "Graph Compatibility",
      subtext: "Curved edges, labels, symbols",
      left: "center",
      top: 18,
      textStyle: {
        color: "#0f172a",
        fontSize: 24,
        fontWeight: "bold"
      },
      subtextStyle: {
        color: "#64748b"
      }
    },
    tooltip: {},
    series: [
      {
        type: "graph",
        layout: "none",
        roam: false,
        edgeSymbol: ["circle", "arrow"],
        edgeSymbolSize: [6, 12],
        label: {
          show: true,
          color: "#0f172a",
          fontSize: 13
        },
        lineStyle: {
          color: "#94a3b8",
          width: 2.5,
          curveness: 0.22
        },
        emphasis: {
          focus: "adjacency",
          lineStyle: {
            width: 4
          }
        },
        data: [
          {
            name: "Gateway",
            x: 160,
            y: 240,
            symbolSize: 68,
            itemStyle: { color: "#2563eb" }
          },
          {
            name: "Auth",
            x: 360,
            y: 120,
            symbolSize: 54,
            itemStyle: { color: "#14b8a6" }
          },
          {
            name: "Orders",
            x: 360,
            y: 360,
            symbolSize: 58,
            itemStyle: { color: "#f97316" }
          },
          {
            name: "Inventory",
            x: 620,
            y: 150,
            symbolSize: 56,
            itemStyle: { color: "#8b5cf6" }
          },
          {
            name: "Billing",
            x: 620,
            y: 330,
            symbolSize: 56,
            itemStyle: { color: "#e11d48" }
          },
          {
            name: "Analytics",
            x: 820,
            y: 240,
            symbolSize: 60,
            itemStyle: { color: "#0f172a" }
          }
        ],
        links: [
          { source: "Gateway", target: "Auth" },
          { source: "Gateway", target: "Orders" },
          { source: "Auth", target: "Inventory" },
          { source: "Orders", target: "Inventory" },
          { source: "Orders", target: "Billing" },
          { source: "Inventory", target: "Analytics" },
          { source: "Billing", target: "Analytics" }
        ]
      }
    ]
  };
}

function createTreeOption() {
  const data = {
    name: "Canvas Engine",
    children: [
      {
        name: "Rendering",
        children: [
          { name: "Paths" },
          { name: "Text" },
          { name: "Images" }
        ]
      },
      {
        name: "Runtime",
        children: [
          { name: "V8 Bindings" },
          { name: "Task Queue" },
          { name: "Script Loader" }
        ]
      },
      {
        name: "Compatibility",
        children: [
          { name: "Bar / Line" },
          { name: "Tree / Radar" },
          { name: "Sunburst / Gauge" }
        ]
      }
    ]
  };

  return {
    animation: false,
    backgroundColor: "#f8fafc",
    title: {
      text: "Tree Compatibility",
      subtext: "Bezier edges + labels + symbols",
      left: "center",
      top: 18,
      textStyle: {
        color: "#0f172a",
        fontSize: 24,
        fontWeight: "bold"
      },
      subtextStyle: {
        color: "#64748b"
      }
    },
    series: [
      {
        type: "tree",
        data: [data],
        top: 86,
        left: 80,
        bottom: 48,
        right: 120,
        symbol: "roundRect",
        symbolSize: [110, 36],
        orient: "LR",
        expandAndCollapse: false,
        initialTreeDepth: -1,
        lineStyle: {
          color: "#94a3b8",
          width: 2,
          curveness: 0.55
        },
        label: {
          position: "inside",
          verticalAlign: "middle",
          align: "center",
          color: "#0f172a",
          fontSize: 13
        },
        itemStyle: {
          color: "#dbeafe",
          borderColor: "#60a5fa",
          borderWidth: 2
        },
        leaves: {
          label: {
            position: "inside",
            color: "#082f49"
          },
          itemStyle: {
            color: "#ccfbf1",
            borderColor: "#14b8a6"
          }
        },
        emphasis: {
          focus: "descendant"
        }
      }
    ]
  };
}

function createThemeRiverOption() {
  const data = [
    ["2026-01-01", 24, "Search"],
    ["2026-01-02", 26, "Search"],
    ["2026-01-03", 28, "Search"],
    ["2026-01-04", 27, "Search"],
    ["2026-01-05", 30, "Search"],
    ["2026-01-06", 33, "Search"],
    ["2026-01-01", 14, "Ads"],
    ["2026-01-02", 15, "Ads"],
    ["2026-01-03", 18, "Ads"],
    ["2026-01-04", 17, "Ads"],
    ["2026-01-05", 19, "Ads"],
    ["2026-01-06", 21, "Ads"],
    ["2026-01-01", 10, "Social"],
    ["2026-01-02", 12, "Social"],
    ["2026-01-03", 11, "Social"],
    ["2026-01-04", 13, "Social"],
    ["2026-01-05", 14, "Social"],
    ["2026-01-06", 16, "Social"]
  ];

  return {
    animation: false,
    backgroundColor: "#fffaf0",
    title: {
      text: "ThemeRiver Compatibility",
      subtext: "Smoothed stacked streams",
      left: "center",
      top: 18,
      textStyle: {
        color: "#1f2937",
        fontSize: 24,
        fontWeight: "bold"
      },
      subtextStyle: {
        color: "#6b7280"
      }
    },
    tooltip: {
      trigger: "axis"
    },
    singleAxis: {
      type: "time",
      top: 100,
      bottom: 50,
      axisLine: {
        lineStyle: {
          color: "#94a3b8"
        }
      },
      axisLabel: {
        color: "#334155"
      }
    },
    series: [
      {
        type: "themeRiver",
        emphasis: {
          itemStyle: {
            shadowBlur: 16,
            shadowColor: "rgba(15, 23, 42, 0.18)"
          }
        },
        label: {
          show: true,
          color: "#0f172a"
        },
        data,
        color: ["#2563eb", "#14b8a6", "#f97316"]
      }
    ]
  };
}

function createGeoHeatmapOption() {
  echarts.registerMap("compare_geo_heatmap_grid", {
    type: "FeatureCollection",
    features: [
      {
        type: "Feature",
        id: "north-west",
        properties: { name: "North West" },
        geometry: {
          type: "Polygon",
          coordinates: [[[100, 40], [110, 40], [110, 30], [100, 30], [100, 40]]]
        }
      },
      {
        type: "Feature",
        id: "north-east",
        properties: { name: "North East" },
        geometry: {
          type: "Polygon",
          coordinates: [[[110, 40], [120, 40], [120, 30], [110, 30], [110, 40]]]
        }
      },
      {
        type: "Feature",
        id: "south-west",
        properties: { name: "South West" },
        geometry: {
          type: "Polygon",
          coordinates: [[[100, 30], [110, 30], [110, 20], [100, 20], [100, 30]]]
        }
      },
      {
        type: "Feature",
        id: "south-east",
        properties: { name: "South East" },
        geometry: {
          type: "Polygon",
          coordinates: [[[110, 30], [120, 30], [120, 20], [110, 20], [110, 30]]]
        }
      }
    ]
  });

  return {
    animation: false,
    backgroundColor: "#f8fafc",
    title: {
      text: "Geo Heatmap Compatibility",
      subtext: "Exercises getImageData / putImageData path",
      left: "center",
      top: 18,
      textStyle: {
        color: "#0f172a",
        fontSize: 24,
        fontWeight: "bold"
      },
      subtextStyle: {
        color: "#64748b",
        fontSize: 12
      }
    },
    tooltip: {
      trigger: "item"
    },
    geo: {
      map: "compare_geo_heatmap_grid",
      roam: false,
      left: 120,
      right: 120,
      top: 96,
      bottom: 40,
      itemStyle: {
        areaColor: "#e2e8f0",
        borderColor: "#94a3b8",
        borderWidth: 2
      },
      emphasis: {
        itemStyle: {
          areaColor: "#cbd5e1"
        }
      }
    },
    visualMap: {
      min: 0,
      max: 100,
      calculable: true,
      orient: "horizontal",
      left: "center",
      bottom: 12,
      inRange: {
        color: ["#dbeafe", "#60a5fa", "#2563eb", "#0f172a"]
      }
    },
    series: [
      {
        type: "heatmap",
        coordinateSystem: "geo",
        pointSize: 26,
        blurSize: 20,
        data: [
          { name: "NW-1", value: [103, 36, 20] },
          { name: "NW-2", value: [107, 33, 48] },
          { name: "NE-1", value: [114, 36, 72] },
          { name: "NE-2", value: [118, 34, 94] },
          { name: "SW-1", value: [104, 27, 34] },
          { name: "SW-2", value: [108, 24, 56] },
          { name: "SE-1", value: [114, 27, 64] },
          { name: "SE-2", value: [118, 24, 88] }
        ]
      }
    ]
  };
}

function createRichGraphicOption() {
  const badge = document.createElement("canvas");
  badge.width = 128;
  badge.height = 128;

  const badgeCtx = badge.getContext("2d");
  badgeCtx.fillStyle = "#0f172a";
  badgeCtx.beginPath();
  badgeCtx.arc(64, 64, 58, 0, Math.PI * 2);
  badgeCtx.fill();
  badgeCtx.fillStyle = "#f8fafc";
  badgeCtx.beginPath();
  badgeCtx.arc(64, 50, 18, 0, Math.PI * 2);
  badgeCtx.fill();
  badgeCtx.strokeStyle = "#38bdf8";
  badgeCtx.lineWidth = 8;
  badgeCtx.beginPath();
  badgeCtx.arc(64, 82, 24, Math.PI, Math.PI * 2);
  badgeCtx.stroke();

  return {
    animation: false,
    backgroundColor: "#f8fafc",
    title: {
      text: "Rich Text + Graphic + MarkPoint",
      left: "center",
      top: 20,
      textStyle: {
        color: "#0f172a",
        fontSize: 24,
        fontWeight: "bold"
      }
    },
    grid: {
      left: 72,
      right: 56,
      top: 112,
      bottom: 72
    },
    tooltip: {
      trigger: "axis"
    },
    xAxis: {
      type: "category",
      boundaryGap: false,
      data: ["Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"],
      axisLabel: {
        color: "#334155"
      }
    },
    yAxis: {
      type: "value",
      min: 100,
      max: 380,
      splitLine: {
        lineStyle: {
          color: "#dbe4f0",
          type: "dashed"
        }
      },
      axisLabel: {
        color: "#334155"
      }
    },
    series: [
      {
        name: "Visits",
        type: "line",
        smooth: true,
        symbol: "circle",
        symbolSize: 10,
        lineStyle: {
          color: "#2563eb",
          width: 4
        },
        itemStyle: {
          color: "#1d4ed8"
        },
        areaStyle: {
          color: new echarts.graphic.LinearGradient(0, 0, 0, 1, [
            { offset: 0, color: "rgba(37, 99, 235, 0.28)" },
            { offset: 1, color: "rgba(37, 99, 235, 0.02)" }
          ])
        },
        label: {
          show: true,
          position: "top",
          formatter(params) {
            return `{value|${params.value}}\n{unit|visits}`;
          },
          rich: {
            value: {
              color: "#0f172a",
              fontSize: 14,
              fontWeight: "bold",
              padding: [0, 0, 2, 0]
            },
            unit: {
              color: "#64748b",
              fontSize: 10,
              backgroundColor: "#e2e8f0",
              padding: [3, 6, 3, 6],
              borderRadius: 8
            }
          }
        },
        markPoint: {
          symbol: "pin",
          symbolSize: 54,
          itemStyle: {
            color: "#f97316"
          },
          label: {
            color: "#ffffff",
            fontWeight: "bold",
            formatter: "{c}"
          },
          data: [
            { type: "max", name: "Peak" }
          ]
        },
        markLine: {
          symbol: ["none", "none"],
          lineStyle: {
            color: "#ef4444",
            type: "dashed",
            width: 2
          },
          label: {
            color: "#ef4444",
            formatter: "Threshold"
          },
          data: [
            { yAxis: 280 }
          ]
        },
        data: [180, 232, 201, 234, 290, 330, 310]
      }
    ],
    graphic: [
      {
        type: "group",
        right: 34,
        top: 94,
        children: [
          {
            type: "rect",
            shape: { x: 0, y: 0, width: 190, height: 104, r: 16 },
            style: {
              fill: "#ffffff",
              stroke: "#cbd5e1",
              lineWidth: 2,
              shadowBlur: 18,
              shadowColor: "rgba(15, 23, 42, 0.10)"
            }
          },
          {
            type: "image",
            style: {
              image: badge.toDataURL("image/png"),
              x: 18,
              y: 18,
              width: 54,
              height: 54
            }
          },
          {
            type: "text",
            style: {
              x: 86,
              y: 32,
              text: "Peak Week",
              fill: "#0f172a",
              font: "bold 18px sans-serif"
            }
          },
          {
            type: "text",
            style: {
              x: 86,
              y: 60,
              text: "330 visits",
              fill: "#64748b",
              font: "13px sans-serif"
            }
          }
        ]
      }
    ]
  };
}

function createPatternBarOption() {
  const tile = document.createElement("canvas");
  tile.width = 64;
  tile.height = 64;

  const tileCtx = tile.getContext("2d");
  tileCtx.fillStyle = "#eff6ff";
  tileCtx.fillRect(0, 0, 64, 64);
  tileCtx.strokeStyle = "#2563eb";
  tileCtx.lineWidth = 6;
  tileCtx.beginPath();
  tileCtx.moveTo(4, 60);
  tileCtx.lineTo(60, 4);
  tileCtx.stroke();
  tileCtx.fillStyle = "#f97316";
  tileCtx.beginPath();
  tileCtx.arc(18, 18, 8, 0, Math.PI * 2);
  tileCtx.fill();

  return {
    animation: false,
    backgroundColor: "#f8fafc",
    title: {
      text: "Pattern Fill Bar",
      left: "center",
      top: 24,
      textStyle: {
        color: "#0f172a",
        fontSize: 24,
        fontWeight: "bold"
      }
    },
    grid: {
      left: 72,
      right: 48,
      top: 96,
      bottom: 64
    },
    xAxis: {
      type: "category",
      data: ["Q1", "Q2", "Q3", "Q4"],
      axisLabel: {
        color: "#334155"
      }
    },
    yAxis: {
      type: "value",
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
        type: "bar",
        barWidth: 72,
        data: [120, 188, 156, 210],
        itemStyle: {
          color: {
            image: tile.toDataURL("image/png"),
            repeat: "repeat"
          },
          borderColor: "#1d4ed8",
          borderWidth: 2
        }
      }
    ]
  };
}

function createDataZoomMarkAreaOption() {
  const days = ["Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun", "Mon2", "Tue2", "Wed2"];
  const values = [120, 132, 101, 134, 90, 230, 210, 188, 166, 142];

  return {
    animation: false,
    backgroundColor: "#fffaf0",
    title: {
      text: "DataZoom + MarkArea Compatibility",
      subtext: "Slider + shaded region + line labels",
      left: "center",
      top: 18,
      textStyle: {
        color: "#1f2937",
        fontSize: 24,
        fontWeight: "bold"
      },
      subtextStyle: {
        color: "#6b7280"
      }
    },
    grid: {
      left: 70,
      right: 50,
      top: 100,
      bottom: 110
    },
    xAxis: {
      type: "category",
      data: days,
      axisLabel: {
        color: "#334155"
      }
    },
    yAxis: {
      type: "value",
      axisLabel: {
        color: "#334155"
      },
      splitLine: {
        lineStyle: {
          color: "#e5e7eb",
          type: "dashed"
        }
      }
    },
    dataZoom: [
      {
        type: "slider",
        bottom: 34,
        height: 34,
        start: 20,
        end: 80,
        fillerColor: "rgba(37, 99, 235, 0.18)",
        borderColor: "#cbd5e1",
        textStyle: {
          color: "#475569"
        }
      }
    ],
    series: [
      {
        type: "line",
        smooth: true,
        data: values,
        symbol: "circle",
        symbolSize: 9,
        lineStyle: {
          color: "#2563eb",
          width: 4
        },
        itemStyle: {
          color: "#1d4ed8"
        },
        areaStyle: {
          color: "rgba(37, 99, 235, 0.10)"
        },
        markArea: {
          itemStyle: {
            color: "rgba(249, 115, 22, 0.14)"
          },
          label: {
            color: "#9a3412",
            fontWeight: "bold"
          },
          data: [
            [
              { name: "Campaign", xAxis: "Thu" },
              { xAxis: "Sat" }
            ]
          ]
        },
        markLine: {
          symbol: ["none", "none"],
          lineStyle: {
            color: "#ef4444",
            type: "dashed",
            width: 2
          },
          label: {
            color: "#ef4444"
          },
          data: [{ yAxis: 160, name: "Target" }]
        }
      }
    ]
  };
}

function createLinesOption() {
  return {
    animation: false,
    backgroundColor: "#f8fafc",
    title: {
      text: "Lines Compatibility",
      subtext: "Curves + arrows + effect",
      left: "center",
      top: 18,
      textStyle: {
        color: "#0f172a",
        fontSize: 24,
        fontWeight: "bold"
      },
      subtextStyle: {
        color: "#64748b"
      }
    },
    xAxis: {
      type: "value",
      min: 0,
      max: 100,
      splitLine: {
        lineStyle: {
          color: "#dbe4f0",
          type: "dashed"
        }
      },
      axisLabel: {
        color: "#334155"
      }
    },
    yAxis: {
      type: "value",
      min: 0,
      max: 100,
      splitLine: {
        lineStyle: {
          color: "#dbe4f0",
          type: "dashed"
        }
      },
      axisLabel: {
        color: "#334155"
      }
    },
    series: [
      {
        type: "lines",
        coordinateSystem: "cartesian2d",
        polyline: false,
        effect: {
          show: false,
          constantSpeed: 28,
          symbol: "arrow",
          symbolSize: 10,
          trailLength: 0
        },
        lineStyle: {
          width: 3,
          opacity: 0.7,
          curveness: 0.28,
          color: "#2563eb"
        },
        data: [
          { coords: [[12, 20], [32, 76]] },
          { coords: [[22, 70], [48, 30]] },
          { coords: [[38, 18], [66, 82]] },
          { coords: [[54, 72], [82, 26]] },
          { coords: [[72, 40], [92, 78]] }
        ]
      }
    ]
  };
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

function createBoxplotOption() {
  return {
    animation: false,
    backgroundColor: "#f8fafc",
    title: {
      text: "Boxplot Compatibility",
      subtext: "Quartiles + whiskers + outliers",
      left: "center",
      top: 18
    },
    tooltip: {
      trigger: "item"
    },
    grid: {
      left: 72,
      right: 56,
      top: 108,
      bottom: 64
    },
    xAxis: {
      type: "category",
      data: ["North", "South", "West", "Central"],
      boundaryGap: true,
      axisLabel: {
        color: "#334155"
      }
    },
    yAxis: {
      type: "value",
      min: 0,
      max: 70,
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
        type: "boxplot",
        itemStyle: {
          color: "#bfdbfe",
          borderColor: "#2563eb",
          borderWidth: 2
        },
        emphasis: {
          itemStyle: {
            color: "#93c5fd",
            borderColor: "#1d4ed8"
          }
        },
        data: [
          [12, 18, 26, 33, 42],
          [16, 22, 29, 36, 48],
          [10, 15, 21, 27, 37],
          [20, 25, 31, 40, 52]
        ]
      },
      {
        type: "scatter",
        name: "Outlier",
        symbolSize: 10,
        itemStyle: {
          color: "#f97316"
        },
        data: [
          [0, 46],
          [1, 54],
          [2, 41],
          [3, 59]
        ]
      }
    ]
  };
}

function createPolarBarOption() {
  return {
    animation: false,
    backgroundColor: "#fffaf0",
    title: {
      text: "Polar Bar Compatibility",
      subtext: "Polar coordinate + round cap bars",
      left: "center",
      top: 18
    },
    angleAxis: {
      type: "category",
      data: ["Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"],
      axisLabel: {
        color: "#334155"
      }
    },
    radiusAxis: {
      min: 0,
      max: 40,
      axisLabel: {
        color: "#334155"
      },
      splitLine: {
        lineStyle: {
          color: "#e5e7eb",
          type: "dashed"
        }
      }
    },
    polar: {
      center: ["50%", "58%"],
      radius: "72%"
    },
    series: [
      {
        type: "bar",
        coordinateSystem: "polar",
        roundCap: true,
        itemStyle: {
          color: "#2563eb"
        },
        data: [18, 26, 22, 34, 28, 31, 24]
      }
    ]
  };
}

function createSingleAxisScatterOption() {
  const points = [
    [0, 12, 24],
    [0, 28, 36],
    [0, 46, 30],
    [0, 64, 42],
    [0, 82, 34]
  ];

  return {
    animation: false,
    backgroundColor: "#f8fafc",
    title: {
      text: "Single Axis Compatibility",
      subtext: "singleAxis + scatter strip layout",
      left: "center",
      top: 18
    },
    tooltip: {
      trigger: "item"
    },
    singleAxis: {
      left: 72,
      right: 56,
      top: "58%",
      height: 44,
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
        coordinateSystem: "singleAxis",
        data: points,
        symbolSize(value) {
          return value[2];
        },
        itemStyle: {
          color: "#14b8a6",
          shadowBlur: 16,
          shadowColor: "rgba(20, 184, 166, 0.28)"
        },
        label: {
          show: true,
          position: "top",
          formatter(param) {
            return `P${param.dataIndex + 1}`;
          },
          color: "#0f172a",
          fontWeight: "bold"
        }
      }
    ]
  };
}

function createLegendSelectedOption() {
  return {
    animation: false,
    backgroundColor: "#f8fafc",
    title: {
      text: "Legend Selected Compatibility",
      subtext: "Legend selected mode + hidden series state",
      left: "center",
      top: 18
    },
    legend: {
      top: 66,
      selected: {
        Ads: false,
        Email: false
      },
      textStyle: {
        color: "#334155"
      }
    },
    tooltip: {
      trigger: "axis"
    },
    grid: {
      left: 72,
      right: 56,
      top: 116,
      bottom: 64
    },
    xAxis: {
      type: "category",
      data: ["Jan", "Feb", "Mar", "Apr", "May", "Jun"],
      axisLabel: {
        color: "#334155"
      }
    },
    yAxis: {
      type: "value",
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
        name: "Search",
        type: "line",
        smooth: true,
        data: [92, 108, 126, 136, 152, 168],
        itemStyle: { color: "#2563eb" }
      },
      {
        name: "Direct",
        type: "line",
        smooth: true,
        data: [74, 80, 92, 101, 112, 124],
        itemStyle: { color: "#14b8a6" }
      },
      {
        name: "Ads",
        type: "line",
        smooth: true,
        data: [58, 66, 79, 82, 96, 108],
        itemStyle: { color: "#f97316" }
      },
      {
        name: "Email",
        type: "line",
        smooth: true,
        data: [32, 36, 44, 51, 56, 62],
        itemStyle: { color: "#8b5cf6" }
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

function createMarkpointMarklineOption() {
  const values = [126, 154, 141, 182, 166, 197, 188];

  return {
    animation: false,
    backgroundColor: "#f8fafc",
    title: {
      text: "MarkPoint / MarkLine Compatibility",
      subtext: "Markers + average line overlay",
      left: "center",
      top: 18
    },
    tooltip: {
      trigger: "axis"
    },
    grid: {
      left: 72,
      right: 56,
      top: 104,
      bottom: 64
    },
    xAxis: {
      type: "category",
      data: ["Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"],
      axisLabel: {
        color: "#334155"
      }
    },
    yAxis: {
      type: "value",
      min: 100,
      max: 220,
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
        type: "line",
        smooth: true,
        data: values,
        itemStyle: {
          color: "#2563eb"
        },
        lineStyle: {
          width: 4
        },
        areaStyle: {
          color: "rgba(37, 99, 235, 0.12)"
        },
        markPoint: {
          symbolSize: 46,
          itemStyle: {
            color: "#f97316"
          },
          label: {
            color: "#ffffff",
            fontWeight: "bold"
          },
          data: [
            { type: "max", name: "Peak" },
            { type: "min", name: "Low" }
          ]
        },
        markLine: {
          symbol: ["none", "none"],
          lineStyle: {
            color: "#ef4444",
            width: 2,
            type: "dashed"
          },
          label: {
            color: "#b91c1c",
            formatter(param) {
              return `Avg ${Math.round(param.value)}`;
            }
          },
          data: [{ type: "average", name: "Average" }]
        }
      }
    ]
  };
}

function createLegendScrollOption() {
  const months = ["Jan", "Feb", "Mar", "Apr", "May", "Jun"];
  const palette = [
    "#2563eb",
    "#14b8a6",
    "#f97316",
    "#8b5cf6",
    "#e11d48",
    "#0f766e",
    "#0891b2",
    "#dc2626"
  ];

  const series = palette.map((color, index) => ({
    name: `Series ${index + 1}`,
    type: "line",
    smooth: true,
    showSymbol: false,
    lineStyle: {
      width: 3
    },
    itemStyle: {
      color
    },
    data: months.map((_, monthIndex) => 48 + index * 8 + monthIndex * (index % 3 + 4))
  }));

  return {
    animation: false,
    backgroundColor: "#fffaf0",
    title: {
      text: "Legend Scroll Compatibility",
      subtext: "Scrollable legend + multiple series",
      left: "center",
      top: 18
    },
    legend: {
      type: "scroll",
      top: 66,
      left: 56,
      right: 56,
      pageIconColor: "#2563eb",
      pageTextStyle: {
        color: "#334155"
      },
      textStyle: {
        color: "#334155"
      }
    },
    tooltip: {
      trigger: "axis"
    },
    grid: {
      left: 72,
      right: 56,
      top: 126,
      bottom: 64
    },
    xAxis: {
      type: "category",
      data: months,
      axisLabel: {
        color: "#334155"
      }
    },
    yAxis: {
      type: "value",
      axisLabel: {
        color: "#334155"
      },
      splitLine: {
        lineStyle: {
          color: "#e5e7eb",
          type: "dashed"
        }
      }
    },
    series
  };
}

function createAxisPointerOption() {
  return {
    animation: false,
    backgroundColor: "#f8fafc",
    title: {
      text: "AxisPointer Compatibility",
      subtext: "Crosshair + linked tooltip state",
      left: "center",
      top: 18
    },
    tooltip: {
      trigger: "axis",
      axisPointer: {
        type: "cross"
      }
    },
    axisPointer: {
      link: [{ xAxisIndex: "all" }],
      label: {
        backgroundColor: "#0f172a"
      }
    },
    grid: {
      left: 72,
      right: 56,
      top: 108,
      bottom: 64
    },
    xAxis: {
      type: "category",
      data: ["Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"],
      axisLabel: {
        color: "#334155"
      }
    },
    yAxis: {
      type: "value",
      min: 60,
      max: 220,
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
        name: "Visits",
        type: "line",
        smooth: true,
        data: [88, 106, 128, 132, 158, 174, 192],
        itemStyle: {
          color: "#2563eb"
        }
      },
      {
        name: "Orders",
        type: "bar",
        barWidth: 22,
        data: [72, 84, 96, 103, 118, 129, 146],
        itemStyle: {
          color: "#f97316"
        }
      }
    ]
  };
}

function createVisualMapScatterOption() {
  const points = [
    [12, 18, 42],
    [20, 36, 55],
    [28, 52, 63],
    [36, 44, 71],
    [48, 68, 88],
    [56, 58, 76],
    [64, 80, 94],
    [74, 72, 82],
    [84, 90, 97]
  ];

  return {
    animation: false,
    backgroundColor: "#f8fafc",
    title: {
      text: "VisualMap Compatibility",
      subtext: "Continuous visualMap + scatter coloring",
      left: "center",
      top: 18
    },
    tooltip: {
      trigger: "item"
    },
    grid: {
      left: 72,
      right: 120,
      top: 108,
      bottom: 64
    },
    xAxis: {
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
    visualMap: {
      min: 40,
      max: 100,
      dimension: 2,
      orient: "vertical",
      right: 28,
      top: "middle",
      text: ["High", "Low"],
      calculable: true,
      textStyle: {
        color: "#334155"
      },
      inRange: {
        color: ["#93c5fd", "#2563eb", "#0f766e"]
      }
    },
    series: [
      {
        type: "scatter",
        data: points,
        symbolSize(value) {
          return 10 + (value[2] - 40) * 0.32;
        },
        itemStyle: {
          shadowBlur: 18,
          shadowColor: "rgba(15, 23, 42, 0.18)"
        },
        label: {
          show: true,
          position: "top",
          formatter(param) {
            return String(param.data[2]);
          },
          color: "#0f172a",
          fontWeight: "bold"
        }
      }
    ]
  };
}

function createVisualMapPiecewiseOption() {
  const points = [
    [18, 20, 0],
    [26, 34, 1],
    [38, 46, 1],
    [52, 28, 2],
    [61, 52, 2],
    [72, 66, 3],
    [84, 78, 3]
  ];

  return {
    animation: false,
    backgroundColor: "#fffaf0",
    title: {
      text: "Piecewise VisualMap Compatibility",
      subtext: "Piecewise categories + scatter coloring",
      left: "center",
      top: 18
    },
    tooltip: {
      trigger: "item"
    },
    grid: {
      left: 72,
      right: 168,
      top: 108,
      bottom: 64
    },
    xAxis: {
      type: "value",
      min: 0,
      max: 100,
      axisLabel: {
        color: "#334155"
      },
      splitLine: {
        lineStyle: {
          color: "#e5e7eb",
          type: "dashed"
        }
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
          color: "#e5e7eb",
          type: "dashed"
        }
      }
    },
    visualMap: {
      type: "piecewise",
      dimension: 2,
      right: 24,
      top: "middle",
      textStyle: {
        color: "#334155"
      },
      pieces: [
        { value: 0, label: "Seed", color: "#93c5fd" },
        { value: 1, label: "Warm", color: "#2563eb" },
        { value: 2, label: "Hot", color: "#f97316" },
        { value: 3, label: "Peak", color: "#dc2626" }
      ]
    },
    series: [
      {
        type: "scatter",
        data: points,
        symbolSize(value) {
          return 16 + value[2] * 5;
        },
        label: {
          show: true,
          position: "top",
          formatter(param) {
            return ["Seed", "Warm", "Warm+", "Hot", "Hot+", "Peak", "Peak+"][param.dataIndex];
          },
          color: "#0f172a",
          fontWeight: "bold"
        }
      }
    ]
  };
}

function createMapPiecewiseOption() {
  const demoGeoJson = {
    type: "FeatureCollection",
    features: [
      {
        type: "Feature",
        properties: { name: "North" },
        geometry: { type: "Polygon", coordinates: [[[0, 6], [4, 6], [4, 10], [0, 10], [0, 6]]] }
      },
      {
        type: "Feature",
        properties: { name: "West" },
        geometry: { type: "Polygon", coordinates: [[[0, 1], [4, 1], [4, 6], [0, 6], [0, 1]]] }
      },
      {
        type: "Feature",
        properties: { name: "Central" },
        geometry: { type: "Polygon", coordinates: [[[4, 1], [7, 1], [7, 6], [4, 6], [4, 1]]] }
      },
      {
        type: "Feature",
        properties: { name: "East" },
        geometry: { type: "Polygon", coordinates: [[[7, 1], [11, 1], [11, 8], [7, 8], [7, 1]]] }
      }
    ]
  };

  echarts.registerMap("compare_piecewise_regions", demoGeoJson);

  return {
    animation: false,
    backgroundColor: "#f8fafc",
    title: {
      text: "Map Piecewise VisualMap Compatibility",
      subtext: "Map + piecewise categories",
      left: "center",
      top: 18
    },
    tooltip: {
      trigger: "item"
    },
    visualMap: {
      type: "piecewise",
      left: 40,
      bottom: 28,
      textStyle: {
        color: "#334155"
      },
      pieces: [
        { min: 80, label: "Tier A", color: "#1d4ed8" },
        { min: 60, max: 79, label: "Tier B", color: "#60a5fa" },
        { min: 40, max: 59, label: "Tier C", color: "#f97316" },
        { max: 39, label: "Tier D", color: "#fecaca" }
      ]
    },
    series: [
      {
        type: "map",
        map: "compare_piecewise_regions",
        left: "center",
        top: 96,
        width: "74%",
        roam: false,
        label: {
          show: true,
          color: "#0f172a",
          fontWeight: "bold"
        },
        itemStyle: {
          borderColor: "#ffffff",
          borderWidth: 3
        },
        emphasis: {
          itemStyle: {
            areaColor: "#0f766e"
          }
        },
        data: [
          { name: "North", value: 78 },
          { name: "West", value: 34 },
          { name: "Central", value: 52 },
          { name: "East", value: 91 }
        ]
      }
    ]
  };
}

function createAxisPointerMultigridOption() {
  const days = ["Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"];

  return {
    animation: false,
    backgroundColor: "#f8fafc",
    title: {
      text: "Multi-Grid AxisPointer Compatibility",
      subtext: "Linked crosshair across stacked grids",
      left: "center",
      top: 18
    },
    tooltip: {
      trigger: "axis",
      axisPointer: {
        type: "cross"
      }
    },
    axisPointer: {
      link: [{ xAxisIndex: [0, 1] }],
      label: {
        backgroundColor: "#0f172a"
      }
    },
    grid: [
      { left: 72, right: 56, top: 108, height: 132 },
      { left: 72, right: 56, top: 300, height: 132 }
    ],
    xAxis: [
      {
        type: "category",
        gridIndex: 0,
        data: days,
        axisLabel: { color: "#334155" }
      },
      {
        type: "category",
        gridIndex: 1,
        data: days,
        axisLabel: { color: "#334155" }
      }
    ],
    yAxis: [
      {
        type: "value",
        gridIndex: 0,
        min: 60,
        max: 220,
        axisLabel: { color: "#334155" },
        splitLine: {
          lineStyle: { color: "#dbe4f0", type: "dashed" }
        }
      },
      {
        type: "value",
        gridIndex: 1,
        min: 20,
        max: 120,
        axisLabel: { color: "#334155" },
        splitLine: {
          lineStyle: { color: "#dbe4f0", type: "dashed" }
        }
      }
    ],
    series: [
      {
        name: "Visits",
        type: "line",
        xAxisIndex: 0,
        yAxisIndex: 0,
        smooth: true,
        data: [88, 106, 128, 132, 158, 174, 192],
        itemStyle: { color: "#2563eb" }
      },
      {
        name: "Orders",
        type: "bar",
        xAxisIndex: 1,
        yAxisIndex: 1,
        barWidth: 22,
        data: [34, 44, 52, 61, 73, 85, 96],
        itemStyle: { color: "#f97316" }
      }
    ]
  };
}

function createCalendarScatterOption() {
  const data = [];
  const start = new Date("2026-01-01T00:00:00Z");
  for (let i = 0; i < 140; i += 1) {
    const date = new Date(start.getTime() + i * 24 * 60 * 60 * 1000);
    const iso = date.toISOString().slice(0, 10);
    const value = 22 + (i % 9) * 6 + Math.round(Math.sin(i / 8) * 8);
    data.push([iso, value]);
  }

  return {
    animation: false,
    backgroundColor: "#fffaf0",
    title: {
      text: "Calendar Scatter Compatibility",
      subtext: "Calendar coordinate + scatter sizing",
      left: "center",
      top: 18
    },
    calendar: {
      top: 120,
      left: 60,
      right: 40,
      cellSize: ["auto", 24],
      range: "2026-01-01",
      splitLine: {
        show: true,
        lineStyle: {
          color: "#ffffff",
          width: 2
        }
      },
      itemStyle: {
        borderColor: "#ffffff",
        borderWidth: 2
      },
      dayLabel: {
        firstDay: 1,
        color: "#334155"
      },
      monthLabel: {
        color: "#0f172a"
      },
      yearLabel: {
        show: false
      }
    },
    visualMap: {
      min: 10,
      max: 80,
      orient: "horizontal",
      left: "center",
      top: 72,
      inRange: {
        color: ["#bfdbfe", "#60a5fa", "#2563eb"]
      },
      textStyle: {
        color: "#334155"
      }
    },
    series: [
      {
        type: "scatter",
        coordinateSystem: "calendar",
        data,
        symbolSize(value) {
          return Math.max(8, value[1] / 2.3);
        },
        itemStyle: {
          color: "#2563eb",
          shadowBlur: 14,
          shadowColor: "rgba(37, 99, 235, 0.24)"
        }
      }
    ]
  };
}

function createCalendarEffectScatterOption() {
  const events = [
    ["2026-01-08", 48],
    ["2026-01-18", 76],
    ["2026-02-04", 64],
    ["2026-02-16", 88],
    ["2026-03-02", 72],
    ["2026-03-14", 94]
  ];

  return {
    animation: false,
    backgroundColor: "#fffaf0",
    title: {
      text: "Calendar Effect Scatter Compatibility",
      subtext: "Calendar coordinate + effectScatter events",
      left: "center",
      top: 18
    },
    calendar: {
      top: 120,
      left: 60,
      right: 40,
      cellSize: ["auto", 24],
      range: "2026-01-01",
      splitLine: {
        show: true,
        lineStyle: {
          color: "#ffffff",
          width: 2
        }
      },
      itemStyle: {
        borderColor: "#ffffff",
        borderWidth: 2
      },
      dayLabel: {
        firstDay: 1,
        color: "#334155"
      },
      monthLabel: {
        color: "#0f172a"
      },
      yearLabel: {
        show: false
      }
    },
    series: [
      {
        type: "effectScatter",
        coordinateSystem: "calendar",
        data: events,
        symbolSize(value) {
          return value[1] / 3;
        },
        showEffectOn: "emphasis",
        rippleEffect: {
          scale: 3,
          brushType: "stroke"
        },
        itemStyle: {
          color: "#f97316",
          shadowBlur: 16,
          shadowColor: "rgba(249, 115, 22, 0.24)"
        }
      }
    ]
  };
}

function createCalendarHeatmapOption() {
  const data = [];
  const start = new Date("2026-01-01T00:00:00Z");
  for (let i = 0; i < 180; i += 1) {
    const date = new Date(start.getTime() + i * 24 * 60 * 60 * 1000);
    const iso = date.toISOString().slice(0, 10);
    const value = Math.round((Math.sin(i / 11) + 1.2) * 22 + (i % 7) * 3);
    data.push([iso, value]);
  }

  return {
    animation: false,
    backgroundColor: "#f8fafc",
    title: {
      text: "Calendar Heatmap Compatibility",
      subtext: "Calendar layout + cell labels",
      left: "center",
      top: 18,
      textStyle: {
        color: "#0f172a",
        fontSize: 24,
        fontWeight: "bold"
      },
      subtextStyle: {
        color: "#64748b"
      }
    },
    visualMap: {
      min: 0,
      max: 64,
      orient: "horizontal",
      left: "center",
      top: 72,
      inRange: {
        color: ["#dbeafe", "#60a5fa", "#2563eb", "#0f172a"]
      },
      textStyle: {
        color: "#334155"
      }
    },
    calendar: {
      top: 130,
      left: 60,
      right: 40,
      cellSize: ["auto", 22],
      range: "2026-01-01",
      splitLine: {
        show: true,
        lineStyle: {
          color: "#ffffff",
          width: 2
        }
      },
      itemStyle: {
        borderColor: "#ffffff",
        borderWidth: 2
      },
      dayLabel: {
        firstDay: 1,
        color: "#334155"
      },
      monthLabel: {
        color: "#0f172a"
      },
      yearLabel: {
        show: false
      }
    },
    series: [
      {
        type: "heatmap",
        coordinateSystem: "calendar",
        data
      }
    ]
  };
}

function createDatasetTransformOption() {
  return {
    animation: false,
    backgroundColor: "#fffaf0",
    title: {
      text: "Dataset / Transform Compatibility",
      subtext: "dataset + sort transform + encode",
      left: "center",
      top: 18
    },
    dataset: [
      {
        source: [
          ["name", "score"],
          ["North", 88],
          ["South", 72],
          ["West", 94],
          ["Central", 81],
          ["East", 97]
        ]
      },
      {
        transform: {
          type: "sort",
          config: {
            dimension: "score",
            order: "desc"
          }
        }
      }
    ],
    grid: {
      left: 72,
      right: 56,
      top: 108,
      bottom: 64
    },
    xAxis: {
      type: "value",
      min: 0,
      max: 110,
      axisLabel: {
        color: "#334155"
      },
      splitLine: {
        lineStyle: {
          color: "#e5e7eb",
          type: "dashed"
        }
      }
    },
    yAxis: {
      type: "category",
      axisLabel: {
        color: "#334155"
      }
    },
    series: [
      {
        type: "bar",
        datasetIndex: 1,
        encode: {
          x: "score",
          y: "name"
        },
        label: {
          show: true,
          position: "right",
          color: "#0f172a"
        },
        itemStyle: {
          color: "#14b8a6",
          borderRadius: [0, 8, 8, 0]
        }
      }
    ]
  };
}

function createDatasetMultiSeriesOption() {
  return {
    animation: false,
    backgroundColor: "#fffaf0",
    title: {
      text: "Dataset Multi-Series Compatibility",
      subtext: "Shared dataset + encode across bar/line",
      left: "center",
      top: 18
    },
    legend: {
      top: 66,
      textStyle: {
        color: "#334155"
      }
    },
    tooltip: {
      trigger: "axis"
    },
    dataset: {
      source: [
        ["month", "Visits", "Orders"],
        ["Jan", 82, 36],
        ["Feb", 96, 42],
        ["Mar", 90, 48],
        ["Apr", 110, 57],
        ["May", 124, 66],
        ["Jun", 136, 78]
      ]
    },
    grid: {
      left: 72,
      right: 56,
      top: 116,
      bottom: 64
    },
    xAxis: {
      type: "category",
      axisLabel: {
        color: "#334155"
      }
    },
    yAxis: {
      type: "value",
      axisLabel: {
        color: "#334155"
      },
      splitLine: {
        lineStyle: {
          color: "#e5e7eb",
          type: "dashed"
        }
      }
    },
    series: [
      {
        name: "Visits",
        type: "bar",
        encode: {
          x: "month",
          y: "Visits"
        },
        itemStyle: {
          color: "#2563eb",
          borderRadius: [8, 8, 0, 0]
        }
      },
      {
        name: "Orders",
        type: "line",
        smooth: true,
        encode: {
          x: "month",
          y: "Orders"
        },
        itemStyle: {
          color: "#f97316"
        },
        lineStyle: {
          width: 4
        }
      }
    ]
  };
}

function createToolboxMagicTypeOption() {
  return {
    animation: false,
    backgroundColor: "#fffaf0",
    title: {
      text: "Toolbox MagicType Compatibility",
      subtext: "Toolbox icon set + magicType feature",
      left: "center",
      top: 18
    },
    tooltip: {
      trigger: "axis"
    },
    legend: {
      top: 66,
      textStyle: {
        color: "#334155"
      }
    },
    toolbox: {
      right: 28,
      top: 18,
      itemSize: 20,
      iconStyle: {
        borderColor: "#475569"
      },
      feature: {
        magicType: {
          show: true,
          type: ["line", "bar", "stack"]
        },
        restore: { show: true },
        saveAsImage: { show: true }
      }
    },
    grid: {
      left: 72,
      right: 72,
      top: 116,
      bottom: 64
    },
    xAxis: {
      type: "category",
      data: ["Jan", "Feb", "Mar", "Apr", "May", "Jun"],
      axisLabel: {
        color: "#334155"
      }
    },
    yAxis: {
      type: "value",
      axisLabel: {
        color: "#334155"
      },
      splitLine: {
        lineStyle: {
          color: "#e5e7eb",
          type: "dashed"
        }
      }
    },
    series: [
      {
        name: "Visits",
        type: "bar",
        data: [82, 96, 90, 110, 124, 136],
        itemStyle: {
          color: "#2563eb",
          borderRadius: [8, 8, 0, 0]
        }
      },
      {
        name: "Orders",
        type: "bar",
        data: [36, 42, 48, 57, 66, 78],
        itemStyle: {
          color: "#f97316",
          borderRadius: [8, 8, 0, 0]
        }
      }
    ]
  };
}

function createToolboxDataZoomOption() {
  const categories = Array.from({ length: 16 }, (_, index) => `W${index + 1}`);
  const values = [42, 48, 46, 60, 66, 62, 74, 81, 78, 84, 88, 94, 102, 99, 108, 116];

  return {
    animation: false,
    backgroundColor: "#f8fafc",
    title: {
      text: "Toolbox DataZoom Compatibility",
      subtext: "Toolbox dataZoom + slider + line chart",
      left: "center",
      top: 18
    },
    tooltip: {
      trigger: "axis"
    },
    toolbox: {
      right: 28,
      top: 18,
      itemSize: 20,
      iconStyle: {
        borderColor: "#475569"
      },
      feature: {
        dataZoom: {
          show: true,
          yAxisIndex: "none"
        },
        restore: { show: true },
        saveAsImage: { show: true }
      }
    },
    grid: {
      left: 72,
      right: 56,
      top: 116,
      bottom: 110
    },
    xAxis: {
      type: "category",
      data: categories,
      axisLabel: {
        color: "#334155"
      }
    },
    yAxis: {
      type: "value",
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
    dataZoom: [
      {
        type: "inside",
        start: 20,
        end: 70
      },
      {
        type: "slider",
        start: 20,
        end: 70,
        bottom: 44
      }
    ],
    series: [
      {
        type: "line",
        smooth: true,
        data: values,
        itemStyle: {
          color: "#2563eb"
        },
        areaStyle: {
          color: "rgba(37, 99, 235, 0.12)"
        }
      }
    ]
  };
}

function createToolboxOption() {
  return {
    animation: false,
    backgroundColor: "#f8fafc",
    title: {
      text: "Toolbox Compatibility",
      subtext: "Toolbox icons + legend + smooth lines",
      left: "center",
      top: 18
    },
    legend: {
      top: 66,
      textStyle: {
        color: "#334155"
      }
    },
    toolbox: {
      right: 28,
      top: 18,
      itemSize: 20,
      iconStyle: {
        borderColor: "#475569"
      },
      feature: {
        dataView: { show: true, readOnly: true },
        restore: { show: true },
        saveAsImage: { show: true }
      }
    },
    grid: {
      left: 72,
      right: 72,
      top: 116,
      bottom: 64
    },
    xAxis: {
      type: "category",
      data: ["Jan", "Feb", "Mar", "Apr", "May", "Jun"],
      axisLabel: {
        color: "#334155"
      }
    },
    yAxis: {
      type: "value",
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
        name: "Visits",
        type: "line",
        smooth: true,
        data: [82, 96, 90, 110, 124, 136],
        itemStyle: {
          color: "#2563eb"
        }
      },
      {
        name: "Orders",
        type: "line",
        smooth: true,
        data: [56, 62, 74, 79, 88, 101],
        itemStyle: {
          color: "#f97316"
        }
      }
    ]
  };
}

function createBrushOption() {
  const points = [
    [12, 24], [18, 30], [24, 42], [30, 34], [36, 52],
    [44, 46], [52, 68], [58, 60], [66, 78], [74, 72], [82, 88]
  ];

  return {
    animation: false,
    backgroundColor: "#fffaf0",
    title: {
      text: "Brush Compatibility",
      subtext: "Brush component + scatter selection overlay",
      left: "center",
      top: 18
    },
    tooltip: {
      trigger: "item"
    },
    brush: {
      toolbox: ["rect", "polygon", "clear"],
      xAxisIndex: "all",
      yAxisIndex: "all",
      brushStyle: {
        borderWidth: 2,
        color: "rgba(37, 99, 235, 0.12)",
        borderColor: "#2563eb"
      }
    },
    grid: {
      left: 72,
      right: 56,
      top: 108,
      bottom: 64
    },
    xAxis: {
      type: "value",
      min: 0,
      max: 100,
      axisLabel: {
        color: "#334155"
      },
      splitLine: {
        lineStyle: {
          color: "#e5e7eb",
          type: "dashed"
        }
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
          color: "#e5e7eb",
          type: "dashed"
        }
      }
    },
    series: [
      {
        type: "scatter",
        data: points,
        symbolSize: 14,
        itemStyle: {
          color: "#2563eb"
        }
      }
    ]
  };
}

function createEffectScatterOption() {
  const basePoints = [
    [18, 22],
    [26, 36],
    [34, 28],
    [42, 48],
    [56, 38],
    [68, 58],
    [76, 70],
    [82, 62]
  ];

  const focusPoints = [
    { name: "Alpha", value: [26, 36, 72] },
    { name: "Beta", value: [56, 38, 88] },
    { name: "Gamma", value: [76, 70, 96] }
  ];

  return {
    animation: false,
    backgroundColor: "#fffaf0",
    title: {
      text: "Effect Scatter Compatibility",
      subtext: "Ripple scatter + labels",
      left: "center",
      top: 18
    },
    grid: {
      left: 72,
      right: 56,
      top: 104,
      bottom: 64
    },
    xAxis: {
      type: "value",
      min: 0,
      max: 100,
      axisLabel: {
        color: "#334155"
      },
      splitLine: {
        lineStyle: {
          color: "#e5e7eb",
          type: "dashed"
        }
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
          color: "#e5e7eb",
          type: "dashed"
        }
      }
    },
    series: [
      {
        type: "scatter",
        data: basePoints,
        symbolSize: 10,
        itemStyle: {
          color: "#94a3b8"
        }
      },
      {
        type: "effectScatter",
        coordinateSystem: "cartesian2d",
        data: focusPoints,
        symbolSize(value) {
          return value[2] / 6;
        },
        showEffectOn: "emphasis",
        rippleEffect: {
          scale: 3.6,
          brushType: "stroke"
        },
        itemStyle: {
          color: "#2563eb",
          shadowBlur: 20,
          shadowColor: "rgba(37, 99, 235, 0.35)"
        },
        label: {
          show: true,
          formatter(param) {
            return param.data.name;
          },
          position: "top",
          color: "#0f172a",
          fontWeight: "bold"
        }
      }
    ]
  };
}

function createMapOption() {
  const demoGeoJson = {
    type: "FeatureCollection",
    features: [
      {
        type: "Feature",
        properties: { name: "North" },
        geometry: {
          type: "Polygon",
          coordinates: [[[0, 6], [4, 6], [4, 10], [0, 10], [0, 6]]]
        }
      },
      {
        type: "Feature",
        properties: { name: "West" },
        geometry: {
          type: "Polygon",
          coordinates: [[[0, 1], [4, 1], [4, 6], [0, 6], [0, 1]]]
        }
      },
      {
        type: "Feature",
        properties: { name: "Central" },
        geometry: {
          type: "Polygon",
          coordinates: [[[4, 1], [7, 1], [7, 6], [4, 6], [4, 1]]]
        }
      },
      {
        type: "Feature",
        properties: { name: "East" },
        geometry: {
          type: "Polygon",
          coordinates: [[[7, 1], [11, 1], [11, 8], [7, 8], [7, 1]]]
        }
      }
    ]
  };

  echarts.registerMap("compare_demo_regions", demoGeoJson);

  return {
    animation: false,
    backgroundColor: "#f8fafc",
    title: {
      text: "Map Compatibility",
      subtext: "registerMap + region labels + visualMap",
      left: "center",
      top: 18
    },
    tooltip: {
      trigger: "item"
    },
    visualMap: {
      min: 18,
      max: 82,
      left: 52,
      bottom: 36,
      text: ["High", "Low"],
      inRange: {
        color: ["#bfdbfe", "#60a5fa", "#2563eb"]
      },
      textStyle: {
        color: "#334155"
      }
    },
    series: [
      {
        type: "map",
        map: "compare_demo_regions",
        left: "center",
        top: 96,
        width: "74%",
        roam: false,
        label: {
          show: true,
          color: "#0f172a",
          fontWeight: "bold"
        },
        itemStyle: {
          borderColor: "#ffffff",
          borderWidth: 3
        },
        emphasis: {
          label: {
            color: "#111827"
          },
          itemStyle: {
            areaColor: "#f97316"
          }
        },
        data: [
          { name: "North", value: 72 },
          { name: "West", value: 28 },
          { name: "Central", value: 46 },
          { name: "East", value: 82 }
        ]
      }
    ]
  };
}

function createVisualMapMapScatterOption() {
  const demoGeoJson = {
    type: "FeatureCollection",
    features: [
      {
        type: "Feature",
        properties: { name: "North" },
        geometry: { type: "Polygon", coordinates: [[[0, 6], [4, 6], [4, 10], [0, 10], [0, 6]]] }
      },
      {
        type: "Feature",
        properties: { name: "West" },
        geometry: { type: "Polygon", coordinates: [[[0, 1], [4, 1], [4, 6], [0, 6], [0, 1]]] }
      },
      {
        type: "Feature",
        properties: { name: "Central" },
        geometry: { type: "Polygon", coordinates: [[[4, 1], [7, 1], [7, 6], [4, 6], [4, 1]]] }
      },
      {
        type: "Feature",
        properties: { name: "East" },
        geometry: { type: "Polygon", coordinates: [[[7, 1], [11, 1], [11, 8], [7, 8], [7, 1]]] }
      }
    ]
  };

  echarts.registerMap("compare_visualmap_map_regions", demoGeoJson);

  return {
    animation: false,
    backgroundColor: "#fffaf0",
    title: {
      text: "VisualMap Map Scatter Compatibility",
      subtext: "Map regions + scatter overlay + continuous visualMap",
      left: "center",
      top: 18
    },
    tooltip: {
      trigger: "item"
    },
    visualMap: {
      min: 20,
      max: 100,
      orient: "horizontal",
      left: "center",
      top: 72,
      inRange: {
        color: ["#bfdbfe", "#60a5fa", "#2563eb", "#0f766e"]
      },
      textStyle: {
        color: "#334155"
      }
    },
    geo: {
      map: "compare_visualmap_map_regions",
      left: "center",
      top: 116,
      width: "70%",
      roam: false,
      label: {
        show: true,
        color: "#0f172a",
        fontWeight: "bold"
      },
      itemStyle: {
        areaColor: "#e2e8f0",
        borderColor: "#ffffff",
        borderWidth: 3
      }
    },
    series: [
      {
        type: "scatter",
        coordinateSystem: "geo",
        data: [
          { name: "North Hub", value: [2, 8, 44] },
          { name: "West Hub", value: [2, 3.5, 31] },
          { name: "Central Hub", value: [5.5, 3.5, 68] },
          { name: "East Hub", value: [9, 5.5, 92] }
        ],
        symbolSize(value) {
          return 14 + value[2] / 8;
        },
        label: {
          show: true,
          position: "right",
          formatter(param) {
            return param.data.name;
          },
          color: "#0f172a",
          fontWeight: "bold"
        },
        itemStyle: {
          shadowBlur: 16,
          shadowColor: "rgba(15, 23, 42, 0.18)"
        }
      }
    ]
  };
}

function createGeoEffectScatterOption() {
  const demoGeoJson = {
    type: "FeatureCollection",
    features: [
      {
        type: "Feature",
        properties: { name: "North" },
        geometry: {
          type: "Polygon",
          coordinates: [[[0, 6], [4, 6], [4, 10], [0, 10], [0, 6]]]
        }
      },
      {
        type: "Feature",
        properties: { name: "West" },
        geometry: {
          type: "Polygon",
          coordinates: [[[0, 1], [4, 1], [4, 6], [0, 6], [0, 1]]]
        }
      },
      {
        type: "Feature",
        properties: { name: "Central" },
        geometry: {
          type: "Polygon",
          coordinates: [[[4, 1], [7, 1], [7, 6], [4, 6], [4, 1]]]
        }
      },
      {
        type: "Feature",
        properties: { name: "East" },
        geometry: {
          type: "Polygon",
          coordinates: [[[7, 1], [11, 1], [11, 8], [7, 8], [7, 1]]]
        }
      }
    ]
  };

  echarts.registerMap("compare_geo_effect_regions", demoGeoJson);

  return {
    animation: false,
    backgroundColor: "#f8fafc",
    title: {
      text: "Geo Effect Scatter Compatibility",
      subtext: "geo component + effectScatter overlay",
      left: "center",
      top: 18
    },
    geo: {
      map: "compare_geo_effect_regions",
      left: "center",
      top: 100,
      width: "72%",
      roam: false,
      label: {
        show: true,
        color: "#0f172a",
        fontWeight: "bold"
      },
      itemStyle: {
        areaColor: "#e2e8f0",
        borderColor: "#ffffff",
        borderWidth: 3
      },
      emphasis: {
        itemStyle: {
          areaColor: "#bfdbfe"
        }
      }
    },
    series: [
      {
        type: "effectScatter",
        coordinateSystem: "geo",
        data: [
          { name: "North Hub", value: [2, 8, 82] },
          { name: "Central Hub", value: [5.5, 3.5, 66] },
          { name: "East Hub", value: [9, 5.5, 94] }
        ],
        symbolSize(value) {
          return value[2] / 5;
        },
        showEffectOn: "emphasis",
        rippleEffect: {
          scale: 3.2,
          brushType: "stroke"
        },
        itemStyle: {
          color: "#f97316",
          shadowBlur: 20,
          shadowColor: "rgba(249, 115, 22, 0.3)"
        },
        label: {
          show: true,
          position: "right",
          formatter(param) {
            return param.data.name;
          },
          color: "#0f172a",
          fontWeight: "bold"
        }
      }
    ]
  };
}

function createGeoLinesOption() {
  const demoGeoJson = {
    type: "FeatureCollection",
    features: [
      {
        type: "Feature",
        properties: { name: "North" },
        geometry: {
          type: "Polygon",
          coordinates: [[[0, 6], [4, 6], [4, 10], [0, 10], [0, 6]]]
        }
      },
      {
        type: "Feature",
        properties: { name: "West" },
        geometry: {
          type: "Polygon",
          coordinates: [[[0, 1], [4, 1], [4, 6], [0, 6], [0, 1]]]
        }
      },
      {
        type: "Feature",
        properties: { name: "Central" },
        geometry: {
          type: "Polygon",
          coordinates: [[[4, 1], [7, 1], [7, 6], [4, 6], [4, 1]]]
        }
      },
      {
        type: "Feature",
        properties: { name: "East" },
        geometry: {
          type: "Polygon",
          coordinates: [[[7, 1], [11, 1], [11, 8], [7, 8], [7, 1]]]
        }
      }
    ]
  };

  echarts.registerMap("compare_geo_lines_regions", demoGeoJson);

  return {
    animation: false,
    backgroundColor: "#f8fafc",
    title: {
      text: "Geo Lines Compatibility",
      subtext: "Geo coordinate + lines + city markers",
      left: "center",
      top: 18
    },
    geo: {
      map: "compare_geo_lines_regions",
      left: "center",
      top: 100,
      width: "72%",
      roam: false,
      label: {
        show: true,
        color: "#0f172a",
        fontWeight: "bold"
      },
      itemStyle: {
        areaColor: "#e2e8f0",
        borderColor: "#ffffff",
        borderWidth: 3
      }
    },
    series: [
      {
        type: "lines",
        coordinateSystem: "geo",
        effect: {
          show: false,
          symbol: "arrow",
          symbolSize: 10,
          trailLength: 0
        },
        lineStyle: {
          width: 3,
          color: "#2563eb",
          curveness: 0.2,
          opacity: 0.8
        },
        data: [
          { coords: [[2, 8], [5.5, 3.5]] },
          { coords: [[5.5, 3.5], [9, 5.5]] },
          { coords: [[2, 8], [9, 5.5]] }
        ]
      },
      {
        type: "effectScatter",
        coordinateSystem: "geo",
        data: [
          { name: "North Hub", value: [2, 8, 76] },
          { name: "Central Hub", value: [5.5, 3.5, 64] },
          { name: "East Hub", value: [9, 5.5, 88] }
        ],
        symbolSize(value) {
          return value[2] / 6;
        },
        showEffectOn: "emphasis",
        rippleEffect: {
          scale: 3,
          brushType: "stroke"
        },
        itemStyle: {
          color: "#f97316",
          shadowBlur: 18,
          shadowColor: "rgba(249, 115, 22, 0.28)"
        },
        label: {
          show: true,
          position: "right",
          formatter(param) {
            return param.data.name;
          },
          color: "#0f172a",
          fontWeight: "bold"
        }
      }
    ]
  };
}

function createOption(caseId) {
  switch (caseId) {
    case "bar":
      return createBarOption();
    case "line":
      return createLineOption();
    case "pie":
      return createPieOption();
    case "scatter":
      return createScatterOption();
    case "heatmap":
      return createHeatmapOption();
    case "candlestick":
      return createCandlestickOption();
    case "funnel":
      return createFunnelOption();
    case "radar":
      return createRadarOption();
    case "sankey":
      return createSankeyOption();
    case "treemap":
      return createTreemapOption();
    case "parallel":
      return createParallelOption();
    case "graph":
      return createGraphOption();
    case "tree":
      return createTreeOption();
    case "theme_river":
      return createThemeRiverOption();
    case "geo_heatmap":
      return createGeoHeatmapOption();
    case "rich_graphic":
      return createRichGraphicOption();
    case "pattern_bar":
      return createPatternBarOption();
    case "datazoom_markarea":
      return createDataZoomMarkAreaOption();
    case "lines":
      return createLinesOption();
    case "sunburst":
      return createSunburstOption();
    case "boxplot":
      return createBoxplotOption();
    case "polar_bar":
      return createPolarBarOption();
    case "single_axis_scatter":
      return createSingleAxisScatterOption();
    case "legend_selected":
      return createLegendSelectedOption();
    case "timeline":
      return createTimelineOption();
    case "markpoint_markline":
      return createMarkpointMarklineOption();
    case "legend_scroll":
      return createLegendScrollOption();
    case "axis_pointer":
      return createAxisPointerOption();
    case "visualmap_scatter":
      return createVisualMapScatterOption();
    case "visualmap_piecewise":
      return createVisualMapPiecewiseOption();
    case "map_piecewise":
      return createMapPiecewiseOption();
    case "axis_pointer_multigrid":
      return createAxisPointerMultigridOption();
    case "calendar_scatter":
      return createCalendarScatterOption();
    case "calendar_effect_scatter":
      return createCalendarEffectScatterOption();
    case "calendar_heatmap":
      return createCalendarHeatmapOption();
    case "dataset_transform":
      return createDatasetTransformOption();
    case "dataset_multi_series":
      return createDatasetMultiSeriesOption();
    case "toolbox":
      return createToolboxOption();
    case "toolbox_magic_type":
      return createToolboxMagicTypeOption();
    case "toolbox_datazoom":
      return createToolboxDataZoomOption();
    case "brush":
      return createBrushOption();
    case "custom_series":
      return createCustomSeriesOption();
    case "pictorial_bar":
      return createPictorialBarOption();
    case "gauge":
      return createGaugeOption();
    case "effect_scatter":
      return createEffectScatterOption();
    case "image_scatter":
      return createImageScatterOption();
    case "map":
      return createMapOption();
    case "visualmap_map_scatter":
      return createVisualMapMapScatterOption();
    case "geo_effect_scatter":
      return createGeoEffectScatterOption();
    case "geo_lines":
      return createGeoLinesOption();
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
  currentChart.setOption(createOption(caseInfo.id), true);
  applyCaseActions(caseInfo.id, currentChart);
  setStatus(`浏览器侧已重绘: ${caseInfo.title}`);
}

async function renderEngineCase() {
  const caseInfo = findCase(currentCaseId);
  if (!caseInfo) {
    return;
  }

  const requestId = ++engineRenderRequestId;
  setEngineLoading(true, `后端 PNG 渲染中: ${caseInfo.title}`);
  setStatus(`后端渲染中: ${caseInfo.title}`);

  try {
    const response = await fetch(`/api/render/${caseInfo.id}`, {
      method: "POST"
    });
    const payload = await response.json();

    if (!response.ok || !payload.ok) {
      throw new Error(payload.error || `render failed: ${response.status}`);
    }

    await preloadImage(payload.imageUrl);

    if (requestId !== engineRenderRequestId) {
      return;
    }

    engineImageElement.src = payload.imageUrl;
    engineImageElement.style.display = "block";
    enginePlaceholderElement.style.display = "none";
    setStatus(`后端 PNG 已更新: ${caseInfo.title}`);
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
