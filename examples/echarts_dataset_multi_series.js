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
    text: "Dataset Multi-Series Compatibility",
    subtext: "Shared dataset + encode across bar/line",
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
  legend: {
    top: 66,
    textStyle: {
      color: "#334155"
    }
  },
  tooltip: {
    trigger: "axis"
  },
  dataset: {
    source: [
      ["month", "Visits", "Orders"],
      ["Jan", 82, 36],
      ["Feb", 96, 42],
      ["Mar", 90, 48],
      ["Apr", 110, 57],
      ["May", 124, 66],
      ["Jun", 136, 78]
    ]
  },
  grid: {
    left: 72,
    right: 56,
    top: 116,
    bottom: 64
  },
  xAxis: {
    type: "category",
    axisLabel: {
      color: "#334155"
    }
  },
  yAxis: {
    type: "value",
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
  series: [
    {
      name: "Visits",
      type: "bar",
      encode: {
        x: "month",
        y: "Visits"
      },
      itemStyle: {
        color: "#2563eb",
        borderRadius: [8, 8, 0, 0]
      }
    },
    {
      name: "Orders",
      type: "line",
      smooth: true,
      encode: {
        x: "month",
        y: "Orders"
      },
      itemStyle: {
        color: "#f97316"
      },
      lineStyle: {
        width: 4
      }
    }
  ]
});

chart.getZr().refreshImmediately();
canvas.saveToPng("../output/echarts_dataset_multi_series.png");
console.log("wrote ../output/echarts_dataset_multi_series.png");
