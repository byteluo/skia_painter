const { renderChart } = loadScript("./lib/echarts_setup.js");

const width = 960;
const height = 540;




renderChart({
  option: {
      animation: false,
      backgroundColor: "#f8fafc",
      title: {
        text: "Toolbox Compatibility",
        subtext: "Toolbox icons + legend + smooth lines",
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
      legend: {
        top: 66,
        textStyle: {
          color: "#334155"
        }
      },
      toolbox: {
        right: 28,
        top: 18,
        itemSize: 20,
        iconStyle: {
          borderColor: "#475569"
        },
        feature: {
          dataView: { show: true, readOnly: true },
          restore: { show: true },
          saveAsImage: { show: true }
        }
      },
      grid: {
        left: 72,
        right: 72,
        top: 116,
        bottom: 64
      },
      xAxis: {
        type: "category",
        data: ["Jan", "Feb", "Mar", "Apr", "May", "Jun"],
        axisLabel: {
          color: "#334155"
        }
      },
      yAxis: {
        type: "value",
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
          name: "Visits",
          type: "line",
          smooth: true,
          data: [82, 96, 90, 110, 124, 136],
          itemStyle: {
            color: "#2563eb"
          }
        },
        {
          name: "Orders",
          type: "line",
          smooth: true,
          data: [56, 62, 74, 79, 88, 101],
          itemStyle: {
            color: "#f97316"
          }
        }
      ]
  },
  width,
  height,
  output: "../output/echarts_toolbox.png",
});
console.log("wrote ../output/echarts_toolbox.png");
