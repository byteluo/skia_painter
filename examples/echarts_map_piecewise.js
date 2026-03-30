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

echarts.registerMap("piecewise_regions", demoGeoJson);

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
    text: "Map Piecewise VisualMap Compatibility",
    subtext: "Map + piecewise categories",
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
    trigger: "item"
  },
  visualMap: {
    type: "piecewise",
    left: 40,
    bottom: 28,
    textStyle: {
      color: "#334155"
    },
    pieces: [
      { min: 80, label: "Tier A", color: "#1d4ed8" },
      { min: 60, max: 79, label: "Tier B", color: "#60a5fa" },
      { min: 40, max: 59, label: "Tier C", color: "#f97316" },
      { max: 39, label: "Tier D", color: "#fecaca" }
    ]
  },
  series: [
    {
      type: "map",
      map: "piecewise_regions",
      left: "center",
      top: 96,
      width: "74%",
      roam: false,
      label: {
        show: true,
        color: "#0f172a",
        fontWeight: "bold"
      },
      itemStyle: {
        borderColor: "#ffffff",
        borderWidth: 3
      },
      emphasis: {
        itemStyle: {
          areaColor: "#0f766e"
        }
      },
      data: [
        { name: "North", value: 78 },
        { name: "West", value: 34 },
        { name: "Central", value: 52 },
        { name: "East", value: 91 }
      ]
    }
  ]
});

chart.getZr().refreshImmediately();
canvas.saveToPng("../output/echarts_map_piecewise.png");
console.log("wrote ../output/echarts_map_piecewise.png");
