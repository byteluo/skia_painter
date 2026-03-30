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
  backgroundColor: "#fffaf0",
  title: {
    text: "Traffic Sources",
    subtext: "Pie compatibility",
    left: "center",
    top: 24,
    textStyle: {
      color: "#1f2937",
      fontSize: 24,
      fontWeight: "bold"
    },
    subtextStyle: {
      color: "#6b7280",
      fontSize: 12
    }
  },
  tooltip: {
    trigger: "item"
  },
  legend: {
    orient: "vertical",
    right: 32,
    top: "middle",
    textStyle: {
      color: "#374151",
      fontSize: 14
    }
  },
  series: [
    {
      name: "Sources",
      type: "pie",
      radius: ["34%", "68%"],
      center: ["40%", "56%"],
      padAngle: 1.2,
      avoidLabelOverlap: true,
      itemStyle: {
        borderColor: "#fffaf0",
        borderWidth: 4,
        borderRadius: 8
      },
      label: {
        color: "#111827",
        fontSize: 13,
        formatter: "{b}\n{d}%"
      },
      labelLine: {
        length: 18,
        length2: 12,
        lineStyle: {
          color: "#94a3b8",
          width: 2
        }
      },
      emphasis: {
        itemStyle: {
          shadowBlur: 24,
          shadowColor: "rgba(15, 23, 42, 0.18)"
        }
      },
      data: [
        { value: 1048, name: "Search", itemStyle: { color: "#2563eb" } },
        { value: 735, name: "Direct", itemStyle: { color: "#f97316" } },
        { value: 580, name: "Email", itemStyle: { color: "#14b8a6" } },
        { value: 484, name: "Union Ads", itemStyle: { color: "#8b5cf6" } },
        { value: 300, name: "Video", itemStyle: { color: "#e11d48" } }
      ]
    }
  ]
});

chart.getZr().refreshImmediately();
canvas.saveToPng("../output/echarts_pie.png");
console.log("wrote ../output/echarts_pie.png");
