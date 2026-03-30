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

const values = [9, 14, 18, 22, 26];

chart.setOption({
  animation: false,
  backgroundColor: "#fffaf0",
  title: {
    text: "Pictorial Bar Compatibility",
    subtext: "Repeated symbols + clipping",
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
    left: 80,
    right: 60,
    top: 112,
    bottom: 56
  },
  xAxis: {
    type: "category",
    data: ["Mon", "Tue", "Wed", "Thu", "Fri"],
    axisLabel: {
      color: "#334155"
    }
  },
  yAxis: {
    type: "value",
    min: 0,
    max: 28,
    splitLine: {
      lineStyle: {
        color: "#e5e7eb",
        type: "dashed"
      }
    },
    axisLabel: {
      color: "#334155"
    }
  },
  series: [
    {
      type: "pictorialBar",
      symbol: "roundRect",
      symbolSize: [28, 12],
      symbolRepeat: true,
      symbolMargin: 3,
      symbolClip: true,
      itemStyle: {
        color: "#2563eb",
        borderRadius: 6
      },
      data: values,
      z: 10
    },
    {
      type: "pictorialBar",
      symbol: "roundRect",
      symbolSize: [28, 12],
      symbolRepeat: true,
      symbolMargin: 3,
      symbolBoundingData: 28,
      itemStyle: {
        color: "rgba(37, 99, 235, 0.12)",
        borderRadius: 6
      },
      data: new Array(values.length).fill(28),
      z: 1
    }
  ]
});

chart.getZr().refreshImmediately();
canvas.saveToPng("../output/echarts_pictorial_bar.png");
console.log("wrote ../output/echarts_pictorial_bar.png");
