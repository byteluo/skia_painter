const { renderChart } = loadScript("./lib/echarts_setup.js");

const width = 960;
const height = 540;




renderChart({
  option: {
      animation: false,
      backgroundColor: "#f8fafc",
      title: {
        text: "Parallel Compatibility",
        subtext: "Multi-axis polylines + labels",
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
      parallelAxis: [
        { dim: 0, name: "CPU", max: 100, nameTextStyle: { color: "#334155" } },
        { dim: 1, name: "Memory", max: 100, nameTextStyle: { color: "#334155" } },
        { dim: 2, name: "IO", max: 100, nameTextStyle: { color: "#334155" } },
        { dim: 3, name: "Network", max: 100, nameTextStyle: { color: "#334155" } },
        { dim: 4, name: "FPS", max: 100, nameTextStyle: { color: "#334155" } }
      ],
      parallel: {
        left: 88,
        right: 72,
        top: 110,
        bottom: 60,
        parallelAxisDefault: {
          axisLine: {
            lineStyle: {
              color: "#94a3b8",
              width: 2
            }
          },
          axisTick: {
            lineStyle: {
              color: "#cbd5e1"
            }
          },
          axisLabel: {
            color: "#475569"
          },
          splitLine: {
            show: false
          }
        }
      },
      series: [
        {
          type: "parallel",
          smooth: true,
          lineStyle: {
            width: 3,
            opacity: 0.55
          },
          data: [
            { value: [82, 74, 66, 58, 90], lineStyle: { color: "#2563eb" } },
            { value: [64, 52, 78, 70, 72], lineStyle: { color: "#14b8a6" } },
            { value: [48, 68, 58, 84, 64], lineStyle: { color: "#f97316" } },
            { value: [90, 80, 72, 62, 96], lineStyle: { color: "#8b5cf6" } }
          ]
        }
      ]
  },
  width,
  height,
  output: "../output/echarts_parallel.png",
});
console.log("wrote ../output/echarts_parallel.png");
