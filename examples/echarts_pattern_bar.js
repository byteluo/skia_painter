loadScript("../node_modules/echarts/dist/echarts.js");

const width = 960;
const height = 540;
const patternPath = "../output/echarts_pattern_tile.png";

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

const tile = createCanvas(64, 64);
const tctx = tile.getContext("2d");
tctx.fillStyle = "#eff6ff";
tctx.fillRect(0, 0, 64, 64);
tctx.strokeStyle = "#2563eb";
tctx.lineWidth = 6;
tctx.beginPath();
tctx.moveTo(4, 60);
tctx.lineTo(60, 4);
tctx.stroke();
tctx.fillStyle = "#f97316";
tctx.beginPath();
tctx.arc(18, 18, 8, 0, Math.PI * 2);
tctx.fill();
tile.saveToPng(patternPath);

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
    text: "Pattern Fill Bar",
    left: "center",
    top: 24,
    textStyle: {
      color: "#0f172a",
      fontSize: 24,
      fontWeight: "bold"
    }
  },
  grid: {
    left: 72,
    right: 48,
    top: 96,
    bottom: 64
  },
  xAxis: {
    type: "category",
    data: ["Q1", "Q2", "Q3", "Q4"],
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
  series: [
    {
      type: "bar",
      barWidth: 72,
      data: [
        120,
        188,
        156,
        210
      ],
      itemStyle: {
        color: {
          image: patternPath,
          repeat: "repeat"
        },
        borderColor: "#1d4ed8",
        borderWidth: 2
      }
    }
  ]
});

chart.getZr().refreshImmediately();
canvas.saveToPng("../output/echarts_pattern_bar.png");
console.log("wrote ../output/echarts_pattern_tile.png");
console.log("wrote ../output/echarts_pattern_bar.png");
