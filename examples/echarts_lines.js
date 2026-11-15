const { renderChart } = loadScript("./lib/echarts_setup.js");

const width = 960;
const height = 540;




renderChart({
  option: {
      animation: false,
      backgroundColor: "#f8fafc",
      title: {
        text: "Lines Compatibility",
        subtext: "Curves + arrows + effect",
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
      xAxis: {
        type: "value",
        min: 0,
        max: 100,
        splitLine: {
          lineStyle: {
            color: "#dbe4f0",
            type: "dashed"
          }
        },
        axisLabel: {
          color: "#334155"
        }
      },
      yAxis: {
        type: "value",
        min: 0,
        max: 100,
        splitLine: {
          lineStyle: {
            color: "#dbe4f0",
            type: "dashed"
          }
        },
        axisLabel: {
          color: "#334155"
        }
      },
      series: [
        {
          type: "lines",
          coordinateSystem: "cartesian2d",
          polyline: false,
          effect: {
            show: true,
            constantSpeed: 28,
            symbol: "arrow",
            symbolSize: 10,
            trailLength: 0
          },
          lineStyle: {
            width: 3,
            opacity: 0.7,
            curveness: 0.28,
            color: "#2563eb"
          },
          data: [
            { coords: [[12, 20], [32, 76]] },
            { coords: [[22, 70], [48, 30]] },
            { coords: [[38, 18], [66, 82]] },
            { coords: [[54, 72], [82, 26]] },
            { coords: [[72, 40], [92, 78]] }
          ]
        }
      ]
  },
  width,
  height,
  output: "../output/echarts_lines.png",
});
console.log("wrote ../output/echarts_lines.png");
