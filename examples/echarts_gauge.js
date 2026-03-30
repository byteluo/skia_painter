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
    text: "Gauge Compatibility",
    subtext: "Axis arcs + pointer + detail text",
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
      type: "gauge",
      center: ["50%", "58%"],
      radius: "72%",
      startAngle: 210,
      endAngle: -30,
      min: 0,
      max: 100,
      splitNumber: 5,
      axisLine: {
        roundCap: true,
        lineStyle: {
          width: 24,
          color: [
            [0.35, "#38bdf8"],
            [0.7, "#22c55e"],
            [1, "#f97316"]
          ]
        }
      },
      progress: {
        show: true,
        roundCap: true,
        width: 24,
        itemStyle: {
          color: "#2563eb"
        }
      },
      pointer: {
        show: true,
        length: "62%",
        width: 8,
        itemStyle: {
          color: "#0f172a"
        }
      },
      anchor: {
        show: true,
        size: 18,
        itemStyle: {
          color: "#0f172a"
        }
      },
      axisTick: {
        distance: -32,
        splitNumber: 5,
        lineStyle: {
          width: 2,
          color: "#cbd5e1"
        }
      },
      splitLine: {
        distance: -34,
        length: 18,
        lineStyle: {
          width: 4,
          color: "#94a3b8"
        }
      },
      axisLabel: {
        distance: -54,
        color: "#334155",
        fontSize: 14
      },
      detail: {
        valueAnimation: false,
        offsetCenter: [0, "48%"],
        formatter(value) {
          return `{value|${value}}\n{label|Canvas score}`;
        },
        rich: {
          value: {
            fontSize: 46,
            fontWeight: "bold",
            color: "#0f172a",
            lineHeight: 54
          },
          label: {
            fontSize: 15,
            color: "#64748b",
            lineHeight: 20
          }
        }
      },
      title: {
        offsetCenter: [0, "22%"],
        color: "#334155",
        fontSize: 18
      },
      data: [
        {
          value: 82,
          name: "Stability"
        }
      ]
    }
  ]
});

chart.getZr().refreshImmediately();
canvas.saveToPng("../output/echarts_gauge.png");
console.log("wrote ../output/echarts_gauge.png");
