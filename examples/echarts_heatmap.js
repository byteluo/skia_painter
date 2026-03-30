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

const hours = ["12a", "3a", "6a", "9a", "12p", "3p", "6p", "9p"];
const days = ["Sat", "Fri", "Thu", "Wed", "Tue", "Mon", "Sun"];
const data = [
  [0, 0, 2], [1, 0, 4], [2, 0, 8], [3, 0, 12], [4, 0, 8], [5, 0, 6], [6, 0, 4], [7, 0, 3],
  [0, 1, 3], [1, 1, 6], [2, 1, 12], [3, 1, 18], [4, 1, 11], [5, 1, 8], [6, 1, 6], [7, 1, 4],
  [0, 2, 2], [1, 2, 5], [2, 2, 9], [3, 2, 14], [4, 2, 10], [5, 2, 7], [6, 2, 4], [7, 2, 2],
  [0, 3, 1], [1, 3, 3], [2, 3, 7], [3, 3, 11], [4, 3, 15], [5, 3, 12], [6, 3, 9], [7, 3, 5],
  [0, 4, 1], [1, 4, 2], [2, 4, 6], [3, 4, 10], [4, 4, 16], [5, 4, 18], [6, 4, 13], [7, 4, 8],
  [0, 5, 2], [1, 5, 4], [2, 5, 7], [3, 5, 12], [4, 5, 19], [5, 5, 21], [6, 5, 14], [7, 5, 9],
  [0, 6, 1], [1, 6, 2], [2, 6, 5], [3, 6, 8], [4, 6, 13], [5, 6, 12], [6, 6, 8], [7, 6, 4]
];

chart.setOption({
  animation: false,
  backgroundColor: "#f8fafc",
  title: {
    text: "Heatmap Compatibility",
    left: "center",
    top: 24,
    textStyle: {
      color: "#0f172a",
      fontSize: 24,
      fontWeight: "bold"
    }
  },
  tooltip: {
    position: "top"
  },
  grid: {
    left: 90,
    right: 50,
    top: 96,
    bottom: 60
  },
  xAxis: {
    type: "category",
    data: hours,
    splitArea: {
      show: true
    },
    axisLabel: {
      color: "#334155"
    }
  },
  yAxis: {
    type: "category",
    data: days,
    splitArea: {
      show: true
    },
    axisLabel: {
      color: "#334155"
    }
  },
  visualMap: {
    min: 0,
    max: 24,
    calculable: true,
    orient: "horizontal",
    left: "center",
    bottom: 12,
    inRange: {
      color: ["#dbeafe", "#60a5fa", "#2563eb", "#0f172a"]
    }
  },
  series: [
    {
      type: "heatmap",
      data,
      label: {
        show: true,
        color: "#ffffff"
      },
      emphasis: {
        itemStyle: {
          shadowBlur: 10,
          shadowColor: "rgba(15, 23, 42, 0.25)"
        }
      }
    }
  ]
});

chart.getZr().refreshImmediately();
canvas.saveToPng("../output/echarts_heatmap.png");
console.log("wrote ../output/echarts_heatmap.png");
