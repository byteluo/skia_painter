loadScript("../node_modules/echarts/dist/echarts.js");

const width = 960;
const height = 540;

function createCanvas(width, height) {
  const canvas = new Canvas(width, height);
  canvas.style = {};
  return canvas;
}

echarts.setPlatformAPI({
  createCanvas() {
    return createCanvas(width, height);
  },
  loadImage(src, onload) {
    const image = { width: 0, height: 0, src };
    if (typeof onload === "function") {
      onload();
    }
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
    text: "Skia + V8 + ECharts",
    left: "center",
    textStyle: {
      color: "#0f172a",
      fontSize: 24,
      fontWeight: "bold"
    }
  },
  grid: {
    left: 80,
    right: 40,
    top: 90,
    bottom: 70
  },
  tooltip: {},
  xAxis: {
    type: "category",
    data: ["Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"],
    axisLine: {
      lineStyle: {
        color: "#475569"
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
        color: "#475569"
      }
    },
    splitLine: {
      lineStyle: {
        color: "#cbd5e1"
      }
    },
    axisLabel: {
      color: "#334155"
    }
  },
  series: [
    {
      name: "Visits",
      type: "bar",
      barWidth: 42,
      data: [150, 230, 224, 218, 135, 147, 260],
      itemStyle: {
        color: "#2563eb"
      }
    }
  ]
});

chart.getZr().refreshImmediately();
canvas.saveToPng("output/echarts_bar.png");
console.log("wrote output/echarts_bar.png");
