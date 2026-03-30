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

const data = {
  name: "Canvas Engine",
  children: [
    {
      name: "Rendering",
      children: [
        { name: "Paths" },
        { name: "Text" },
        { name: "Images" }
      ]
    },
    {
      name: "Runtime",
      children: [
        { name: "V8 Bindings" },
        { name: "Task Queue" },
        { name: "Script Loader" }
      ]
    },
    {
      name: "Compatibility",
      children: [
        { name: "Bar / Line" },
        { name: "Tree / Radar" },
        { name: "Sunburst / Gauge" }
      ]
    }
  ]
};

chart.setOption({
  animation: false,
  backgroundColor: "#f8fafc",
  title: {
    text: "Tree Compatibility",
    subtext: "Bezier edges + labels + symbols",
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
  series: [
    {
      type: "tree",
      data: [data],
      top: 86,
      left: 80,
      bottom: 48,
      right: 120,
      symbol: "roundRect",
      symbolSize: [110, 36],
      orient: "LR",
      expandAndCollapse: false,
      initialTreeDepth: -1,
      lineStyle: {
        color: "#94a3b8",
        width: 2,
        curveness: 0.55
      },
      label: {
        position: "inside",
        verticalAlign: "middle",
        align: "center",
        color: "#0f172a",
        fontSize: 13
      },
      itemStyle: {
        color: "#dbeafe",
        borderColor: "#60a5fa",
        borderWidth: 2
      },
      leaves: {
        label: {
          position: "inside",
          color: "#082f49"
        },
        itemStyle: {
          color: "#ccfbf1",
          borderColor: "#14b8a6"
        }
      },
      emphasis: {
        focus: "descendant"
      }
    }
  ]
});

chart.getZr().refreshImmediately();
canvas.saveToPng("../output/echarts_tree.png");
console.log("wrote ../output/echarts_tree.png");
