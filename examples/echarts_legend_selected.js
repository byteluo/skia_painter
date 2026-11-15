const { renderChart } = loadScript("./lib/echarts_setup.js");

const width = 960;
const height = 540;




renderChart({
  option: {
      animation: false,
      backgroundColor: "#f8fafc",
      title: {
        text: "Legend Selected Compatibility",
        subtext: "Legend selected mode + hidden series state",
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
        selected: {
          Ads: false,
          Email: false
        },
        textStyle: {
          color: "#334155"
        }
      },
      tooltip: {
        trigger: "axis"
      },
      grid: {
        left: 72,
        right: 56,
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
          name: "Search",
          type: "line",
          smooth: true,
          data: [92, 108, 126, 136, 152, 168],
          itemStyle: { color: "#2563eb" }
        },
        {
          name: "Direct",
          type: "line",
          smooth: true,
          data: [74, 80, 92, 101, 112, 124],
          itemStyle: { color: "#14b8a6" }
        },
        {
          name: "Ads",
          type: "line",
          smooth: true,
          data: [58, 66, 79, 82, 96, 108],
          itemStyle: { color: "#f97316" }
        },
        {
          name: "Email",
          type: "line",
          smooth: true,
          data: [32, 36, 44, 51, 56, 62],
          itemStyle: { color: "#8b5cf6" }
        }
      ]
  },
  width,
  height,
  output: "../output/echarts_legend_selected.png",
});
console.log("wrote ../output/echarts_legend_selected.png");
