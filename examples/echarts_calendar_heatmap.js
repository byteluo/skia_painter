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
for (let i = 0; i < 180; i += 1) {
  const date = new Date(start.getTime() + i * 24 * 60 * 60 * 1000);
  const iso = date.toISOString().slice(0, 10);
  const value = Math.round((Math.sin(i / 11) + 1.2) * 22 + (i % 7) * 3);
  data.push([iso, value]);
}

chart.setOption({
  animation: false,
  backgroundColor: "#f8fafc",
  title: {
    text: "Calendar Heatmap Compatibility",
    subtext: "Calendar layout + cell labels",
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
  visualMap: {
    min: 0,
    max: 64,
    orient: "horizontal",
    left: "center",
    top: 72,
    inRange: {
      color: ["#dbeafe", "#60a5fa", "#2563eb", "#0f172a"]
    },
    textStyle: {
      color: "#334155"
    }
  },
  calendar: {
    top: 130,
    left: 60,
    right: 40,
    cellSize: ["auto", 22],
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
  series: [
    {
      type: "heatmap",
      coordinateSystem: "calendar",
      data
    }
  ]
});

chart.getZr().refreshImmediately();
canvas.saveToPng("../output/echarts_calendar_heatmap.png");
console.log("wrote ../output/echarts_calendar_heatmap.png");
