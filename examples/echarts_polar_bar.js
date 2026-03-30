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
    text: "Polar Bar Compatibility",
    subtext: "Polar coordinate + round cap bars",
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
  angleAxis: {
    type: "category",
    data: ["Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"],
    axisLabel: {
      color: "#334155"
    }
  },
  radiusAxis: {
    min: 0,
    max: 40,
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
  polar: {
    center: ["50%", "58%"],
    radius: "72%"
  },
  series: [
    {
      type: "bar",
      coordinateSystem: "polar",
      roundCap: true,
      itemStyle: {
        color: "#2563eb"
      },
      data: [18, 26, 22, 34, 28, 31, 24]
    }
  ]
});

chart.getZr().refreshImmediately();
canvas.saveToPng("../output/echarts_polar_bar.png");
console.log("wrote ../output/echarts_polar_bar.png");
