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
      geometry: {
        type: "Polygon",
        coordinates: [[[0, 6], [4, 6], [4, 10], [0, 10], [0, 6]]]
      }
    },
    {
      type: "Feature",
      properties: { name: "West" },
      geometry: {
        type: "Polygon",
        coordinates: [[[0, 1], [4, 1], [4, 6], [0, 6], [0, 1]]]
      }
    },
    {
      type: "Feature",
      properties: { name: "Central" },
      geometry: {
        type: "Polygon",
        coordinates: [[[4, 1], [7, 1], [7, 6], [4, 6], [4, 1]]]
      }
    },
    {
      type: "Feature",
      properties: { name: "East" },
      geometry: {
        type: "Polygon",
        coordinates: [[[7, 1], [11, 1], [11, 8], [7, 8], [7, 1]]]
      }
    }
  ]
};

echarts.registerMap("geo_effect_regions", demoGeoJson);

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
    text: "Geo Effect Scatter Compatibility",
    subtext: "geo component + effectScatter overlay",
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
  geo: {
    map: "geo_effect_regions",
    left: "center",
    top: 100,
    width: "72%",
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
    },
    emphasis: {
      itemStyle: {
        areaColor: "#bfdbfe"
      }
    }
  },
  series: [
    {
      type: "effectScatter",
      coordinateSystem: "geo",
      data: [
        { name: "North Hub", value: [2, 8, 82] },
        { name: "Central Hub", value: [5.5, 3.5, 66] },
        { name: "East Hub", value: [9, 5.5, 94] }
      ],
      symbolSize(value) {
        return value[2] / 5;
      },
      showEffectOn: "emphasis",
      rippleEffect: {
        scale: 3.2,
        brushType: "stroke"
      },
      itemStyle: {
        color: "#f97316",
        shadowBlur: 20,
        shadowColor: "rgba(249, 115, 22, 0.3)"
      },
      label: {
        show: true,
        position: "right",
        formatter(param) {
          return param.data.name;
        },
        color: "#0f172a",
        fontWeight: "bold"
      }
    }
  ]
});

chart.getZr().refreshImmediately();
canvas.saveToPng("../output/echarts_geo_effect_scatter.png");
console.log("wrote ../output/echarts_geo_effect_scatter.png");
