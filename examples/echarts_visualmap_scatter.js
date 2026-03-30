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

chart.setOption({
  animation: false,
  backgroundColor: "#f8fafc",
  title: {
    text: "VisualMap Compatibility",
    subtext: "Continuous visualMap + scatter coloring",
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
});

chart.getZr().refreshImmediately();
canvas.saveToPng("../output/echarts_visualmap_scatter.png");
console.log("wrote ../output/echarts_visualmap_scatter.png");
