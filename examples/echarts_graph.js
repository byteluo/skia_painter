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
    text: "Graph Compatibility",
    subtext: "Curved edges, labels, symbols",
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
  tooltip: {},
  series: [
    {
      type: "graph",
      layout: "none",
      roam: false,
      edgeSymbol: ["circle", "arrow"],
      edgeSymbolSize: [6, 12],
      label: {
        show: true,
        color: "#0f172a",
        fontSize: 13
      },
      lineStyle: {
        color: "#94a3b8",
        width: 2.5,
        curveness: 0.22
      },
      emphasis: {
        focus: "adjacency",
        lineStyle: {
          width: 4
        }
      },
      data: [
        {
          name: "Gateway",
          x: 160,
          y: 240,
          symbolSize: 68,
          itemStyle: { color: "#2563eb" }
        },
        {
          name: "Auth",
          x: 360,
          y: 120,
          symbolSize: 54,
          itemStyle: { color: "#14b8a6" }
        },
        {
          name: "Orders",
          x: 360,
          y: 360,
          symbolSize: 58,
          itemStyle: { color: "#f97316" }
        },
        {
          name: "Inventory",
          x: 620,
          y: 150,
          symbolSize: 56,
          itemStyle: { color: "#8b5cf6" }
        },
        {
          name: "Billing",
          x: 620,
          y: 330,
          symbolSize: 56,
          itemStyle: { color: "#e11d48" }
        },
        {
          name: "Analytics",
          x: 820,
          y: 240,
          symbolSize: 60,
          itemStyle: { color: "#0f172a" }
        }
      ],
      links: [
        { source: "Gateway", target: "Auth" },
        { source: "Gateway", target: "Orders" },
        { source: "Auth", target: "Inventory" },
        { source: "Orders", target: "Inventory" },
        { source: "Orders", target: "Billing" },
        { source: "Inventory", target: "Analytics" },
        { source: "Billing", target: "Analytics" }
      ]
    }
  ]
});

chart.getZr().refreshImmediately();
canvas.saveToPng("../output/echarts_graph.png");
console.log("wrote ../output/echarts_graph.png");
