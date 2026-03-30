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
    text: "Weekly Traffic Trend",
    left: "center",
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
    bottom: 64
  },
  xAxis: {
    type: "category",
    boundaryGap: false,
    data: ["Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"],
    axisLine: {
      lineStyle: {
        color: "#64748b",
        type: "dashed"
      }
    },
    splitLine: {
      show: true,
      lineStyle: {
        color: "#cbd5e1",
        type: "dashed"
      }
    },
    axisLabel: {
      color: "#334155"
    }
  },
  yAxis: {
    type: "value",
    axisLine: {
      show: true,
      lineStyle: {
        color: "#64748b"
      }
    },
    splitLine: {
      lineStyle: {
        color: "#cbd5e1",
        type: [6, 4]
      }
    },
    axisLabel: {
      color: "#334155"
    }
  },
  series: [
    {
      name: "Visits",
      type: "line",
      smooth: true,
      symbol: "circle",
      symbolSize: 10,
      lineStyle: {
        color: "#2563eb",
        width: 4,
        type: "dashed"
      },
      itemStyle: {
        color: "#1d4ed8"
      },
      areaStyle: {
        color: new echarts.graphic.LinearGradient(0, 0, 0, 1, [
          { offset: 0, color: "rgba(37, 99, 235, 0.35)" },
          { offset: 1, color: "rgba(37, 99, 235, 0.04)" }
        ])
      },
      data: [180, 232, 201, 234, 290, 330, 310]
    }
  ]
});

chart.getZr().refreshImmediately();
canvas.saveToPng("output/echarts_line.png");
console.log("wrote output/echarts_line.png");
