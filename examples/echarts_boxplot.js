const { renderChart } = loadScript("./lib/echarts_setup.js");

const width = 960;
const height = 540;




renderChart({
  option: {
      animation: false,
      backgroundColor: "#f8fafc",
      title: {
        text: "Boxplot Compatibility",
        subtext: "Quartiles + whiskers + outliers",
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
      grid: {
        left: 72,
        right: 56,
        top: 108,
        bottom: 64
      },
      xAxis: {
        type: "category",
        data: ["North", "South", "West", "Central"],
        boundaryGap: true,
        axisLabel: {
          color: "#334155"
        }
      },
      yAxis: {
        type: "value",
        min: 0,
        max: 70,
        axisLabel: {
          color: "#334155"
        },
        splitLine: {
          lineStyle: {
            color: "#dbe4f0",
            type: "dashed"
          }
        }
      },
      series: [
        {
          type: "boxplot",
          itemStyle: {
            color: "#bfdbfe",
            borderColor: "#2563eb",
            borderWidth: 2
          },
          emphasis: {
            itemStyle: {
              color: "#93c5fd",
              borderColor: "#1d4ed8"
            }
          },
          data: [
            [12, 18, 26, 33, 42],
            [16, 22, 29, 36, 48],
            [10, 15, 21, 27, 37],
            [20, 25, 31, 40, 52]
          ]
        },
        {
          type: "scatter",
          name: "Outlier",
          symbolSize: 10,
          itemStyle: {
            color: "#f97316"
          },
          data: [
            [0, 46],
            [1, 54],
            [2, 41],
            [3, 59]
          ]
        }
      ]
  },
  width,
  height,
  output: "../output/echarts_boxplot.png",
});
console.log("wrote ../output/echarts_boxplot.png");
