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
    text: "Funnel Compatibility",
    subtext: "Polygon slices + inside labels",
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
    trigger: "item"
  },
  series: [
    {
      type: "funnel",
      top: 90,
      left: "18%",
      width: "64%",
      height: "72%",
      min: 0,
      max: 100,
      sort: "descending",
      gap: 4,
      label: {
        show: true,
        position: "inside",
        color: "#ffffff",
        fontSize: 14
      },
      labelLine: {
        show: false
      },
      itemStyle: {
        borderColor: "#fffaf0",
        borderWidth: 3
      },
      data: [
        { value: 100, name: "Exposure", itemStyle: { color: "#2563eb" } },
        { value: 80, name: "Clicks", itemStyle: { color: "#14b8a6" } },
        { value: 58, name: "Visits", itemStyle: { color: "#f97316" } },
        { value: 36, name: "Leads", itemStyle: { color: "#8b5cf6" } },
        { value: 22, name: "Orders", itemStyle: { color: "#e11d48" } }
      ]
    }
  ]
});

chart.getZr().refreshImmediately();
canvas.saveToPng("../output/echarts_funnel.png");
console.log("wrote ../output/echarts_funnel.png");
