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
const data = [
  [220, 232, 210, 240],
  [232, 218, 208, 238],
  [218, 256, 214, 268],
  [256, 246, 236, 262],
  [246, 278, 242, 286],
  [278, 294, 270, 304],
  [294, 286, 280, 308]
];

chart.setOption({
  animation: false,
  backgroundColor: "#f8fafc",
  title: {
    text: "Candlestick Compatibility",
    subtext: "Body + wick + category axis",
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
  grid: {
    left: 80,
    right: 50,
    top: 100,
    bottom: 56
  },
  xAxis: {
    type: "category",
    data: days,
    axisLabel: {
      color: "#334155"
    }
  },
  yAxis: {
    scale: true,
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
      type: "candlestick",
      data,
      itemStyle: {
        color: "#f97316",
        color0: "#2563eb",
        borderColor: "#ea580c",
        borderColor0: "#1d4ed8",
        borderWidth: 2
      }
    }
  ]
});

chart.getZr().refreshImmediately();
canvas.saveToPng("../output/echarts_candlestick.png");
console.log("wrote ../output/echarts_candlestick.png");
