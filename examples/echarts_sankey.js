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
    text: "Sankey Compatibility",
    subtext: "Layout + flowing edges",
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
  tooltip: {
    trigger: "item"
  },
  series: [
    {
      type: "sankey",
      left: 40,
      right: 40,
      top: 90,
      bottom: 30,
      emphasis: {
        focus: "adjacency"
      },
      lineStyle: {
        color: "source",
        curveness: 0.5,
        opacity: 0.35
      },
      label: {
        color: "#0f172a",
        fontSize: 13
      },
      data: [
        { name: "Traffic", itemStyle: { color: "#2563eb" } },
        { name: "Search", itemStyle: { color: "#14b8a6" } },
        { name: "Social", itemStyle: { color: "#f97316" } },
        { name: "Email", itemStyle: { color: "#8b5cf6" } },
        { name: "Checkout", itemStyle: { color: "#e11d48" } },
        { name: "Subscribed", itemStyle: { color: "#0f172a" } }
      ],
      links: [
        { source: "Traffic", target: "Search", value: 42 },
        { source: "Traffic", target: "Social", value: 24 },
        { source: "Traffic", target: "Email", value: 14 },
        { source: "Search", target: "Checkout", value: 22 },
        { source: "Social", target: "Checkout", value: 16 },
        { source: "Email", target: "Subscribed", value: 12 },
        { source: "Checkout", target: "Subscribed", value: 18 }
      ]
    }
  ]
});

chart.getZr().refreshImmediately();
canvas.saveToPng("../output/echarts_sankey.png");
console.log("wrote ../output/echarts_sankey.png");
