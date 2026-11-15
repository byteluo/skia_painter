const { renderChart } = loadScript("./lib/echarts_setup.js");

const width = 960;
const height = 540;




renderChart({
  option: {
      baseOption: {
        animation: false,
        backgroundColor: "#f8fafc",
        timeline: {
          axisType: "category",
          autoPlay: false,
          top: 18,
          left: 80,
          right: 80,
          data: ["Q1", "Q2", "Q3"],
          label: {
            color: "#334155"
          },
          lineStyle: {
            color: "#94a3b8"
          },
          checkpointStyle: {
            color: "#2563eb",
            borderColor: "#1d4ed8"
          },
          controlStyle: {
            color: "#475569",
            borderColor: "#94a3b8"
          }
        },
        title: {
          text: "Timeline Compatibility",
          subtext: "Timeline component + option switching",
          left: "center",
          top: 64,
          textStyle: {
            color: "#0f172a",
            fontSize: 24,
            fontWeight: "bold"
          },
          subtextStyle: {
            color: "#64748b"
          }
        },
        grid: {
          left: 80,
          right: 56,
          top: 150,
          bottom: 60
        },
        xAxis: {
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
        yAxis: {
          type: "category",
          axisLabel: {
            color: "#334155"
          }
        },
        series: [
          {
            type: "bar",
            label: {
              show: true,
              position: "right",
              color: "#0f172a"
            },
            itemStyle: {
              borderRadius: [0, 8, 8, 0]
            }
          }
        ]
      },
      options: [
        {
          yAxis: { data: ["Search", "Direct", "Ads", "Email"] },
          series: [{ data: [120, 90, 66, 40], itemStyle: { color: "#2563eb" } }]
        },
        {
          yAxis: { data: ["Search", "Direct", "Ads", "Email"] },
          series: [{ data: [138, 102, 72, 52], itemStyle: { color: "#14b8a6" } }]
        },
        {
          yAxis: { data: ["Search", "Direct", "Ads", "Email"] },
          series: [{ data: [154, 118, 88, 60], itemStyle: { color: "#f97316" } }]
        }
      ]
  },
  width,
  height,
  output: "../output/echarts_timeline_bar.png",
  afterSetOption(chart) {
    chart.dispatchAction({ type: "timelineChange", currentIndex: 2 });
  },
});
console.log("wrote ../output/echarts_timeline_bar.png");
