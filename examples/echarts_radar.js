loadScript("../node_modules/echarts/dist/echarts.js");

const width = 960;
const height = 540;

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
});

chart.getZr().refreshImmediately();
canvas.saveToPng("../output/echarts_radar.png");
console.log("wrote ../output/echarts_radar.png");
