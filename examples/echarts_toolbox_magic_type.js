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
    text: "Toolbox MagicType Compatibility",
    subtext: "Toolbox icon set + magicType feature",
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
  legend: {
    top: 66,
    textStyle: {
      color: "#334155"
    }
  },
  toolbox: {
    right: 28,
    top: 18,
    itemSize: 20,
    iconStyle: {
      borderColor: "#475569"
    },
    feature: {
      magicType: {
        show: true,
        type: ["line", "bar", "stack"]
      },
      restore: { show: true },
      saveAsImage: { show: true }
    }
  },
  grid: {
    left: 72,
    right: 72,
    top: 116,
    bottom: 64
  },
  xAxis: {
    type: "category",
    data: ["Jan", "Feb", "Mar", "Apr", "May", "Jun"],
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
      data: [82, 96, 90, 110, 124, 136],
      itemStyle: {
        color: "#2563eb",
        borderRadius: [8, 8, 0, 0]
      }
    },
    {
      name: "Orders",
      type: "bar",
      data: [36, 42, 48, 57, 66, 78],
      itemStyle: {
        color: "#f97316",
        borderRadius: [8, 8, 0, 0]
      }
    }
  ]
});

chart.getZr().refreshImmediately();
canvas.saveToPng("../output/echarts_toolbox_magic_type.png");
console.log("wrote ../output/echarts_toolbox_magic_type.png");
