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

const demoGeoJson = {
  type: "FeatureCollection",
  features: [
    {
      type: "Feature",
      properties: { name: "North" },
      geometry: { type: "Polygon", coordinates: [[[0, 6], [4, 6], [4, 10], [0, 10], [0, 6]]] }
    },
    {
      type: "Feature",
      properties: { name: "West" },
      geometry: { type: "Polygon", coordinates: [[[0, 1], [4, 1], [4, 6], [0, 6], [0, 1]]] }
    },
    {
      type: "Feature",
      properties: { name: "Central" },
      geometry: { type: "Polygon", coordinates: [[[4, 1], [7, 1], [7, 6], [4, 6], [4, 1]]] }
    },
    {
      type: "Feature",
      properties: { name: "East" },
      geometry: { type: "Polygon", coordinates: [[[7, 1], [11, 1], [11, 8], [7, 8], [7, 1]]] }
    }
  ]
};

echarts.registerMap("visualmap_map_regions", demoGeoJson);

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
    text: "VisualMap Map Scatter Compatibility",
    subtext: "Map regions + scatter overlay + continuous visualMap",
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
    trigger: "item"
  },
  visualMap: {
    min: 20,
    max: 100,
    orient: "horizontal",
    left: "center",
    top: 72,
    inRange: {
      color: ["#bfdbfe", "#60a5fa", "#2563eb", "#0f766e"]
    },
    textStyle: {
      color: "#334155"
    }
  },
  geo: {
    map: "visualmap_map_regions",
    left: "center",
    top: 116,
    width: "70%",
    roam: false,
    label: {
      show: true,
      color: "#0f172a",
      fontWeight: "bold"
    },
    itemStyle: {
      areaColor: "#e2e8f0",
      borderColor: "#ffffff",
      borderWidth: 3
    }
  },
  series: [
    {
      type: "scatter",
      coordinateSystem: "geo",
      data: [
        { name: "North Hub", value: [2, 8, 44] },
        { name: "West Hub", value: [2, 3.5, 31] },
        { name: "Central Hub", value: [5.5, 3.5, 68] },
        { name: "East Hub", value: [9, 5.5, 92] }
      ],
      symbolSize(value) {
        return 14 + value[2] / 8;
      },
      label: {
        show: true,
        position: "right",
        formatter(param) {
          return param.data.name;
        },
        color: "#0f172a",
        fontWeight: "bold"
      },
      itemStyle: {
        shadowBlur: 16,
        shadowColor: "rgba(15, 23, 42, 0.18)"
      }
    }
  ]
});

chart.getZr().refreshImmediately();
canvas.saveToPng("../output/echarts_visualmap_map_scatter.png");
console.log("wrote ../output/echarts_visualmap_map_scatter.png");
