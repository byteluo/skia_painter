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
    text: "Boxplot Compatibility",
    subtext: "Quartiles + whiskers + outliers",
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
});

chart.getZr().refreshImmediately();
canvas.saveToPng("../output/echarts_boxplot.png");
console.log("wrote ../output/echarts_boxplot.png");
