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

chart.setOption({
  animation: false,
  backgroundColor: "#fffaf0",
  title: {
    text: "Effect Scatter Compatibility",
    subtext: "Ripple scatter + labels",
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
});

chart.getZr().refreshImmediately();
canvas.saveToPng("../output/echarts_effect_scatter.png");
console.log("wrote ../output/echarts_effect_scatter.png");
