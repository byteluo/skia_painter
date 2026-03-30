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

const days = ["Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"];

chart.setOption({
  animation: false,
  backgroundColor: "#f8fafc",
  title: {
    text: "Multi-Grid AxisPointer Compatibility",
    subtext: "Linked crosshair across stacked grids",
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
});

chart.dispatchAction({
  type: "updateAxisPointer",
  currTrigger: "mousemove",
  x: 650,
  y: 220
});

chart.getZr().refreshImmediately();
canvas.saveToPng("../output/echarts_axis_pointer_multigrid.png");
console.log("wrote ../output/echarts_axis_pointer_multigrid.png");
