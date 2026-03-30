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

const data = [
  ["2026-01-01", 24, "Search"],
  ["2026-01-02", 26, "Search"],
  ["2026-01-03", 28, "Search"],
  ["2026-01-04", 27, "Search"],
  ["2026-01-05", 30, "Search"],
  ["2026-01-06", 33, "Search"],
  ["2026-01-01", 14, "Ads"],
  ["2026-01-02", 15, "Ads"],
  ["2026-01-03", 18, "Ads"],
  ["2026-01-04", 17, "Ads"],
  ["2026-01-05", 19, "Ads"],
  ["2026-01-06", 21, "Ads"],
  ["2026-01-01", 10, "Social"],
  ["2026-01-02", 12, "Social"],
  ["2026-01-03", 11, "Social"],
  ["2026-01-04", 13, "Social"],
  ["2026-01-05", 14, "Social"],
  ["2026-01-06", 16, "Social"]
];

chart.setOption({
  animation: false,
  backgroundColor: "#fffaf0",
  title: {
    text: "ThemeRiver Compatibility",
    subtext: "Smoothed stacked streams",
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
  tooltip: {
    trigger: "axis"
  },
  singleAxis: {
    type: "time",
    top: 100,
    bottom: 50,
    axisLine: {
      lineStyle: {
        color: "#94a3b8"
      }
    },
    axisLabel: {
      color: "#334155"
    }
  },
  series: [
    {
      type: "themeRiver",
      emphasis: {
        itemStyle: {
          shadowBlur: 16,
          shadowColor: "rgba(15, 23, 42, 0.18)"
        }
      },
      label: {
        show: true,
        color: "#0f172a"
      },
      data,
      color: ["#2563eb", "#14b8a6", "#f97316"]
    }
  ]
});

chart.getZr().refreshImmediately();
canvas.saveToPng("../output/echarts_theme_river.png");
console.log("wrote ../output/echarts_theme_river.png");
