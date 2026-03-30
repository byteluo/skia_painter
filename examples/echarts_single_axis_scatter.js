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
  [0, 12, 24],
  [0, 28, 36],
  [0, 46, 30],
  [0, 64, 42],
  [0, 82, 34]
];

chart.setOption({
  animation: false,
  backgroundColor: "#f8fafc",
  title: {
    text: "Single Axis Compatibility",
    subtext: "singleAxis + scatter strip layout",
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
});

chart.getZr().refreshImmediately();
canvas.saveToPng("../output/echarts_single_axis_scatter.png");
console.log("wrote ../output/echarts_single_axis_scatter.png");
