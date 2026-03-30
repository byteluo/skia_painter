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
  [12, 24], [18, 30], [24, 42], [30, 34], [36, 52],
  [44, 46], [52, 68], [58, 60], [66, 78], [74, 72], [82, 88]
];

chart.setOption({
  animation: false,
  backgroundColor: "#fffaf0",
  title: {
    text: "Brush Compatibility",
    subtext: "Brush component + scatter selection overlay",
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
});

chart.dispatchAction({
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

chart.getZr().refreshImmediately();
canvas.saveToPng("../output/echarts_brush.png");
console.log("wrote ../output/echarts_brush.png");
