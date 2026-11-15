const { renderChart } = loadScript("./lib/echarts_setup.js");

const width = 960;
const height = 540;



echarts.registerMap("mini-grid", {
  type: "FeatureCollection",
  features: [
    {
      type: "Feature",
      id: "north-west",
      properties: { name: "North West" },
      geometry: {
        type: "Polygon",
        coordinates: [[[100, 40], [110, 40], [110, 30], [100, 30], [100, 40]]]
      }
    },
    {
      type: "Feature",
      id: "north-east",
      properties: { name: "North East" },
      geometry: {
        type: "Polygon",
        coordinates: [[[110, 40], [120, 40], [120, 30], [110, 30], [110, 40]]]
      }
    },
    {
      type: "Feature",
      id: "south-west",
      properties: { name: "South West" },
      geometry: {
        type: "Polygon",
        coordinates: [[[100, 30], [110, 30], [110, 20], [100, 20], [100, 30]]]
      }
    },
    {
      type: "Feature",
      id: "south-east",
      properties: { name: "South East" },
      geometry: {
        type: "Polygon",
        coordinates: [[[110, 30], [120, 30], [120, 20], [110, 20], [110, 30]]]
      }
    }
  ]
});


renderChart({
  option: {
      animation: false,
      backgroundColor: "#f8fafc",
      title: {
        text: "Geo Heatmap Compatibility",
        subtext: "Exercises getImageData / putImageData path",
        left: "center",
        top: 18,
        textStyle: {
          color: "#0f172a",
          fontSize: 24,
          fontWeight: "bold"
        },
        subtextStyle: {
          color: "#64748b",
          fontSize: 12
        }
      },
      tooltip: {
        trigger: "item"
      },
      geo: {
        map: "mini-grid",
        roam: false,
        left: 120,
        right: 120,
        top: 96,
        bottom: 40,
        itemStyle: {
          areaColor: "#e2e8f0",
          borderColor: "#94a3b8",
          borderWidth: 2
        },
        emphasis: {
          itemStyle: {
            areaColor: "#cbd5e1"
          }
        }
      },
      visualMap: {
        min: 0,
        max: 100,
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
          coordinateSystem: "geo",
          pointSize: 26,
          blurSize: 20,
          data: [
            { name: "NW-1", value: [103, 36, 20] },
            { name: "NW-2", value: [107, 33, 48] },
            { name: "NE-1", value: [114, 36, 72] },
            { name: "NE-2", value: [118, 34, 94] },
            { name: "SW-1", value: [104, 27, 34] },
            { name: "SW-2", value: [108, 24, 56] },
            { name: "SE-1", value: [114, 27, 64] },
            { name: "SE-2", value: [118, 24, 88] }
          ]
        }
      ]
  },
  width,
  height,
  output: "../output/echarts_geo_heatmap.png",
});
console.log("wrote ../output/echarts_geo_heatmap.png");
