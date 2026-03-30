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

const categories = Array.from({ length: 16 }, (_, index) => `W${index + 1}`);
const values = [42, 48, 46, 60, 66, 62, 74, 81, 78, 84, 88, 94, 102, 99, 108, 116];

chart.setOption({
  animation: false,
  backgroundColor: "#f8fafc",
  title: {
    text: "Toolbox DataZoom Compatibility",
    subtext: "Toolbox dataZoom + slider + line chart",
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
    trigger: "axis"
  },
  toolbox: {
    right: 28,
    top: 18,
    itemSize: 20,
    iconStyle: {
      borderColor: "#475569"
    },
    feature: {
      dataZoom: {
        show: true,
        yAxisIndex: "none"
      },
      restore: { show: true },
      saveAsImage: { show: true }
    }
  },
  grid: {
    left: 72,
    right: 56,
    top: 116,
    bottom: 110
  },
  xAxis: {
    type: "category",
    data: categories,
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
        color: "#dbe4f0",
        type: "dashed"
      }
    }
  },
  dataZoom: [
    {
      type: "inside",
      start: 20,
      end: 70
    },
    {
      type: "slider",
      start: 20,
      end: 70,
      bottom: 44
    }
  ],
  series: [
    {
      type: "line",
      smooth: true,
      data: values,
      itemStyle: {
        color: "#2563eb"
      },
      areaStyle: {
        color: "rgba(37, 99, 235, 0.12)"
      }
    }
  ]
});

chart.getZr().refreshImmediately();
canvas.saveToPng("../output/echarts_toolbox_datazoom.png");
console.log("wrote ../output/echarts_toolbox_datazoom.png");
