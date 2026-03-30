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
    text: "Lines Compatibility",
    subtext: "Curves + arrows + effect",
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
  xAxis: {
    type: "value",
    min: 0,
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
  yAxis: {
    type: "value",
    min: 0,
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
  series: [
    {
      type: "lines",
      coordinateSystem: "cartesian2d",
      polyline: false,
      effect: {
        show: true,
        constantSpeed: 28,
        symbol: "arrow",
        symbolSize: 10,
        trailLength: 0
      },
      lineStyle: {
        width: 3,
        opacity: 0.7,
        curveness: 0.28,
        color: "#2563eb"
      },
      data: [
        { coords: [[12, 20], [32, 76]] },
        { coords: [[22, 70], [48, 30]] },
        { coords: [[38, 18], [66, 82]] },
        { coords: [[54, 72], [82, 26]] },
        { coords: [[72, 40], [92, 78]] }
      ]
    }
  ]
});

chart.getZr().refreshImmediately();
canvas.saveToPng("../output/echarts_lines.png");
console.log("wrote ../output/echarts_lines.png");
