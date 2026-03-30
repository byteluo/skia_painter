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

const events = [
  ["2026-01-08", 48],
  ["2026-01-18", 76],
  ["2026-02-04", 64],
  ["2026-02-16", 88],
  ["2026-03-02", 72],
  ["2026-03-14", 94]
];

chart.setOption({
  animation: false,
  backgroundColor: "#fffaf0",
  title: {
    text: "Calendar Effect Scatter Compatibility",
    subtext: "Calendar coordinate + effectScatter events",
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
  series: [
    {
      type: "effectScatter",
      coordinateSystem: "calendar",
      data: events,
      symbolSize(value) {
        return value[1] / 3;
      },
      showEffectOn: "emphasis",
      rippleEffect: {
        scale: 3,
        brushType: "stroke"
      },
      itemStyle: {
        color: "#f97316",
        shadowBlur: 16,
        shadowColor: "rgba(249, 115, 22, 0.24)"
      }
    }
  ]
});

chart.getZr().refreshImmediately();
canvas.saveToPng("../output/echarts_calendar_effect_scatter.png");
console.log("wrote ../output/echarts_calendar_effect_scatter.png");
