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

const values = [126, 154, 141, 182, 166, 197, 188];

chart.setOption({
  animation: false,
  backgroundColor: "#f8fafc",
  title: {
    text: "MarkPoint / MarkLine Compatibility",
    subtext: "Markers + average line overlay",
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
  grid: {
    left: 72,
    right: 56,
    top: 104,
    bottom: 64
  },
  xAxis: {
    type: "category",
    data: ["Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"],
    axisLabel: {
      color: "#334155"
    }
  },
  yAxis: {
    type: "value",
    min: 100,
    max: 220,
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
  series: [
    {
      type: "line",
      smooth: true,
      data: values,
      itemStyle: {
        color: "#2563eb"
      },
      lineStyle: {
        width: 4
      },
      areaStyle: {
        color: "rgba(37, 99, 235, 0.12)"
      },
      markPoint: {
        symbolSize: 46,
        itemStyle: {
          color: "#f97316"
        },
        label: {
          color: "#ffffff",
          fontWeight: "bold"
        },
        data: [
          { type: "max", name: "Peak" },
          { type: "min", name: "Low" }
        ]
      },
      markLine: {
        symbol: ["none", "none"],
        lineStyle: {
          color: "#ef4444",
          width: 2,
          type: "dashed"
        },
        label: {
          color: "#b91c1c",
          formatter(param) {
            return `Avg ${Math.round(param.value)}`;
          }
        },
        data: [{ type: "average", name: "Average" }]
      }
    }
  ]
});

chart.getZr().refreshImmediately();
canvas.saveToPng("../output/echarts_markpoint_markline.png");
console.log("wrote ../output/echarts_markpoint_markline.png");
