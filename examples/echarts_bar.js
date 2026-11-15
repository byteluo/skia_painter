const { renderChart } = loadScript("./lib/echarts_setup.js");

const width = 960;
const height = 540;




renderChart({
  option: {
      animation: false,
      backgroundColor: "#f8fafc",
      title: {
        text: "Skia + V8 + ECharts",
        left: "center",
        textStyle: {
          color: "#0f172a",
          fontSize: 24,
          fontWeight: "bold"
        }
      },
      grid: {
        left: 80,
        right: 40,
        top: 90,
        bottom: 70
      },
      tooltip: {},
      xAxis: {
        type: "category",
        data: ["Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"],
        axisLine: {
          lineStyle: {
            color: "#475569"
          }
        },
        axisLabel: {
          color: "#334155"
        }
      },
      yAxis: {
        type: "value",
        axisLine: {
          show: true,
          lineStyle: {
            color: "#475569"
          }
        },
        splitLine: {
          lineStyle: {
            color: "#cbd5e1"
          }
        },
        axisLabel: {
          color: "#334155"
        }
      },
      series: [
        {
          name: "Visits",
          type: "bar",
          barWidth: 42,
          data: [150, 230, 224, 218, 135, 147, 260],
          itemStyle: {
            color: "#2563eb"
          }
        }
      ]
  },
  width,
  height,
  output: "../output/echarts_bar.png",
});
console.log("wrote ../output/echarts_bar.png");
