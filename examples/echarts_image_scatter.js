loadScript("../node_modules/echarts/dist/echarts.js");

const width = 960;
const height = 540;
const markerPath = "../output/echarts_marker.png";

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

const markerCanvas = createCanvas(96, 96);
const markerCtx = markerCanvas.getContext("2d");
markerCtx.fillStyle = "#f8fafc";
markerCtx.beginPath();
markerCtx.arc(48, 48, 42, 0, Math.PI * 2);
markerCtx.fill();
markerCtx.strokeStyle = "#0f172a";
markerCtx.lineWidth = 6;
markerCtx.stroke();
markerCtx.fillStyle = "#2563eb";
markerCtx.beginPath();
markerCtx.arc(48, 40, 16, 0, Math.PI * 2);
markerCtx.fill();
markerCtx.fillStyle = "#f97316";
markerCtx.beginPath();
markerCtx.arc(48, 72, 18, Math.PI, Math.PI * 2);
markerCtx.fill();
markerCanvas.saveToPng(markerPath);

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
    text: "Image Marker Scatter",
    subtext: "drawImage compatibility",
    left: "center",
    top: 24,
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
    left: 72,
    right: 48,
    top: 96,
    bottom: 64
  },
  tooltip: {
    trigger: "item"
  },
  xAxis: {
    type: "category",
    data: ["A", "B", "C", "D", "E"],
    axisLabel: {
      color: "#334155"
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
  series: [
    {
      type: "scatter",
      symbol: `image://${markerPath}`,
      symbolSize: 44,
      data: [
        ["A", 24],
        ["B", 52],
        ["C", 38],
        ["D", 76],
        ["E", 61]
      ]
    }
  ]
});

chart.getZr().refreshImmediately();
canvas.saveToPng("../output/echarts_image_scatter.png");
console.log("wrote ../output/echarts_marker.png");
console.log("wrote ../output/echarts_image_scatter.png");
