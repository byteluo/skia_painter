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

const data = [
  ["Alpha", 12, 26, "#2563eb"],
  ["Beta", 28, 18, "#14b8a6"],
  ["Gamma", 44, 34, "#f97316"],
  ["Delta", 60, 22, "#8b5cf6"],
  ["Epsilon", 76, 30, "#e11d48"]
];

chart.setOption({
  animation: false,
  backgroundColor: "#f8fafc",
  title: {
    text: "Custom Series Compatibility",
    subtext: "renderItem group + shapes + text",
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
  grid: {
    left: 80,
    right: 56,
    top: 96,
    bottom: 60
  },
  xAxis: {
    min: 0,
    max: 90,
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
  yAxis: {
    min: 0,
    max: 40,
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
      type: "custom",
      renderItem(params, api) {
        const category = api.value(0);
        const x = api.value(1);
        const y = api.value(2);
        const color = api.value(3);
        const point = api.coord([x, y]);

        return {
          type: "group",
          children: [
            {
              type: "circle",
              shape: {
                cx: point[0],
                cy: point[1],
                r: 24
              },
              style: {
                fill: color,
                stroke: "#ffffff",
                lineWidth: 3,
                shadowBlur: 16,
                shadowColor: "rgba(15, 23, 42, 0.14)"
              }
            },
            {
              type: "text",
              style: {
                x: point[0],
                y: point[1] + 2,
                text: category,
                textAlign: "center",
                textVerticalAlign: "middle",
                fill: "#ffffff",
                font: "bold 13px sans-serif"
              }
            }
          ]
        };
      },
      data
    }
  ]
});

chart.getZr().refreshImmediately();
canvas.saveToPng("../output/echarts_custom_series.png");
console.log("wrote ../output/echarts_custom_series.png");
