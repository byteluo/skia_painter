const { renderChart } = loadScript("./lib/echarts_setup.js");

const width = 960;
const height = 540;




renderChart({
  option: {
      animation: false,
      backgroundColor: "#f8fafc",
      title: {
        text: "Weekly Traffic Trend",
        left: "center",
        textStyle: {
          color: "#0f172a",
          fontSize: 24,
          fontWeight: "bold"
        }
      },
      grid: {
        left: 72,
        right: 48,
        top: 96,
        bottom: 64
      },
      xAxis: {
        type: "category",
        boundaryGap: false,
        data: ["Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"],
        axisLine: {
          lineStyle: {
            color: "#64748b",
            type: "dashed"
          }
        },
        splitLine: {
          show: true,
          lineStyle: {
            color: "#cbd5e1",
            type: "dashed"
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
            color: "#64748b"
          }
        },
        splitLine: {
          lineStyle: {
            color: "#cbd5e1",
            type: [6, 4]
          }
        },
        axisLabel: {
          color: "#334155"
        }
      },
      series: [
        {
          name: "Visits",
          type: "line",
          smooth: true,
          symbol: "circle",
          symbolSize: 10,
          lineStyle: {
            color: "#2563eb",
            width: 4,
            type: "dashed"
          },
          itemStyle: {
            color: "#1d4ed8"
          },
          areaStyle: {
            color: new echarts.graphic.LinearGradient(0, 0, 0, 1, [
              { offset: 0, color: "rgba(37, 99, 235, 0.35)" },
              { offset: 1, color: "rgba(37, 99, 235, 0.04)" }
            ])
          },
          data: [180, 232, 201, 234, 290, 330, 310]
        }
      ]
  },
  width,
  height,
  output: "../output/echarts_line.png",
});
console.log("wrote ../output/echarts_line.png");
