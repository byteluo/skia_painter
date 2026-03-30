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
    text: "Dataset / Transform Compatibility",
    subtext: "dataset + sort transform + encode",
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
  dataset: [
    {
      source: [
        ["name", "score"],
        ["North", 88],
        ["South", 72],
        ["West", 94],
        ["Central", 81],
        ["East", 97]
      ]
    },
    {
      transform: {
        type: "sort",
        config: {
          dimension: "score",
          order: "desc"
        }
      }
    }
  ],
  grid: {
    left: 72,
    right: 56,
    top: 108,
    bottom: 64
  },
  xAxis: {
    type: "value",
    min: 0,
    max: 110,
    axisLabel: {
      color: "#334155"
    },
    splitLine: {
      lineStyle: {
        color: "#e5e7eb",
        type: "dashed"
      }
    }
  },
  yAxis: {
    type: "category",
    axisLabel: {
      color: "#334155"
    }
  },
  series: [
    {
      type: "bar",
      datasetIndex: 1,
      encode: {
        x: "score",
        y: "name"
      },
      label: {
        show: true,
        position: "right",
        color: "#0f172a"
      },
      itemStyle: {
        color: "#14b8a6",
        borderRadius: [0, 8, 8, 0]
      }
    }
  ]
});

chart.getZr().refreshImmediately();
canvas.saveToPng("../output/echarts_dataset_transform.png");
console.log("wrote ../output/echarts_dataset_transform.png");
