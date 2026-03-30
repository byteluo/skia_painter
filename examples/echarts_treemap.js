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
    text: "Treemap Compatibility",
    subtext: "Nested rectangles + labels",
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
      type: "treemap",
      roam: false,
      breadcrumb: {
        show: false
      },
      width: "86%",
      height: "72%",
      left: "7%",
      top: 88,
      label: {
        show: true,
        formatter: "{b}",
        color: "#ffffff",
        fontSize: 14
      },
      upperLabel: {
        show: true,
        height: 28,
        color: "#0f172a",
        fontSize: 14
      },
      itemStyle: {
        borderColor: "#ffffff",
        borderWidth: 3,
        gapWidth: 3
      },
      levels: [
        {
          itemStyle: {
            borderColor: "#e2e8f0",
            borderWidth: 0,
            gapWidth: 3
          }
        },
        {
          color: ["#2563eb", "#f97316", "#14b8a6", "#8b5cf6"],
          colorSaturation: [0.35, 0.75],
          itemStyle: {
            gapWidth: 3,
            borderColorSaturation: 0.6
          }
        }
      ],
      data: [
        {
          name: "Frontend",
          value: 38,
          children: [
            { name: "Charts", value: 14 },
            { name: "Editor", value: 9 },
            { name: "Design System", value: 8 },
            { name: "Runtime", value: 7 }
          ]
        },
        {
          name: "Backend",
          value: 31,
          children: [
            { name: "Canvas API", value: 11 },
            { name: "Raster Export", value: 8 },
            { name: "Text Layout", value: 7 },
            { name: "Images", value: 5 }
          ]
        },
        {
          name: "QA",
          value: 19,
          children: [
            { name: "Smoke", value: 6 },
            { name: "Regression", value: 7 },
            { name: "Examples", value: 6 }
          ]
        },
        {
          name: "Docs",
          value: 12,
          children: [
            { name: "README", value: 5 },
            { name: "Install", value: 4 },
            { name: "API", value: 3 }
          ]
        }
      ]
    }
  ]
});

chart.getZr().refreshImmediately();
canvas.saveToPng("../output/echarts_treemap.png");
console.log("wrote ../output/echarts_treemap.png");
