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

const months = ["Jan", "Feb", "Mar", "Apr", "May", "Jun"];
const palette = [
  "#2563eb",
  "#14b8a6",
  "#f97316",
  "#8b5cf6",
  "#e11d48",
  "#0f766e",
  "#0891b2",
  "#dc2626"
];

const series = palette.map((color, index) => ({
  name: `Series ${index + 1}`,
  type: "line",
  smooth: true,
  showSymbol: false,
  lineStyle: {
    width: 3
  },
  itemStyle: {
    color
  },
  data: months.map((_, monthIndex) => 48 + index * 8 + monthIndex * (index % 3 + 4))
}));

chart.setOption({
  animation: false,
  backgroundColor: "#fffaf0",
  title: {
    text: "Legend Scroll Compatibility",
    subtext: "Scrollable legend + multiple series",
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
  legend: {
    type: "scroll",
    top: 66,
    left: 56,
    right: 56,
    pageIconColor: "#2563eb",
    pageTextStyle: {
      color: "#334155"
    },
    textStyle: {
      color: "#334155"
    }
  },
  tooltip: {
    trigger: "axis"
  },
  grid: {
    left: 72,
    right: 56,
    top: 126,
    bottom: 64
  },
  xAxis: {
    type: "category",
    data: months,
    axisLabel: {
      color: "#334155"
    }
  },
  yAxis: {
    type: "value",
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
  series
});

chart.getZr().refreshImmediately();
canvas.saveToPng("../output/echarts_legend_scroll.png");
console.log("wrote ../output/echarts_legend_scroll.png");
