loadScript("../node_modules/echarts/dist/echarts.js");

const width = 960;
const height = 540;
const badgePath = "../output/echarts_graphic_badge.png";

function createCanvas(targetWidth, targetHeight) {
  const canvas = new Canvas(targetWidth, targetHeight);
  canvas.style = {};
  return canvas;
}

echarts.setPlatformAPI({
  createCanvas() {
    return createCanvas(width, height);
  },
  loadImage(src, onload) {
    const image = new Image();
    if (typeof onload === "function") {
      image.onload = onload;
    }
    image.src = src;
    return image;
  }
});

const badgeCanvas = createCanvas(128, 128);
const badgeCtx = badgeCanvas.getContext("2d");
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
badgeCanvas.saveToPng(badgePath);

const canvas = createCanvas(width, height);
const chart = echarts.init(canvas, null, {
  renderer: "canvas",
  width,
  height
});

chart.setOption({
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
            image: badgePath,
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
});

chart.getZr().refreshImmediately();
canvas.saveToPng("../output/echarts_rich_graphic.png");
console.log("wrote ../output/echarts_graphic_badge.png");
console.log("wrote ../output/echarts_rich_graphic.png");
