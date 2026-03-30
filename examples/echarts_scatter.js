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
  [12, 82, 38], [18, 91, 52], [24, 76, 43], [32, 88, 65], [40, 69, 34],
  [48, 95, 73], [55, 78, 46], [62, 84, 58], [70, 92, 80], [78, 74, 40]
];

chart.setOption({
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
});

chart.getZr().refreshImmediately();
canvas.saveToPng("../output/echarts_scatter.png");
console.log("wrote ../output/echarts_scatter.png");
