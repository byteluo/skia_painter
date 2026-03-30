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

const days = ["Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun", "Mon2", "Tue2", "Wed2"];
const values = [120, 132, 101, 134, 90, 230, 210, 188, 166, 142];

chart.setOption({
  animation: false,
  backgroundColor: "#fffaf0",
  title: {
    text: "DataZoom + MarkArea Compatibility",
    subtext: "Slider + shaded region + line labels",
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
  grid: {
    left: 70,
    right: 50,
    top: 100,
    bottom: 110
  },
  xAxis: {
    type: "category",
    data: days,
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
  dataZoom: [
    {
      type: "slider",
      bottom: 34,
      height: 34,
      start: 20,
      end: 80,
      fillerColor: "rgba(37, 99, 235, 0.18)",
      borderColor: "#cbd5e1",
      textStyle: {
        color: "#475569"
      }
    }
  ],
  series: [
    {
      type: "line",
      smooth: true,
      data: values,
      symbol: "circle",
      symbolSize: 9,
      lineStyle: {
        color: "#2563eb",
        width: 4
      },
      itemStyle: {
        color: "#1d4ed8"
      },
      areaStyle: {
        color: "rgba(37, 99, 235, 0.10)"
      },
      markArea: {
        itemStyle: {
          color: "rgba(249, 115, 22, 0.14)"
        },
        label: {
          color: "#9a3412",
          fontWeight: "bold"
        },
        data: [
          [
            { name: "Campaign", xAxis: "Thu" },
            { xAxis: "Sat" }
          ]
        ]
      },
      markLine: {
        symbol: ["none", "none"],
        lineStyle: {
          color: "#ef4444",
          type: "dashed",
          width: 2
        },
        label: {
          color: "#ef4444"
        },
        data: [{ yAxis: 160, name: "Target" }]
      }
    }
  ]
});

chart.dispatchAction({
  type: "dataZoom",
  start: 30,
  end: 90
});
chart.getZr().refreshImmediately();
canvas.saveToPng("../output/echarts_datazoom_markarea.png");
console.log("wrote ../output/echarts_datazoom_markarea.png");
