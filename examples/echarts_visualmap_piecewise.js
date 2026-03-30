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
  [18, 20, 0],
  [26, 34, 1],
  [38, 46, 1],
  [52, 28, 2],
  [61, 52, 2],
  [72, 66, 3],
  [84, 78, 3]
];

chart.setOption({
  animation: false,
  backgroundColor: "#fffaf0",
  title: {
    text: "Piecewise VisualMap Compatibility",
    subtext: "Piecewise categories + scatter coloring",
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
});

chart.getZr().refreshImmediately();
canvas.saveToPng("../output/echarts_visualmap_piecewise.png");
console.log("wrote ../output/echarts_visualmap_piecewise.png");
