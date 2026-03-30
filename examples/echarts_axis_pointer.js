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
    text: "AxisPointer Compatibility",
    subtext: "Crosshair + linked tooltip state",
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
    trigger: "axis",
    axisPointer: {
      type: "cross"
    }
  },
  axisPointer: {
    link: [{ xAxisIndex: "all" }],
    label: {
      backgroundColor: "#0f172a"
    }
  },
  grid: {
    left: 72,
    right: 56,
    top: 108,
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
    min: 60,
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
      name: "Visits",
      type: "line",
      smooth: true,
      data: [88, 106, 128, 132, 158, 174, 192],
      itemStyle: {
        color: "#2563eb"
      }
    },
    {
      name: "Orders",
      type: "bar",
      barWidth: 22,
      data: [72, 84, 96, 103, 118, 129, 146],
      itemStyle: {
        color: "#f97316"
      }
    }
  ]
});

chart.dispatchAction({
  type: "updateAxisPointer",
  currTrigger: "mousemove",
  x: 650,
  y: 250
});

chart.getZr().refreshImmediately();
canvas.saveToPng("../output/echarts_axis_pointer.png");
console.log("wrote ../output/echarts_axis_pointer.png");
