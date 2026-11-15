const { renderChart } = loadScript("./lib/echarts_setup.js");

const width = 960;
const height = 540;




renderChart({
  option: {
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
  },
  width,
  height,
  output: "../output/echarts_polar_bar.png",
});
console.log("wrote ../output/echarts_polar_bar.png");
