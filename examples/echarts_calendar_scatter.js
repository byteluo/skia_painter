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

const data = [];
const start = new Date("2026-01-01T00:00:00Z");
for (let i = 0; i < 140; i += 1) {
  const date = new Date(start.getTime() + i * 24 * 60 * 60 * 1000);
  const iso = date.toISOString().slice(0, 10);
  const value = 22 + (i % 9) * 6 + Math.round(Math.sin(i / 8) * 8);
  data.push([iso, value]);
}

chart.setOption({
  animation: false,
  backgroundColor: "#fffaf0",
  title: {
    text: "Calendar Scatter Compatibility",
    subtext: "Calendar coordinate + scatter sizing",
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
  calendar: {
    top: 120,
    left: 60,
    right: 40,
    cellSize: ["auto", 24],
    range: "2026-01-01",
    splitLine: {
      show: true,
      lineStyle: {
        color: "#ffffff",
        width: 2
      }
    },
    itemStyle: {
      borderColor: "#ffffff",
      borderWidth: 2
    },
    dayLabel: {
      firstDay: 1,
      color: "#334155"
    },
    monthLabel: {
      color: "#0f172a"
    },
    yearLabel: {
      show: false
    }
  },
  visualMap: {
    min: 10,
    max: 80,
    orient: "horizontal",
    left: "center",
    top: 72,
    inRange: {
      color: ["#bfdbfe", "#60a5fa", "#2563eb"]
    },
    textStyle: {
      color: "#334155"
    }
  },
  series: [
    {
      type: "scatter",
      coordinateSystem: "calendar",
      data,
      symbolSize(value) {
        return Math.max(8, value[1] / 2.3);
      },
      itemStyle: {
        color: "#2563eb",
        shadowBlur: 14,
        shadowColor: "rgba(37, 99, 235, 0.24)"
      }
    }
  ]
});

chart.getZr().refreshImmediately();
canvas.saveToPng("../output/echarts_calendar_scatter.png");
console.log("wrote ../output/echarts_calendar_scatter.png");
