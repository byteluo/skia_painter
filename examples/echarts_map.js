const { renderChart } = loadScript("./lib/echarts_setup.js");

const width = 960;
const height = 540;



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

echarts.registerMap("demo_regions", demoGeoJson);


renderChart({
  option: {
      animation: false,
      backgroundColor: "#f8fafc",
      title: {
        text: "Map Compatibility",
        subtext: "registerMap + region labels + visualMap",
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
        min: 18,
        max: 82,
        left: 52,
        bottom: 36,
        text: ["High", "Low"],
        inRange: {
          color: ["#bfdbfe", "#60a5fa", "#2563eb"]
        },
        textStyle: {
          color: "#334155"
        }
      },
      series: [
        {
          type: "map",
          map: "demo_regions",
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
            label: {
              color: "#111827"
            },
            itemStyle: {
              areaColor: "#f97316"
            }
          },
          data: [
            { name: "North", value: 72 },
            { name: "West", value: 28 },
            { name: "Central", value: 46 },
            { name: "East", value: 82 }
          ]
        }
      ]
  },
  width,
  height,
  output: "../output/echarts_map.png",
});
console.log("wrote ../output/echarts_map.png");
